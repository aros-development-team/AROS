/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - control mode: files, environment variables, jobs, reboot.

    Everything in this file changes system state and is only reachable
    when the agent was started with CONTROL: without it every command here
    answers ERR 501. The protocol stays line-oriented; PUT and GET carry a
    raw byte body right after their request / status line (see
    host/PROTOCOL.md, "Control mode").

    Jobs: RUN starts a command through a small wrapper process so that the
    agent never blocks on it. AROS' dos.library has no process-death
    notification (NP_NotifyOnDeath is a default tag only), so the wrapper
    runs the command synchronously and records the result in its job slot
    when it returns; the agent polls the slots.
*/

#include "arda.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CTL_CHUNK           65536       /* file transfer chunk */
#define CTL_MAX_PUT         (512UL * 1024 * 1024)
#define CTL_JOB_STACK       (256 * 1024)
#define CTL_WAIT_MAX        600         /* seconds a WAIT may block the agent */

static char *ctlbuf;                    /* CTL_CHUNK bytes, allocated in init */

/* ---- helpers ---------------------------------------------------------- */

static BOOL ctl_enabled(struct Arda *a)
{
    if (a->control)
        return TRUE;
    arda_resp_err(&a->resp, 501, "control mode off (start ARDA with CONTROL)");
    return FALSE;
}

static void ctl_dos_err(struct Arda *a, const char *what)
{
    char fault[128];

    Fault(IoErr(), NULL, fault, sizeof(fault));
    arda_resp_reset(&a->resp);
    arda_resp_line(&a->resp, "ERR 500 %s: %s", what, fault[0] ? fault : "dos error");
}

/* first word of args into word (cap bytes), rest returned trimmed */
static char *ctl_word(char *args, char *word, ULONG cap)
{
    ULONG i = 0;

    while (*args == ' ' || *args == '\t')
        args++;
    while (*args && *args != ' ' && *args != '\t' && i + 1 < cap)
        word[i++] = *args++;
    word[i] = '\0';
    while (*args == ' ' || *args == '\t')
        args++;
    return args;
}

void arda_ctl_init(struct Arda *a)
{
    ULONG i;

    ctlbuf = AllocMem(CTL_CHUNK, MEMF_PUBLIC);
    for (i = 0; i < ARDA_MAX_JOBS; i++)
        a->jobs[i].state = ARDA_JOB_FREE;
}

void arda_ctl_exit(struct Arda *a)
{
    ULONG i;

    /* jobs still running keep their wrapper; the slots are just forgotten */
    for (i = 0; i < ARDA_MAX_JOBS; i++)
        if (a->jobs[i].state == ARDA_JOB_RUNNING)
            a->jobs[i].orphaned = TRUE;
    if (ctlbuf)
    {
        FreeMem(ctlbuf, CTL_CHUNK);
        ctlbuf = NULL;
    }
}

/* ---- PUT: raw body into a file ---------------------------------------- */

void arda_cmd_put(struct Arda *a, char *args)
{
    char path[200], sizes[24];
    char *rest;
    ULONG size;
    BPTR fh;

    if (!ctl_enabled(a))
        return;
    rest = ctl_word(args, path, sizeof(path));
    ctl_word(rest, sizes, sizeof(sizes));
    if (!path[0] || !sizes[0])
    {
        arda_resp_err(&a->resp, 400, "usage: PUT <path> <size>");
        return;
    }
    size = (ULONG)strtoul(sizes, NULL, 10);
    if (size > CTL_MAX_PUT)
    {
        arda_resp_err(&a->resp, 400, "size too large");
        return;
    }
    fh = Open(path, MODE_NEWFILE);
    if (!fh)
    {
        ctl_dos_err(a, "cannot create file");
        return;
    }
    a->client.rawfile = fh;
    a->client.rawleft = size;
    a->client.rawtotal = size;
    a->client.rawfailed = FALSE;
    if (size == 0)
    {
        Close(fh);
        a->client.rawfile = BNULL;
        arda_resp_ok(&a->resp, "WROTE 0");
        return;
    }
    arda_resp_ok(&a->resp, "READY");
}

/* Called by the input loop with the next bytes of a PUT body; returns the
   number consumed. Sends the final reply itself when the body is complete. */
ULONG arda_ctl_raw_input(struct Arda *a, const char *data, ULONG n)
{
    struct ArdaClient *c = &a->client;
    ULONG take = (n < c->rawleft) ? n : c->rawleft;

    if (take && !c->rawfailed)
    {
        if (Write(c->rawfile, (APTR)data, take) != (LONG)take)
            c->rawfailed = TRUE;
    }
    c->rawleft -= take;
    if (c->rawleft == 0)
    {
        BOOL failed = c->rawfailed;

        Close(c->rawfile);
        c->rawfile = BNULL;
        if (failed)
            arda_resp_err(&a->resp, 500, "write failed, file incomplete");
        else
        {
            char text[32];

            snprintf(text, sizeof(text), "WROTE %lu", (unsigned long)c->rawtotal);
            arda_resp_ok(&a->resp, text);
        }
        arda_net_reply(a);
    }
    return take;
}

