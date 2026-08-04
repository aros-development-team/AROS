/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Generic POSIX/libc and libdrm shims for the Mesa gallium drivers
    statically linked into mesa3dgl.library. Mesa references a handful of
    POSIX entry points (close/mmap/fcntl/sysconf/clock_gettime/getenv) and
    the libdrm userland API (drmIoctl / drmPrime* / drmSyncobj*); AROS
    provides none natively. These shims either no-op or route through the
    AROS bridge (aros_drm_bridge), so they are driver-agnostic.

    Per-driver glue — screen creation, the present/display path that
    dereferences the driver's Mesa resource types, and the driver's own
    linker stubs — lives in aros_drm_<driver>.c (e.g. aros_drm_vc4.c).

    Only compiled in when MESA3DGL_GALLIUMCORE serves a DRM-needing driver
    (currently gallium_vc4 on raspi); other builds keep arosc's libc.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/types/timespec_s.h>   /* struct timespec for clock_gettime() */

#include <errno.h>
#include <string.h>

#include "vc4_aros_bridge.h"

/* Per-process current bridge: the driver's screen-create glue sets it,
 * the shims below dispatch through it. Single screen per process, same
 * limit as the old in-hidd single fd_storage. */
struct vc4_aros_bridge *aros_drm_bridge = NULL;

/* Serial-log helper for instrumentation inside Mesa driver code (which
 * can't easily include AROS debug headers). */
#include <stdarg.h>
#include <stdio.h>
void vc4_aros_log(const char *fmt, ...)
{
    char lbuf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(lbuf, sizeof(lbuf), fmt, ap);
    va_end(ap);
    bug("%s", lbuf);
}

/* Intercept Mesa's stderr diagnostics (Mesa is statically linked into
 * mesa3dgl and calls fprintf/vfprintf/fputs for GL_INVALID_ENUM etc. that
 * apps trigger by using GL features vc4's GL 2.1 lacks). Routed to the
 * serial log rather than posixc stdio: a strong def here wins over the
 * posixc linklib for Mesa's calls inside mesa3dgl, and going through
 * posixc stdio from library context crashed (its per-task FILE state is
 * unusable there).
 *
 * Always logged, never D()-gated: release Mesa only writes to stderr for
 * things worth seeing - assertion failures, unreachable() and driver
 * warnings. Swallowing them cost a long debugging session where an
 * assertion fired, its message went nowhere, and the only evidence left
 * was an illegal instruction (see the abort() note below). */
int fprintf(FILE *stream, const char *fmt, ...)
{
    char lbuf[512];
    va_list ap;

    (void)stream;
    va_start(ap, fmt);
    vsnprintf(lbuf, sizeof(lbuf), fmt, ap);
    va_end(ap);
    bug("[mesa] %s", lbuf);
    return 0;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
    char lbuf[512];

    (void)stream;
    vsnprintf(lbuf, sizeof(lbuf), fmt, ap);
    bug("[mesa] %s", lbuf);
    return 0;
}

int fputs(const char *s, FILE *stream)
{
    (void)stream;
    bug("[mesa] %s\n", s ? s : "(null)");
    return 0;
}

/* Mesa calls abort() from assert() and unreachable(). stdc's abort()
 * documents that it must not be used from a shared library: it longjmps
 * to the exit point installed by a program's startup code, which does
 * not exist in our context, so it RETURNS. The compiler treats abort()
 * as noreturn and emits no return path, so control then falls out of the
 * function into whatever follows it - a literal pool, in the case that
 * sent us hunting an "Illegal instruction" at a float constant.
 *
 * Terminate the offending task instead, after saying so. RemTask(NULL)
 * does not return and leaves the rest of the system alive, which beats
 * both silently continuing and taking the machine down. */
void abort(void)
{
    bug("[mesa] abort() called - killing task '%s'\n",
        FindTask(NULL)->tc_Node.ln_Name ?
            FindTask(NULL)->tc_Node.ln_Name : "?");

    RemTask(NULL);

    /* RemTask(NULL) never returns; keep the compiler's noreturn
     * contract intact if it ever did. */
    for (;;) ;
}

/*
 * Per-task stdc base resolver, shadowing the linklib autoinit's version
 * (a direct object wins over an archive member). stdc.library hands out
 * PER-TASK bases, but the stock lazy autoinit caches the FIRST caller
 * task's base in a resident global — after that process dies, Mesa's
 * libc calls (fprintf(stderr) etc.) go through the dead base and crash
 * the next GL app run. This version re-opens whenever the calling task
 * changes. Opens are deliberately never closed: per-task bases must
 * outlive process exit (the libc exit sequence itself still calls
 * through them after the autoclose point).
 */
void *__aros_getbase_StdCBase(void)
{
    static void *stdcbase;
    static struct Task *stdctask;
    struct Task *me = FindTask(NULL);

    if (!stdcbase || stdctask != me)
    {
        void *lb = OpenLibrary((STRPTR)"stdc.library", 0);
        if (lb)
        {
            stdcbase = lb;
            stdctask = me;
        }
    }
    return stdcbase;
}

