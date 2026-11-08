/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: The POSIX calls of the statically linked Mesa and libdrm code,
          on exec, dos and timer.device, so that radeonsi.amdd needs no
          per-task C runtime (posixc) and any task may open and call it.
          A process may read files (libdrm's DEVS:amdgpu.ids); writing
          files and starting processes are not offered, and the paths that
          ask for them (debug dumps, traces, the disk cache) fail and carry
          on.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/var.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

struct Device *TimerBase;

static struct timerequest amdd_timereq;
static struct SignalSemaphore amdd_env_lock;

struct amdd_env
{
    struct amdd_env *next;
    char            *value;     /* NULL: unset */
    char            name[1];
};
static struct amdd_env *amdd_env_list;

/* ---- time ---- */

int clock_gettime(clockid_t clk, struct timespec *ts)
{
    struct timeval tv;

    if (!TimerBase || !ts)
    {
        errno = EINVAL;
        return -1;
    }
    if (clk == CLOCK_REALTIME)
        GetSysTime(&tv);
    else
        GetUpTime(&tv);
    ts->tv_sec = tv.tv_secs;
    ts->tv_nsec = tv.tv_micro * 1000;
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    struct timerequest tr;
    struct MsgPort port;
    BYTE sig;

    if (rem)
        rem->tv_sec = rem->tv_nsec = 0;
    if (!req || !TimerBase)
        return 0;
    sig = AllocSignal(-1);
    if (sig < 0)
        return 0;
    memset(&port, 0, sizeof(port));
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_Flags = PA_SIGNAL;
    port.mp_SigBit = sig;
    port.mp_SigTask = FindTask(NULL);
    NEWLIST(&port.mp_MsgList);
    tr = amdd_timereq;
    tr.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    tr.tr_node.io_Message.mn_ReplyPort = &port;
    tr.tr_node.io_Command = TR_ADDREQUEST;
    tr.tr_time.tv_secs = req->tv_sec;
    tr.tr_time.tv_micro = req->tv_nsec / 1000;
    if (tr.tr_time.tv_secs || tr.tr_time.tv_micro)
        DoIO((struct IORequest *)&tr);
    FreeSignal(sig);
    return 0;
}

/* ---- process ---- */

pid_t getpid(void)
{
    return (pid_t)((IPTR)FindTask(NULL) & 0x7fffffff);
}

uid_t getuid(void)  { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void)  { return 0; }
gid_t getegid(void) { return 0; }

int uname(struct utsname *u)
{
    memset(u, 0, sizeof(*u));
    strcpy(u->sysname, "AROS");
    strcpy(u->nodename, "aros");
    strcpy(u->release, "1");
    strcpy(u->version, "1");
#if defined(__x86_64__)
    strcpy(u->machine, "x86_64");
#elif defined(__i386__)
    strcpy(u->machine, "i386");
#elif defined(__aarch64__)
    strcpy(u->machine, "aarch64");
#else
    strcpy(u->machine, "unknown");
#endif
    return 0;
}

long sysconf(int name)
{
    switch (name)
    {
        case _SC_NPROCESSORS_CONF:
        case _SC_NPROCESSORS_ONLN:
            return 1;
        case _SC_PAGESIZE:
            return 4096;
        case _SC_PHYS_PAGES:
            return (long)(AvailMem(MEMF_TOTAL) / 4096);
        default:
            errno = EINVAL;
            return -1;
    }
}

/* Mesa's threads mask signals at creation; AROS tasks have none to mask */
int sigprocmask(int how, const sigset_t *set, sigset_t *old)
{
    if (old)
        memset(old, 0, sizeof(*old));
    return 0;
}

int sigfillset(sigset_t *set)
{
    memset(set, 0xff, sizeof(*set));
    return 0;
}

int sigdelset(sigset_t *set, int sig)
{
    return 0;
}

int getentropy(void *buf, size_t len)
{
    static ULONG seed;
    struct EClockVal ev;
    UBYTE *p = buf;

    if (TimerBase)
    {
        ReadEClock(&ev);
        seed ^= ev.ev_lo ^ (ev.ev_hi << 16);
    }
    seed ^= (ULONG)(IPTR)FindTask(NULL);
    while (len--)
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        *p++ = (UBYTE)seed;
    }
    return 0;
}

/* ---- environment: AROS variables, read by a process; the values seen
   and the ones Mesa sets are kept, so a task sees what was read before ---- */

static struct amdd_env *amdd_env_find(const char *name)
{
    struct amdd_env *e;

    for (e = amdd_env_list; e; e = e->next)
        if (!strcmp(e->name, name))
            return e;
    return NULL;
}