void arda_ctl_raw_abort(struct Arda *a)
{
    if (a->client.rawfile)
    {
        Close(a->client.rawfile);
        a->client.rawfile = BNULL;
    }
    a->client.rawleft = 0;
}

/* ---- GET: file as a raw body ------------------------------------------ */

void arda_cmd_get(struct Arda *a, char *args)
{
    char path[200];
    char head[48];
    struct FileInfoBlock *fib;
    BPTR fh, lock;
    ULONG size, left;

    if (!ctl_enabled(a))
        return;
    ctl_word(args, path, sizeof(path));
    if (!path[0])
    {
        arda_resp_err(&a->resp, 400, "usage: GET <path>");
        return;
    }
    lock = Lock(path, ACCESS_READ);
    if (!lock)
    {
        arda_resp_err(&a->resp, 404, "no such file");
        return;
    }
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib || !Examine(lock, fib) || fib->fib_DirEntryType > 0)
    {
        if (fib)
            FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        arda_resp_err(&a->resp, 404, "not a file");
        return;
    }
    size = fib->fib_Size;
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    fh = Open(path, MODE_OLDFILE);
    if (!fh)
    {
        ctl_dos_err(a, "cannot open file");
        return;
    }
    snprintf(head, sizeof(head), "OK SIZE %lu\n", (unsigned long)size);
    if (!arda_net_send(a, a->client.fd, head, strlen(head)))
    {
        Close(fh);
        arda_net_close_client(a, "send failed");
        a->replied = TRUE;
        return;
    }
    for (left = size; left && a->client.fd >= 0; )
    {
        ULONG want = (left < CTL_CHUNK) ? left : CTL_CHUNK;
        LONG got = Read(fh, ctlbuf, want);

        if (got <= 0)
        {
            /* the file shrank under us: pad so the client's count still works */
            memset(ctlbuf, 0, want);
            got = want;
        }
        if (!arda_net_send(a, a->client.fd, ctlbuf, got))
        {
            arda_net_close_client(a, "send failed");
            break;
        }
        left -= got;
    }
    Close(fh);
    a->replied = TRUE;
}

/* ---- DIR, DELETE, MKDIR ------------------------------------------------ */

void arda_cmd_dir(struct Arda *a, char *args)
{
    char path[200], name[128], date[32];
    struct FileInfoBlock *fib;
    struct DateTime dt;
    BPTR lock;
    ULONG count = 0;

    if (!ctl_enabled(a))
        return;
    ctl_word(args, path, sizeof(path));
    lock = Lock(path[0] ? path : "", ACCESS_READ);
    if (!lock)
    {
        arda_resp_err(&a->resp, 404, "no such directory");
        return;
    }
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib || !Examine(lock, fib))
    {
        if (fib)
            FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        ctl_dos_err(a, "examine failed");
        return;
    }
    arda_resp_begin(&a->resp);
    if (fib->fib_DirEntryType > 0)
    {
        while (ExNext(lock, fib))
        {
            arda_safe_str(name, sizeof(name), fib->fib_FileName);
            memset(&dt, 0, sizeof(dt));
            dt.dat_Stamp = fib->fib_Date;
            dt.dat_Format = FORMAT_INT;
            dt.dat_StrDate = date;
            dt.dat_StrTime = date + 16;
            date[0] = date[16] = '\0';
            DateToStr(&dt);
            arda_resp_line(&a->resp, "ENTRY NAME=\"%s\" TYPE=%s SIZE=%lu DATE=\"%s %s\"", name,
                           fib->fib_DirEntryType > 0 ? "dir" : "file",
                           (unsigned long)fib->fib_Size, date, date + 16);
            count++;
        }
    }
    else
    {
        arda_safe_str(name, sizeof(name), fib->fib_FileName);
        arda_resp_line(&a->resp, "ENTRY NAME=\"%s\" TYPE=file SIZE=%lu", name, (unsigned long)fib->fib_Size);
        count++;
    }
    arda_resp_line(&a->resp, "COUNT=%lu", (unsigned long)count);
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    arda_resp_end(&a->resp);
}

void arda_cmd_delete(struct Arda *a, char *args)
{
    char path[200];

    if (!ctl_enabled(a))
        return;
    ctl_word(args, path, sizeof(path));
    if (!path[0])
    {
        arda_resp_err(&a->resp, 400, "usage: DELETE <path>");
        return;
    }
    if (!DeleteFile(path))
    {
        ctl_dos_err(a, "delete failed");
        return;
    }
    arda_resp_ok(&a->resp, "DELETED");
}