/* close() the fake DRM fd at teardown — no real fd, route to bridge. */
int close(int fd)
{
    if (aros_drm_bridge)
        aros_drm_bridge->close(aros_drm_bridge->ctx);
    return 0;
}

/*
 * getenv() override. Force NIR_VALIDATE=0: the 20.0.8 validator aborts
 * on a benign GLSL type pointer mismatch (type singletons weren't shared
 * across mesa3dgl/driver linklibs pre-refactor; kept as belt-and-braces).
 * Unrecognised vars return NULL, which Mesa treats as "unset".
 */
char *getenv(const char *name)
{
    if (!name)
        return (char *)0;

    if (strcmp(name, "NIR_VALIDATE") == 0)
        return (char *)"0";

    return (char *)0;
}

/*
 * drmIoctl() — route to the bridge so the hidd does the work (BO alloc,
 * CL submit, etc.). The DRM ioctl encoding packs dir/size/type/nr into a
 * u32; we pass just the nr (bits 0-7), which is what the hidd's
 * vc4_aros_ioctl expects (raw DRM and DRM_VC4_* offset by COMMAND_BASE).
 */
int drmIoctl(int fd, unsigned long request, void *arg)
{
    unsigned long nr = request & 0xFF;
    int ret;

    if (!aros_drm_bridge)
    {
        errno = EBADF;
        return -1;
    }

    /* The hidd returns 0, -1 (generic failure) or a NEGATIVE ERRNO for
     * errors Mesa inspects — vc4_bo_wait()/vc4_wait_seqno() treat ETIME
     * as "still busy" and carry on, but abort() the whole process on
     * anything else, so a wait timeout MUST arrive as ETIME here.
     * (errno set inside the hidd never reaches us: each side has its
     * own stdc errno.) */
    ret = aros_drm_bridge->ioctl(aros_drm_bridge->ctx, nr, arg);
    if (ret != 0)
    {
        errno = (ret < -1) ? -ret : EINVAL;
        return -1;
    }
    return 0;
}

/* drmPrime* — unused; fail. */
int drmPrimeFDToHandle(int fd, int prime_fd, unsigned int *handle)
{
    return -1;
}

int drmPrimeHandleToFD(int fd, unsigned int handle, unsigned int flags, int *prime_fd)
{
    return -1;
}

/* Syncobj — has_syncobj=false, so Mesa uses seqno-based sync; fail. */
int drmSyncobjCreate(int fd, unsigned int flags, unsigned int *handle)
{
    return -1;
}

int drmSyncobjDestroy(int fd, unsigned int handle)
{
    return -1;
}

int drmSyncobjImportSyncFile(int fd, unsigned int handle, int sync_file_fd)
{
    return -1;
}

int drmSyncobjExportSyncFile(int fd, unsigned int handle, int *sync_file_fd)
{
    return -1;
}

/*
 * mmap() after MMAP_BO. On AROS the ioctl's "offset" is the BO's
 * CPU-accessible vaddr (CPU == physical), so hand it back as the address.
 */
void *mmap(void *addr, unsigned long length, int prot, int flags, int fd, long offset)
{
    (void)addr; (void)length; (void)prot; (void)flags; (void)fd;
    return (void *)(IPTR)offset;
}

int munmap(void *addr, unsigned long length)
{
    (void)addr; (void)length;
    return 0;
}

/* fcntl() — Mesa dups the fd via F_DUPFD_CLOEXEC; return the same fake fd. */
#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC 1030
#endif

int fcntl(int fd, int cmd, ...)
{
    if (cmd == F_DUPFD_CLOEXEC)
        return fd;
    return -1;
}

#ifndef _SC_PAGE_SIZE
#define _SC_PAGE_SIZE 30
#endif

long sysconf(int name)
{
    if (name == _SC_PAGE_SIZE)
        return 4096;
    return -1;
}

/*
 * clock_gettime() — used for BO cache timeout.
 */
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

int clock_gettime(int clk_id, struct timespec *tp)
{
    if (tp)
    {
        struct DateStamp ds;
        DateStamp(&ds);
        tp->tv_sec = ds.ds_Days * 86400 + ds.ds_Minute * 60 + ds.ds_Tick / 50;
        tp->tv_nsec = (ds.ds_Tick % 50) * 20000000;
    }
    return 0;
}

/* Link anchor for the GalliumCoreAPI table. mesa3dgl references
 * gallium_core_get_api only weakly (glacreatecontext), and a weak
 * reference won't pull gca_table.o out of libgalliumcoreapi.a. This file
 * is always a direct object on raspi, so the strong reference here forces
 * the table (and with it the exported compiler core) to be linked in.
 * The consumer module (vc4gallium.hidd) compiles this file too but has
 * no table — it must not take the reference. */
#ifndef GCA_CONSUMER_MODULE
extern const void *gallium_core_get_api(void);
const void *(*const gallium_core_anchor)(void)
    __attribute__((used)) = gallium_core_get_api;
#endif