static struct amdd_env *amdd_env_add(const char *name, const char *value)
{
    struct amdd_env *e = AllocVec(sizeof(*e) + strlen(name), MEMF_ANY | MEMF_CLEAR);

    if (!e)
        return NULL;
    strcpy(e->name, name);
    if (value && (e->value = AllocVec(strlen(value) + 1, MEMF_ANY)))
        strcpy(e->value, value);
    e->next = amdd_env_list;
    amdd_env_list = e;
    return e;
}

char *__posixc_getenv(const char *name)
{
    struct amdd_env *e;
    struct Task *me = FindTask(NULL);
    char buf[256];
    char *value = NULL;

    if (!name)
        return NULL;
    ObtainSemaphore(&amdd_env_lock);
    if ((e = amdd_env_find(name)))
        value = e->value;
    else if (me->tc_Node.ln_Type == NT_PROCESS && GetVar(name, buf, sizeof(buf), 0) >= 0 &&
             (e = amdd_env_add(name, buf)))
        value = e->value;
    ReleaseSemaphore(&amdd_env_lock);
    return value;
}

int setenv(const char *name, const char *value, int overwrite)
{
    struct amdd_env *e;
    int ret = 0;

    ObtainSemaphore(&amdd_env_lock);
    e = amdd_env_find(name);
    if (e && (overwrite || !e->value))
    {
        FreeVec(e->value);
        e->value = NULL;
        if (value && (e->value = AllocVec(strlen(value) + 1, MEMF_ANY)))
            strcpy(e->value, value);
    }
    else if (!e && !amdd_env_add(name, value))
        ret = -1;
    ReleaseSemaphore(&amdd_env_lock);
    return ret;
}

int unsetenv(const char *name)
{
    struct amdd_env *e;

    ObtainSemaphore(&amdd_env_lock);
    if ((e = amdd_env_find(name)))
    {
        FreeVec(e->value);
        e->value = NULL;
    }
    else
        amdd_env_add(name, NULL);
    ReleaseSemaphore(&amdd_env_lock);
    return 0;
}

/* ---- strings ---- */

char *strtok_r(char *str, const char *delim, char **save)
{
    char *end;

    if (!str)
        str = *save;
    if (!str)
        return NULL;
    str += strspn(str, delim);
    if (!*str)
    {
        *save = NULL;
        return NULL;
    }
    end = str + strcspn(str, delim);
    if (*end)
        *end++ = '\0';
    *save = end;
    return str;
}

int strerror_r(int err, char *buf, size_t len)
{
    const char *s = strerror(err);

    if (!len)
        return ERANGE;
    strncpy(buf, s ? s : "", len - 1);
    buf[len - 1] = '\0';
    return 0;
}