void arda_cmd_mkdir(struct Arda *a, char *args)
{
    char path[200];
    BPTR lock;

    if (!ctl_enabled(a))
        return;
    ctl_word(args, path, sizeof(path));
    if (!path[0])
    {
        arda_resp_err(&a->resp, 400, "usage: MKDIR <path>");
        return;
    }
    lock = CreateDir(path);
    if (!lock)
    {
        ctl_dos_err(a, "mkdir failed");
        return;
    }
    UnLock(lock);
    arda_resp_ok(&a->resp, "CREATED");
}

/* ---- SETENV / GETENV --------------------------------------------------- */

void arda_cmd_setenv(struct Arda *a, char *args)
{
    char name[64];
    char *value;

    if (!ctl_enabled(a))
        return;
    value = ctl_word(args, name, sizeof(name));
    if (!name[0])
    {
        arda_resp_err(&a->resp, 400, "usage: SETENV <name> [value]");
        return;
    }
    if (!*value)
    {
        DeleteVar(name, GVF_GLOBAL_ONLY | LV_VAR);
        arda_resp_ok(&a->resp, "UNSET");
        return;
    }
    if (!SetVar(name, value, -1, GVF_GLOBAL_ONLY | LV_VAR))
    {
        ctl_dos_err(a, "setvar failed");
        return;
    }
    arda_resp_ok(&a->resp, "SET");
}

void arda_cmd_getenv(struct Arda *a, char *args)
{
    char name[64], value[256];

    if (!ctl_enabled(a))
        return;
    ctl_word(args, name, sizeof(name));
    if (!name[0])
    {
        arda_resp_err(&a->resp, 400, "usage: GETENV <name>");
        return;
    }
    if (GetVar(name, value, sizeof(value), GVF_GLOBAL_ONLY | LV_VAR) < 0)
    {
        arda_resp_err(&a->resp, 404, "no such variable");
        return;
    }
    arda_resp_reset(&a->resp);
    arda_resp_line(&a->resp, "OK %s", value);
}

/* ---- RUN / JOBS / JOB / WAIT ------------------------------------------- */

/* The wrapper process: runs the job's command with its output in the
   job's file and records the result. Everything it needs is in the job. */
static void arda_job_entry(void)
{
    struct ArdaJob *job = (struct ArdaJob *)FindTask(NULL)->tc_UserData;
    BPTR out, in;
    LONG rc;

    out = Open(job->output, MODE_NEWFILE);
    in = Open("NIL:", MODE_OLDFILE);
    if (out)
    {
        rc = SystemTags(job->command,
                        SYS_Input, (IPTR)in,
                        SYS_Output, (IPTR)out,
                        SYS_Asynch, FALSE,
                        SYS_UserShell, TRUE,
                        NP_StackSize, CTL_JOB_STACK,
                        TAG_DONE);
        Close(out);
    }
    else
        rc = -1;
    if (in)
        Close(in);
    job->rc = rc;
    job->finished = TRUE;       /* written last; the agent polls it */
}

void arda_cmd_run(struct Arda *a, char *args)
{
    char output[200];
    char name[32];
    char *cmd;
    struct ArdaJob *job = NULL;
    ULONG i;

    if (!ctl_enabled(a))
        return;
    cmd = ctl_word(args, output, sizeof(output));
    if (!output[0] || !*cmd)
    {
        arda_resp_err(&a->resp, 400, "usage: RUN <output-file> <command line>");
        return;
    }
    for (i = 0; i < ARDA_MAX_JOBS; i++)
    {
        if (a->jobs[i].state == ARDA_JOB_FREE || a->jobs[i].state == ARDA_JOB_DONE)
        {
            job = &a->jobs[i];
            break;
        }
    }
    if (!job)
    {
        arda_resp_err(&a->resp, 503, "no free job slot; query finished jobs first");
        return;
    }
    memset(job, 0, sizeof(*job));
    job->id = ++a->jobseq;
    strncpy(job->output, output, sizeof(job->output) - 1);
    strncpy(job->command, cmd, sizeof(job->command) - 1);
    job->started = a->uptime;
    job->state = ARDA_JOB_RUNNING;
    snprintf(name, sizeof(name), "ARDA job %lu", (unsigned long)job->id);
    job->proc = CreateNewProcTags(NP_Entry, (IPTR)arda_job_entry,
                                  NP_Name, (IPTR)name,
                                  NP_StackSize, 32768,
                                  NP_UserData, (IPTR)job,
                                  NP_Priority, 0,
                                  TAG_DONE);
    if (!job->proc)
    {
        job->state = ARDA_JOB_FREE;
        ctl_dos_err(a, "cannot start the job");
        return;
    }
    arda_log(a, "ARDA: job %lu started: %s", (unsigned long)job->id, job->command);
    arda_resp_reset(&a->resp);
    arda_resp_line(&a->resp, "OK JOB %lu", (unsigned long)job->id);
}