/* zlib's crc32, for Mesa's hashes */
unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
{
    static ULONG table[256];
    static BOOL ready;
    ULONG c;
    unsigned int i, k;

    if (!ready)
    {
        for (i = 0; i < 256; i++)
        {
            c = i;
            for (k = 0; k < 8; k++)
                c = (c & 1) ? 0xedb88320UL ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        ready = TRUE;
    }
    if (!buf)
        return 0;
    c = (ULONG)crc ^ 0xffffffffUL;
    while (len--)
        c = table[(c ^ *buf++) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffUL;
}

/* ---- files: read only, from a process ---- */

static UBYTE amdd_stdstream;

struct amdd_file
{
    BPTR    fh;
    BOOL    eof;
};

void *__posixc_getstdout(void) { return &amdd_stdstream; }
void *__posixc_getstderr(void) { return &amdd_stdstream; }

static struct amdd_file *amdd_file(void *f)
{
    return (f && f != (void *)&amdd_stdstream) ? f : NULL;
}

void *__posixc_fopen(const char *path, const char *mode)
{
    struct amdd_file *f;
    BPTR fh;

    if (!path || !mode || mode[0] != 'r' || strchr(mode, '+') ||
        FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS)
    {
        errno = (mode && mode[0] == 'r') ? ENOENT : EROFS;
        return NULL;
    }
    if (!(fh = Open(path, MODE_OLDFILE)))
    {
        errno = ENOENT;
        return NULL;
    }
    if (!(f = AllocVec(sizeof(*f), MEMF_ANY | MEMF_CLEAR)))
    {
        Close(fh);
        errno = ENOMEM;
        return NULL;
    }
    f->fh = fh;
    return f;
}

int __posixc_fclose(void *stream)
{
    struct amdd_file *f = amdd_file(stream);

    if (f)
    {
        Close(f->fh);
        FreeVec(f);
    }
    return 0;
}

size_t __posixc_fread(void *p, size_t s, size_t n, void *stream)
{
    struct amdd_file *f = amdd_file(stream);
    LONG got;

    if (!f || !s || !n)
        return 0;
    got = FRead(f->fh, p, s, n);
    if (got <= 0)
    {
        f->eof = TRUE;
        return 0;
    }
    return got;
}

char *__posixc_fgets(char *buf, int n, void *stream)
{
    struct amdd_file *f = amdd_file(stream);

    if (!f || n <= 0 || !FGets(f->fh, buf, n))
    {
        if (f)
            f->eof = TRUE;
        return NULL;
    }
    return buf;
}

ssize_t getline(char **line, size_t *n, void *stream)
{
    struct amdd_file *f = amdd_file(stream);
    size_t len = 0;
    LONG c;

    if (!f || !line || !n)
    {
        errno = EINVAL;
        return -1;
    }
    while ((c = FGetC(f->fh)) >= 0)
    {
        if (len + 2 > *n)
        {
            size_t size = *n ? *n * 2 : 128;
            char *bigger = realloc(*line, size);

            if (!bigger)
                return -1;
            *line = bigger;
            *n = size;
        }
        (*line)[len++] = (char)c;
        if (c == '\n')
            break;
    }
    if (!len)
    {
        f->eof = TRUE;
        return -1;
    }
    (*line)[len] = '\0';
    return len;
}

int __posixc_fseek(void *stream, long off, int whence)
{
    struct amdd_file *f = amdd_file(stream);
    LONG mode = whence == SEEK_SET ? OFFSET_BEGINNING : whence == SEEK_CUR ? OFFSET_CURRENT : OFFSET_END;

    if (!f)
    {
        errno = EBADF;
        return -1;
    }
    Flush(f->fh);
    if (Seek(f->fh, off, mode) < 0)
        return -1;
    f->eof = FALSE;
    return 0;
}

long __posixc_ftell(void *stream)
{
    struct amdd_file *f = amdd_file(stream);

    if (!f)
    {
        errno = EBADF;
        return -1;
    }
    Flush(f->fh);
    return Seek(f->fh, 0, OFFSET_CURRENT);
}

/* ---- processes and file descriptors: not offered ---- */

void *__posixc_tmpfile(void)                             { errno = EROFS; return NULL; }
void *fdopen(int fd, const char *mode)                   { errno = EBADF; return NULL; }
void *popen(const char *cmd, const char *mode)           { errno = ENOSYS; return NULL; }
int pclose(void *f)                                      { return -1; }
int __posixc_system(const char *cmd)                     { return -1; }

int open(const char *path, int flags, ...)               { errno = ENOENT; return -1; }
int close(int fd)                                        { errno = EBADF; return -1; }
ssize_t read(int fd, void *buf, size_t n)                { errno = EBADF; return -1; }
ssize_t write(int fd, const void *buf, size_t n)         { errno = EBADF; return -1; }
off_t lseek(int fd, off_t off, int whence)               { errno = EBADF; return -1; }
int fstat(int fd, void *st)                              { errno = EBADF; return -1; }
int fcntl(int fd, int cmd, ...)                          { errno = EBADF; return -1; }
int ioctl(int fd, unsigned long req, ...)                { errno = ENOTTY; return -1; }
int access(const char *path, int mode)                   { errno = ENOENT; return -1; }
int mkdir(const char *path, mode_t mode)                 { errno = EROFS; return -1; }
int mkstemp(char *tmpl)                                  { errno = EROFS; return -1; }
int unlink(const char *path)                             { errno = ENOENT; return -1; }
ssize_t readlink(const char *path, char *buf, size_t n)  { errno = ENOENT; return -1; }

/* ---- init ---- */

static int amdd_posix_init(void)
{
    InitSemaphore(&amdd_env_lock);
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&amdd_timereq, 0) == 0)
        TimerBase = amdd_timereq.tr_node.io_Device;
    return TRUE;
}

static int amdd_posix_exit(void)
{
    struct amdd_env *e, *next;

    if (TimerBase)
    {
        CloseDevice((struct IORequest *)&amdd_timereq);
        TimerBase = NULL;
    }
    for (e = amdd_env_list; e; e = next)
    {
        next = e->next;
        FreeVec(e->value);
        FreeVec(e);
    }
    amdd_env_list = NULL;
    return TRUE;
}

ADD2INIT(amdd_posix_init, 0);
ADD2EXIT(amdd_posix_exit, 0);