/* Promote finished wrappers' results; called from the main loop */
void arda_ctl_poll(struct Arda *a)
{
    ULONG i;

    for (i = 0; i < ARDA_MAX_JOBS; i++)
    {
        struct ArdaJob *job = &a->jobs[i];

        if (job->state == ARDA_JOB_RUNNING && job->finished)
        {
            job->state = ARDA_JOB_DONE;
            job->ended = a->uptime;
            arda_log(a, "ARDA: job %lu finished, rc %ld", (unsigned long)job->id, (long)job->rc);
        }
    }
}

static void ctl_job_record(struct Arda *a, struct ArdaJob *job)
{
    char cmd[256];

    arda_safe_str(cmd, sizeof(cmd), job->command);
    arda_resp_line(&a->resp, "JOB ID=%lu STATE=%s RC=%ld OUTPUT=\"%s\" STARTED=%lu ELAPSED=%lu COMMAND=\"%s\"",
                   (unsigned long)job->id, job->state == ARDA_JOB_DONE ? "done" : "running", (long)job->rc,
                   job->output, (unsigned long)job->started,
                   (unsigned long)((job->state == ARDA_JOB_DONE ? job->ended : a->uptime) - job->started), cmd);
}

static struct ArdaJob *ctl_find_job(struct Arda *a, ULONG id)
{
    ULONG i;

    for (i = 0; i < ARDA_MAX_JOBS; i++)
        if (a->jobs[i].state != ARDA_JOB_FREE && a->jobs[i].id == id)
            return &a->jobs[i];
    return NULL;
}

void arda_cmd_jobs(struct Arda *a, char *args)
{
    ULONG i, count = 0;

    if (!ctl_enabled(a))
        return;
    arda_ctl_poll(a);
    arda_resp_begin(&a->resp);
    for (i = 0; i < ARDA_MAX_JOBS; i++)
    {
        if (a->jobs[i].state != ARDA_JOB_FREE)
        {
            ctl_job_record(a, &a->jobs[i]);
            count++;
        }
    }
    arda_resp_line(&a->resp, "COUNT=%lu", (unsigned long)count);
    arda_resp_end(&a->resp);
}

void arda_cmd_job(struct Arda *a, char *args)
{
    char ids[16];
    struct ArdaJob *job;

    if (!ctl_enabled(a))
        return;
    ctl_word(args, ids, sizeof(ids));
    arda_ctl_poll(a);
    job = ids[0] ? ctl_find_job(a, (ULONG)strtoul(ids, NULL, 10)) : NULL;
    if (!job)
    {
        arda_resp_err(&a->resp, 404, "no such job");
        return;
    }
    arda_resp_begin(&a->resp);
    ctl_job_record(a, job);
    arda_resp_end(&a->resp);
}

void arda_cmd_wait(struct Arda *a, char *args)
{
    char ids[16], secs[16];
    char *rest;
    struct ArdaJob *job;
    ULONG limit, waited = 0;

    if (!ctl_enabled(a))
        return;
    rest = ctl_word(args, ids, sizeof(ids));
    ctl_word(rest, secs, sizeof(secs));
    limit = secs[0] ? (ULONG)strtoul(secs, NULL, 10) : 60;
    if (limit > CTL_WAIT_MAX)
        limit = CTL_WAIT_MAX;
    arda_ctl_poll(a);
    job = ids[0] ? ctl_find_job(a, (ULONG)strtoul(ids, NULL, 10)) : NULL;
    if (!job)
    {
        arda_resp_err(&a->resp, 404, "no such job");
        return;
    }
    /* the agent blocks here: one client, and it asked for it */
    while (job->state == ARDA_JOB_RUNNING && waited < limit * 5)
    {
        Delay(10);                  /* 0.2 s */
        waited++;
        arda_ctl_poll(a);
    }
    if (job->state != ARDA_JOB_DONE)
    {
        arda_resp_err(&a->resp, 408, "job still running");
        return;
    }
    arda_resp_reset(&a->resp);
    arda_resp_line(&a->resp, "OK RC %ld", (long)job->rc);
}

/* ---- REBOOT ------------------------------------------------------------ */

void arda_cmd_reboot(struct Arda *a, char *args)
{
    if (!ctl_enabled(a))
        return;
    a->reboot = TRUE;               /* the main loop reboots after the reply went out */
    arda_resp_ok(&a->resp, "REBOOTING");
}
