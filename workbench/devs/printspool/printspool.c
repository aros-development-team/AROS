/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    printspool.device - takes printer output into job files and sends them to
    the printer in the background, so an application's print returns as soon
    as its data is spooled.

    Unit n sends its jobs to the device named by the variable PrintSpool/Unit<n>
    ("parallel.device 0" when it is not set). Jobs go to SPOOL: when that is
    assigned (a partition or drawer, so they outlast a crash), otherwise to
    T:PrintSpool, as <sequence>-u<unit>.tmp while they are written and .job once
    the last opener has closed the unit; a .tmp left by a crash is deleted when
    the spooler first starts, and the .job files are sent again. The spooler
    sends .job files in sequence order, deletes them when done, writes its state
    ("<job>: printing on <device> unit <n>, <sent>/<size> KB", "...: waiting, ...",
    "idle") to PrintSpool/Status, the job's state changes to PrintSpool/Job,
    and drops a job when <job>.cancel exists. The spooler's UI controls the
    rest, not the applications: CMD_STOP and CMD_START on a unit opened with
    PRDOF_SETTINGS pause and resume its queue (the units paused are listed in
    PrintSpool/Paused); <job>.hold keeps a job from being sent, and holds one
    that is being sent between chunks; <job>.stop stops it and holds it, kept;
    <job>.restart sends it again from the start. What
    printer.device says is being printed (PRD_SETJOBINFO) is kept in <job>.info
    as Application= and Document= lines.
*/

#include <aros/debug.h>

#include <dos/dostags.h>
#include <dos/var.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <devices/parallel.h>
#include <devices/timer.h>
#include <devices/serial.h>
#include <devices/printer.h>
#include <devices/newstyle.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "printspool_intern.h"

#define CHUNK_SIZE      65536
#define RETRY_TICKS     (5 * TICKS_PER_SECOND)
#define JOB_SUFFIX      ".job"
#define TMP_SUFFIX      ".tmp"

/* A job whose writer has closed the unit; the spooler finishes it. */
struct FinishedJob
{
    struct MinNode fj_Node;
    BPTR fj_File;
    TEXT fj_Name[64];
    TEXT fj_Application[64];
    TEXT fj_Document[128];
};

static struct MinList finished;

/* SPOOL: when it is assigned, looked up in the DOS list so that a missing one
   brings up no requester. */
static void spool_dir(struct PrintSpoolBase *PrintSpoolBase, STRPTR buf, LONG size)
{
    struct DosList *dl = LockDosList(LDF_ALL | LDF_READ);
    BOOL spool = FindDosEntry(dl, "SPOOL", LDF_ALL) != NULL;

    UnLockDosList(LDF_ALL | LDF_READ);
    strncpy(buf, spool ? "SPOOL:" : "T:PrintSpool", size - 1);
    buf[size - 1] = '\0';
}

static void job_path(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name,
    CONST_STRPTR suffix, STRPTR buf, LONG size)
{
    spool_dir(PrintSpoolBase, buf, size);
    AddPart(buf, name, size);
    if (strlen(buf) + strlen(suffix) < (size_t)size)
        strcat(buf, suffix);
}

/* SetVar() makes the variable's file but not its directory. */
static void set_status(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR text)
{
    BPTR lock = Lock("ENV:PrintSpool", SHARED_LOCK);

    if (lock == BNULL)
        lock = CreateDir("ENV:PrintSpool");
    if (lock != BNULL)
        UnLock(lock);
    if (text != NULL)
        SetVar("PrintSpool/Status", text, -1, GVF_GLOBAL_ONLY);
}

/*
 * PrintSpool/Job: the job being sent, as Key=value lines, rewritten when its
 * state changes (printing, waiting, done, cancelled), for programs that tell
 * the user about printing (PrinterWatch).
 */
static void job_event(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name, LONG unit,
    CONST_STRPTR device, LONG devunit, CONST_STRPTR state, CONST_STRPTR reason,
    CONST_STRPTR app, CONST_STRPTR doc)
{
    TEXT text[512];

    __sprintf(text, "Job=%s\nUnit=%ld\nDevice=%s\nDeviceUnit=%ld\nState=%s\nReason=%s\n"
        "Application=%s\nDocument=%s\n", name, (long)unit, device, (long)devunit, state,
        reason ? reason : (CONST_STRPTR)"", app, doc);
    set_status(PrintSpoolBase, NULL);
    SetVar("PrintSpool/Job", text, -1, GVF_GLOBAL_ONLY);
}

/* Application= and Document= from <job>.info. */
static void job_info(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name,
    STRPTR app, LONG appsize, STRPTR doc, LONG docsize)
{
    TEXT path[300], line[200];
    BPTR fh;

    app[0] = doc[0] = '\0';
    job_path(PrintSpoolBase, name, ".info", path, sizeof(path));
    if ((fh = Open(path, MODE_OLDFILE)) == BNULL)
        return;
    while (FGets(fh, line, sizeof(line)))
    {
        LONG n = strlen(line);

        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';
        if (strncmp(line, "Application=", 12) == 0 && n - 12 < appsize)
            strcpy(app, line + 12);
        else if (strncmp(line, "Document=", 9) == 0 && n - 9 < docsize)
            strcpy(doc, line + 9);
    }
    Close(fh);
}

/* The highest job sequence number in the spool directory. */
static ULONG last_sequence(struct PrintSpoolBase *PrintSpoolBase)
{
    TEXT dir[256];
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR lock;
    ULONG last = 0;

    spool_dir(PrintSpoolBase, dir, sizeof(dir));
    lock = Lock(dir, SHARED_LOCK);
    if (lock == BNULL)
        lock = CreateDir(dir);
    if (lock != BNULL && fib != NULL && Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            ULONG n = 0;
            STRPTR c;

            for (c = fib->fib_FileName; *c >= '0' && *c <= '9'; c++)
                n = n * 10 + (*c - '0');
            if (n > last)
                last = n;
        }
    }
    if (lock != BNULL)
        UnLock(lock);
    FreeDosObject(DOS_FIB, fib);
    return last;
}

static BOOL held(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name);
static BOOL has_flag(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name, CONST_STRPTR suffix);

/* Oldest .job file not held: its name without the suffix goes to name. */
static BOOL next_job(struct PrintSpoolBase *PrintSpoolBase, STRPTR name, LONG size)
{
    TEXT dir[256];
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR lock;
    BOOL found = FALSE;

    spool_dir(PrintSpoolBase, dir, sizeof(dir));
    lock = Lock(dir, SHARED_LOCK);
    if (lock != BNULL && fib != NULL && Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            LONG len = strlen(fib->fib_FileName);

            TEXT candidate[64];

            if (len <= 4 || len - 4 >= size || len - 4 >= (LONG)sizeof(candidate)
                || strcmp(fib->fib_FileName + len - 4, JOB_SUFFIX) != 0)
                continue;
            CopyMem(fib->fib_FileName, candidate, len - 4);
            candidate[len - 4] = '\0';
            /* a held job still goes, to be dropped, once it is cancelled */
            if (held(PrintSpoolBase, candidate) && !has_flag(PrintSpoolBase, candidate, ".cancel"))
                continue;
            /* Names start with a ten-digit sequence, so they sort by age. */
            if (!found || strcmp(candidate, name) < 0)
            {
                strcpy(name, candidate);
                found = TRUE;
            }
        }
    }
    if (lock != BNULL)
        UnLock(lock);
    FreeDosObject(DOS_FIB, fib);
    return found;
}

/* Whether <job><suffix> exists: the spooler UI's control files. */
static BOOL has_flag(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name, CONST_STRPTR suffix)
{
    TEXT path[300];
    BPTR lock;

    job_path(PrintSpoolBase, name, suffix, path, sizeof(path));
    lock = Lock(path, SHARED_LOCK);
    if (lock == BNULL)
        return FALSE;
    UnLock(lock);
    return TRUE;
}

static void set_flag(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name, CONST_STRPTR suffix, BOOL on)
{
    TEXT path[300];
    BPTR file;

    job_path(PrintSpoolBase, name, suffix, path, sizeof(path));
    if (!on)
        DeleteFile(path);
    else if ((file = Open(path, MODE_NEWFILE)) != BNULL)
        Close(file);
}

#define cancelled(base, name) has_flag(base, name, ".cancel")

static LONG job_unit(CONST_STRPTR name)
{
    CONST_STRPTR u = strstr(name, "-u");

    return u != NULL && u[2] >= '0' && u[2] <= '9' ? u[2] - '0' : 0;
}

/* Paused by the spooler UI: its unit's queue, or the job itself. */
static BOOL held(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR name)
{
    return PrintSpoolBase->ps_Paused[job_unit(name)] || has_flag(PrintSpoolBase, name, ".hold");
}

/* "<device> <unit>" from PrintSpool/Unit<n>. */
static void unit_target(struct PrintSpoolBase *PrintSpoolBase, LONG unit, STRPTR device,
    LONG size, LONG *devunit)
{
    TEXT var[24] = "PrintSpool/Unit0";
    TEXT value[128];
    STRPTR space;

    var[15] = '0' + unit;
    strcpy(device, "parallel.device");
    *devunit = 0;
    if (GetVar(var, value, sizeof(value), GVF_GLOBAL_ONLY) <= 0)
        return;
    space = strchr(value, ' ');
    if (space != NULL)
    {
        *space++ = '\0';
        StrToLong(space, devunit);
    }
    if ((LONG)strlen(value) < size)
        strcpy(device, value);
}

/* Writers hand over their closed jobs: close them and rename them to .job. */
static void publish_finished(struct PrintSpoolBase *PrintSpoolBase)
{
    struct FinishedJob *fj;
    TEXT from[300], to[300];

    ObtainSemaphore(&PrintSpoolBase->ps_Lock);
    while ((fj = (struct FinishedJob *)REMHEAD((struct List *)&finished)) != NULL)
    {
        ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
        Close(fj->fj_File);
        if (fj->fj_Application[0] != '\0' || fj->fj_Document[0] != '\0')
        {
            BPTR info;

            job_path(PrintSpoolBase, fj->fj_Name, ".info", to, sizeof(to));
            if ((info = Open(to, MODE_NEWFILE)) != BNULL)
            {
                FPuts(info, "Application=");
                FPuts(info, fj->fj_Application);
                FPuts(info, "\nDocument=");
                FPuts(info, fj->fj_Document);
                FPuts(info, "\n");
                Close(info);
            }
        }
        job_path(PrintSpoolBase, fj->fj_Name, TMP_SUFFIX, from, sizeof(from));
        job_path(PrintSpoolBase, fj->fj_Name, JOB_SUFFIX, to, sizeof(to));
        Rename(from, to);
        FreeMem(fj, sizeof(*fj));
        ObtainSemaphore(&PrintSpoolBase->ps_Lock);
    }
    ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
}

/* Takes the jobs closed by their writers, then sends .job files in order. */
/* Published jobs, and cancelled ones dropped at once rather than when their turn
   comes; active, the one being sent, sees its own cancel. Names are gathered
   before anything is deleted, as ExNext() is not safe across a delete. */
static void housekeep(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR active)
{
    static const char *const suffixes[] = { JOB_SUFFIX, ".info", ".hold", ".stop", ".restart", ".cancel" };
    TEXT dir[256], names[8][64], path[300];
    struct FileInfoBlock *fib;
    LONG count = 0, i, k;
    BPTR lock;

    publish_finished(PrintSpoolBase);
    if ((fib = AllocDosObject(DOS_FIB, NULL)) == NULL)
        return;
    spool_dir(PrintSpoolBase, dir, sizeof(dir));
    if ((lock = Lock(dir, SHARED_LOCK)) != BNULL)
    {
        if (Examine(lock, fib))
            while (count < 8 && ExNext(lock, fib))
            {
                LONG len = strlen(fib->fib_FileName);

                if (len <= 7 || len - 7 >= 64 || strcmp(fib->fib_FileName + len - 7, ".cancel") != 0)
                    continue;
                CopyMem(fib->fib_FileName, names[count], len - 7);
                names[count][len - 7] = '\0';
                if (active == NULL || strcmp(names[count], active) != 0)
                    count++;
            }
        UnLock(lock);
    }
    FreeDosObject(DOS_FIB, fib);
    for (i = 0; i < count; i++)
        for (k = 0; k < (LONG)(sizeof(suffixes) / sizeof(suffixes[0])); k++)
        {
            job_path(PrintSpoolBase, names[i], suffixes[k], path, sizeof(path));
            DeleteFile(path);
        }
}

static void spooler_main(void)
{
    struct PrintSpoolBase *PrintSpoolBase = FindTask(NULL)->tc_UserData;
    UBYTE *chunk = AllocMem(CHUNK_SIZE, MEMF_ANY);
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *io = (struct IOStdReq *)CreateIORequest(port,
        sizeof(struct IOExtPar) > sizeof(struct IOExtSer) ?
        sizeof(struct IOExtPar) : sizeof(struct IOExtSer));
    ULONG sigmask = 1UL << PrintSpoolBase->ps_SpoolerSignal;
    TEXT name[64], path[300], status[200];
    /* Idle, the queue is looked at again now and then: the spooler UI's control
       files (a release, a cancel) come without a signal. */
    struct MsgPort *tport = CreateMsgPort();
    struct timerequest *tr = tport != NULL ? (struct timerequest *)CreateIORequest(tport, sizeof(*tr)) : NULL;

    if (tr != NULL && OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) != 0)
    {
        DeleteIORequest((struct IORequest *)tr);
        tr = NULL;
    }
    set_status(PrintSpoolBase, "idle");
    for (;;)
    {
        BPTR file;
        TEXT device[64], app[64], doc[128], reason[96] = "";
        LONG unit = 0, devunit, actual, sent = 0, total = 0;
        BOOL done = FALSE, started = FALSE, keep = FALSE, paused = FALSE;

        housekeep(PrintSpoolBase, NULL);

        if (chunk == NULL || io == NULL || !next_job(PrintSpoolBase, name, sizeof(name)))
        {
            ULONG tsig = 0, got;

            set_status(PrintSpoolBase, "idle");
            if (tr != NULL)
            {
                tr->tr_node.io_Command = TR_ADDREQUEST;
                tr->tr_time.tv_secs = 2;
                tr->tr_time.tv_micro = 0;
                SendIO((struct IORequest *)tr);
                tsig = 1UL << tport->mp_SigBit;
            }
            got = Wait(sigmask | tsig | SIGBREAKF_CTRL_C);
            if (tr != NULL)
            {
                if (!CheckIO((struct IORequest *)tr))
                    AbortIO((struct IORequest *)tr);
                WaitIO((struct IORequest *)tr);
            }
            if (got & SIGBREAKF_CTRL_C)
                break;
            continue;
        }

        unit = job_unit(name);
        unit_target(PrintSpoolBase, unit, device, sizeof(device), &devunit);
        job_path(PrintSpoolBase, name, JOB_SUFFIX, path, sizeof(path));
        file = Open(path, MODE_OLDFILE);
        if (file == BNULL)
        {
            DeleteFile(path);
            continue;
        }
        job_info(PrintSpoolBase, name, app, sizeof(app), doc, sizeof(doc));

        while (!done)
        {
            if (cancelled(PrintSpoolBase, name))
                break;
            /* held or stopped from the spooler UI before it got going: left as it is */
            if (held(PrintSpoolBase, name) || has_flag(PrintSpoolBase, name, ".stop"))
            {
                keep = TRUE;
                break;
            }
            io->io_Device = NULL;
            if (OpenDevice(device, devunit, (struct IORequest *)io, 0) == 0)
                break;
            __sprintf(status, "%s: waiting, cannot open %s unit %ld", name, device, (long)devunit);
            set_status(PrintSpoolBase, status);
            if (strcmp(reason, "unreachable") != 0)
            {
                strcpy(reason, "unreachable");
                job_event(PrintSpoolBase, name, unit, device, devunit, "waiting", reason, app, doc);
            }
            /* Jobs closed meanwhile become .job now, not when this printer answers:
               a .tmp is taken for half written after a crash. */
            housekeep(PrintSpoolBase, name);
            Delay(RETRY_TICKS);
        }
        if (keep)
        {
            if (has_flag(PrintSpoolBase, name, ".stop"))
            {
                set_flag(PrintSpoolBase, name, ".hold", TRUE);
                set_flag(PrintSpoolBase, name, ".stop", FALSE);
            }
            Close(file);
            continue;
        }
        if (cancelled(PrintSpoolBase, name))
        {
            done = TRUE;
            __sprintf(status, "%s: cancelled", name);
            set_status(PrintSpoolBase, status);
        }
        else
        {
            job_event(PrintSpoolBase, name, unit, device, devunit, "printing", NULL, app, doc);
            started = TRUE;
            reason[0] = '\0';
        }

        {
            struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

            total = 0;
            if (fib != NULL && ExamineFH(file, fib))
                total = fib->fib_Size;
            FreeDosObject(DOS_FIB, fib);
            sent = 0;
        }
        while (!done && (actual = Read(file, chunk, CHUNK_SIZE)) > 0)
        {
            /* The spooler UI: held or paused waits here with the device kept;
               stopped keeps the job and holds it; restarted sends it again. */
            while (held(PrintSpoolBase, name) && !cancelled(PrintSpoolBase, name)
                   && !has_flag(PrintSpoolBase, name, ".stop") && !has_flag(PrintSpoolBase, name, ".restart"))
            {
                if (!paused)
                {
                    paused = TRUE;
                    __sprintf(status, "%s: paused, %ld/%ld KB", name, (long)(sent / 1024), (long)(total / 1024));
                    set_status(PrintSpoolBase, status);
                    job_event(PrintSpoolBase, name, unit, device, devunit, "paused", NULL, app, doc);
                }
                housekeep(PrintSpoolBase, name);
                Delay(TICKS_PER_SECOND);
            }
            if (paused)
            {
                paused = FALSE;
                job_event(PrintSpoolBase, name, unit, device, devunit, "printing", NULL, app, doc);
            }
            if (has_flag(PrintSpoolBase, name, ".stop") || has_flag(PrintSpoolBase, name, ".restart"))
            {
                keep = done = TRUE;
                break;
            }
            /* Jobs closed meanwhile are published while this one prints. */
            housekeep(PrintSpoolBase, name);
            __sprintf(status, "%s: printing on %s unit %ld, %ld/%ld KB", name, device,
                (long)devunit, (long)(sent / 1024), (long)(total / 1024));
            set_status(PrintSpoolBase, status);
            for (;;)
            {
                io->io_Command = CMD_WRITE;
                io->io_Data = chunk;
                io->io_Length = actual;
                io->io_Flags = 0;
                if (DoIO((struct IORequest *)io) == 0)
                {
                    sent += actual;
                    if (reason[0])
                    {
                        /* going again after a wait */
                        reason[0] = '\0';
                        job_event(PrintSpoolBase, name, unit, device, devunit, "printing", NULL, app, doc);
                    }
                    break;
                }
                /* Out of paper, offline, unplugged: wait, then send the chunk again. */
                __sprintf(status, "%s: waiting, %s error %ld", name, device, (long)io->io_Error);
                set_status(PrintSpoolBase, status);
                if (strcmp(reason, "error") != 0)
                {
                    strcpy(reason, "error");
                    job_event(PrintSpoolBase, name, unit, device, devunit, "waiting", reason, app, doc);
                }
                Delay(RETRY_TICKS);
                if (cancelled(PrintSpoolBase, name))
                {
                    done = TRUE;
                    break;
                }

            }
        }
        if (io->io_Device != NULL)
        {
            CloseDevice((struct IORequest *)io);
            io->io_Device = NULL;
        }
        Close(file);
        if (keep)
        {
            /* the printer may have printed some of it; the job is sent whole again */
            BOOL stopped = has_flag(PrintSpoolBase, name, ".stop");

            set_flag(PrintSpoolBase, name, stopped ? ".stop" : ".restart", FALSE);
            if (stopped)
                set_flag(PrintSpoolBase, name, ".hold", TRUE);
            job_event(PrintSpoolBase, name, unit, device, devunit, stopped ? "stopped" : "queued", NULL, app, doc);
            continue;
        }
        /* The device has the whole job (ipp.device has heard back from the printer). */
        job_event(PrintSpoolBase, name, unit, device, devunit,
            done || !started ? "cancelled" : "done", NULL, app, doc);
        DeleteFile(path);
        job_path(PrintSpoolBase, name, ".cancel", path, sizeof(path));
        DeleteFile(path);
        job_path(PrintSpoolBase, name, ".info", path, sizeof(path));
        DeleteFile(path);
        set_flag(PrintSpoolBase, name, ".hold", FALSE);
        D(bug("[printspool] %s done\n", name));
    }

    if (tr != NULL)
    {
        CloseDevice((struct IORequest *)tr);
        DeleteIORequest((struct IORequest *)tr);
    }
    if (tport != NULL)
        DeleteMsgPort(tport);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    if (chunk != NULL)
        FreeMem(chunk, CHUNK_SIZE);
}

/* The spooler process, started on the first job. */
/* Jobs a crash left half written. Only when the spooler first starts, under
   ps_Lock: no unit has a job open yet. */
static void purge_unfinished(struct PrintSpoolBase *PrintSpoolBase)
{
    TEXT dir[256], path[300];
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR lock;

    spool_dir(PrintSpoolBase, dir, sizeof(dir));
    lock = Lock(dir, SHARED_LOCK);
    if (lock != BNULL && fib != NULL && Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            LONG len = strlen(fib->fib_FileName);

            if (len <= 4 || strcmp(fib->fib_FileName + len - 4, TMP_SUFFIX) != 0)
                continue;
            strcpy(path, dir);
            if (AddPart(path, fib->fib_FileName, sizeof(path)))
                DeleteFile(path);
        }
    }
    if (lock != BNULL)
        UnLock(lock);
    FreeDosObject(DOS_FIB, fib);
}

static BOOL start_spooler(struct PrintSpoolBase *PrintSpoolBase)
{
    struct Process *proc;

    if (PrintSpoolBase->ps_Spooler != NULL)
        return TRUE;
    purge_unfinished(PrintSpoolBase);
    proc = CreateNewProcTags(
        NP_Entry, (IPTR)spooler_main,
        NP_Name, (IPTR)"printspool.device",
        NP_UserData, (IPTR)PrintSpoolBase,
        NP_Priority, 0,
        NP_Input, BNULL,
        NP_Output, BNULL,
        NP_CloseInput, FALSE,
        NP_CloseOutput, FALSE,
        TAG_END);
    if (proc == NULL)
        return FALSE;
    PrintSpoolBase->ps_Spooler = proc;
    /* Kept open while the spooler runs, so the device is never expunged under it. */
    PrintSpoolBase->ps_Device.dd_Library.lib_OpenCnt++;
    return TRUE;
}

static LONG write_job(struct PrintSpoolBase *PrintSpoolBase, struct IOStdReq *io)
{
    struct PrintSpoolUnit *pu = (struct PrintSpoolUnit *)io->io_Unit;
    LONG unit = pu - PrintSpoolBase->ps_Unit;
    LONG written;

    ObtainSemaphore(&PrintSpoolBase->ps_Lock);
    if (!start_spooler(PrintSpoolBase))
    {
        ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
        return IOERR_OPENFAIL;
    }
    if (pu->pu_Job == BNULL)
    {
        TEXT path[300];
        ULONG seq;

        if (PrintSpoolBase->ps_Sequence == 0)
            PrintSpoolBase->ps_Sequence = last_sequence(PrintSpoolBase);
        seq = ++PrintSpoolBase->ps_Sequence;
        __sprintf(pu->pu_Name, "%010lu-u%ld", (unsigned long)seq, (long)unit);
        job_path(PrintSpoolBase, pu->pu_Name, TMP_SUFFIX, path, sizeof(path));
        pu->pu_Job = Open(path, MODE_NEWFILE);
        if (pu->pu_Job == BNULL)
        {
            ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
            return IOERR_OPENFAIL;
        }
    }
    ReleaseSemaphore(&PrintSpoolBase->ps_Lock);

    written = Write(pu->pu_Job, io->io_Data, io->io_Length);
    if (written < 0)
    {
        io->io_Actual = 0;
        return PDERR_CANCEL;
    }
    io->io_Actual = written;
    return 0;
}

/* printer.device sends PRD_SETJOBINFO only to an output device that lists it here */
static const UWORD supported[] =
{
    CMD_RESET, CMD_WRITE, CMD_UPDATE, CMD_FLUSH, CMD_STOP, CMD_START, PRD_SETJOBINFO,
    PRD_GETDESTINATION, PRD_SETDESTINATION, NSCMD_DEVICEQUERY, 0
};

/* What PRD_GETDESTINATION and PRD_SETDESTINATION carry for a unit */
static const Tag dest_tags[] = { PDA_Device, PDA_Unit, TAG_DONE };

static BOOL is_settings(struct PrintSpoolBase *PrintSpoolBase, struct Unit *unit)
{
    return (APTR)unit >= (APTR)&PrintSpoolBase->ps_Settings[0]
        && (APTR)unit < (APTR)&PrintSpoolBase->ps_Settings[PRINTSPOOL_UNITS];
}

static LONG unit_number(struct PrintSpoolBase *PrintSpoolBase, struct Unit *unit)
{
    if (is_settings(PrintSpoolBase, unit))
        return ((struct PrintSpoolSettingsUnit *)unit)->pss_Number;
    return (struct PrintSpoolUnit *)unit - PrintSpoolBase->ps_Unit;
}

/* PrintSpool/Unit<n> in dir (ENV: or ENVARC:), as written by SetVar(). */
static BOOL write_target(struct PrintSpoolBase *PrintSpoolBase, CONST_STRPTR dir, LONG unit,
    CONST_STRPTR text)
{
    TEXT path[40];
    BPTR fh, lock;
    LONG len = strlen(text);

    strcpy(path, dir);
    strcat(path, "PrintSpool");
    if ((lock = Lock(path, SHARED_LOCK)) != BNULL || (lock = CreateDir(path)) != BNULL)
        UnLock(lock);
    strcat(path, "/Unit0");
    path[strlen(path) - 1] = '0' + unit;
    if ((fh = Open(path, MODE_NEWFILE)) == BNULL)
        return FALSE;
    if (Write(fh, (APTR)text, len) != len)
        len = -1;
    Close(fh);
    return len >= 0;
}

/* Whether name (len characters) is printspool.device, in any case. */
static BOOL is_self(CONST_STRPTR name, LONG len)
{
    static const char self[] = "printspool.device";
    LONG i;

    if (len != sizeof(self) - 1)
        return FALSE;
    for (i = 0; i < len; i++)
        if ((name[i] | 0x20) != self[i] && name[i] != self[i])
            return FALSE;
    return TRUE;
}

static LONG get_destination(struct PrintSpoolBase *PrintSpoolBase, struct IOStdReq *io)
{
    struct TagItem *tags = io->io_Data, *tag;
    TEXT device[PDA_VALUESIZE];
    LONG devunit;

    if (tags == NULL)
        return IOERR_BADADDRESS;
    unit_target(PrintSpoolBase, unit_number(PrintSpoolBase, io->io_Unit), device, sizeof(device), &devunit);
    while ((tag = LibNextTagItem(&tags)) != NULL)
    {
        if (tag->ti_Data == 0)
            continue;
        switch (tag->ti_Tag)
        {
        case PDA_Tags:
            *(const Tag **)tag->ti_Data = dest_tags;
            break;
        case PDA_Device:
            strcpy((STRPTR)tag->ti_Data, device);
            break;
        case PDA_Unit:
            *(LONG *)tag->ti_Data = devunit;
            break;
        }
    }
    return 0;
}

static LONG set_destination(struct PrintSpoolBase *PrintSpoolBase, struct IOStdReq *io)
{
    struct TagItem *tags = io->io_Data, *tag;
    LONG unit = unit_number(PrintSpoolBase, io->io_Unit), devunit;
    TEXT device[PDA_VALUESIZE], value[PDA_VALUESIZE + 16];
    BOOL save = FALSE, changed = FALSE;

    if (tags == NULL)
        return IOERR_BADADDRESS;
    unit_target(PrintSpoolBase, unit, device, sizeof(device), &devunit);
    while ((tag = LibNextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case PDA_Device:
            if (tag->ti_Data == 0 || strlen((STRPTR)tag->ti_Data) >= sizeof(device)
                || strchr((STRPTR)tag->ti_Data, ' ') != NULL)
                return IOERR_BADADDRESS;
            strcpy(device, (STRPTR)tag->ti_Data);
            changed = TRUE;
            break;
        case PDA_Unit:
            devunit = (LONG)tag->ti_Data;
            changed = TRUE;
            break;
        case PDA_Save:
            save = tag->ti_Data != 0;
            break;
        }
    }
    if (!changed)
        return 0;
    /* never back into itself */
    if (device[0] == '\0' || devunit < 0 || is_self(device, strlen(device)))
        return IOERR_BADADDRESS;
    __sprintf(value, "%s %ld", device, (long)devunit);
    if (!write_target(PrintSpoolBase, "ENV:", unit, value)
        || (save && !write_target(PrintSpoolBase, "ENVARC:", unit, value)))
        return IOERR_OPENFAIL;
    return 0;
}

/* PrintSpool/Paused lists the units the spooler UI has paused; resuming wakes the spooler. */
static void pause_queue(struct PrintSpoolBase *PrintSpoolBase, LONG unit, BOOL pause)
{
    TEXT list[3 * PRINTSPOOL_UNITS + 1];
    LONG i, n = 0;

    ObtainSemaphore(&PrintSpoolBase->ps_Lock);
    PrintSpoolBase->ps_Paused[unit] = pause;
    for (i = 0; i < PRINTSPOOL_UNITS; i++)
        if (PrintSpoolBase->ps_Paused[i])
        {
            if (n)
                list[n++] = ' ';
            list[n++] = '0' + i;
        }
    list[n] = '\0';
    ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
    set_status(PrintSpoolBase, NULL);
    SetVar("PrintSpool/Paused", list, -1, GVF_GLOBAL_ONLY);
    if (!pause && PrintSpoolBase->ps_Spooler != NULL)
        Signal((struct Task *)PrintSpoolBase->ps_Spooler, 1UL << PrintSpoolBase->ps_SpoolerSignal);
}

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IOStdReq *, io, A1),
    struct PrintSpoolBase *, PrintSpoolBase, 5, PrintSpool)
{
    AROS_LIBFUNC_INIT

    BOOL settings = is_settings(PrintSpoolBase, io->io_Unit);

    switch (settings && io->io_Command != PRD_GETDESTINATION && io->io_Command != PRD_SETDESTINATION
            && io->io_Command != NSCMD_DEVICEQUERY && io->io_Command != CMD_STOP && io->io_Command != CMD_START
            ? CMD_INVALID : io->io_Command)
    {
    case CMD_STOP:
    case CMD_START:
        /* The queue is the spooler UI's to pause, not the applications' */
        if (!settings)
        {
            io->io_Error = IOERR_NOCMD;
            break;
        }
        pause_queue(PrintSpoolBase, unit_number(PrintSpoolBase, io->io_Unit), io->io_Command == CMD_STOP);
        io->io_Error = 0;
        break;
    case PRD_GETDESTINATION:
        io->io_Error = get_destination(PrintSpoolBase, io);
        break;
    case PRD_SETDESTINATION:
        io->io_Error = set_destination(PrintSpoolBase, io);
        break;
    case NSCMD_DEVICEQUERY:
    {
        struct NSDeviceQueryResult *nsd = io->io_Data;

        if (nsd == NULL || io->io_Length < sizeof(*nsd))
        {
            io->io_Error = IOERR_BADLENGTH;
            break;
        }
        nsd->DevQueryFormat = 0;
        nsd->SizeAvailable = sizeof(*nsd);
        nsd->DeviceType = NSDEVTYPE_PRINTER;
        nsd->DeviceSubType = 0;
        nsd->SupportedCommands = (UWORD *)supported;
        io->io_Actual = sizeof(*nsd);
        io->io_Error = 0;
        break;
    }
    case CMD_WRITE:
        io->io_Error = write_job(PrintSpoolBase, io);
        break;
    case PRD_SETJOBINFO:
    {
        struct PrintSpoolUnit *pu = (struct PrintSpoolUnit *)io->io_Unit;
        struct TagItem *tags = io->io_Data, *tag;

        ObtainSemaphore(&PrintSpoolBase->ps_Lock);
        while ((tag = LibNextTagItem(&tags)) != NULL)
        {
            STRPTR dest = NULL;
            LONG size = 0;

            if (tag->ti_Tag == PJIA_Application)
            {
                dest = pu->pu_Application;
                size = sizeof(pu->pu_Application);
            }
            else if (tag->ti_Tag == PJIA_Document)
            {
                dest = pu->pu_Document;
                size = sizeof(pu->pu_Document);
            }
            if (dest != NULL)
            {
                strncpy(dest, tag->ti_Data ? (const char *)tag->ti_Data : "", size - 1);
                dest[size - 1] = '\0';
            }
        }
        ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
        io->io_Error = 0;
        break;
    }
    case CMD_FLUSH:
    case CMD_UPDATE:
    case CMD_RESET:
        io->io_Error = 0;
        break;
    default:
        io->io_Error = IOERR_NOCMD;
        break;
    }
    if (!(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct PrintSpoolBase *, PrintSpoolBase, 6, PrintSpool)
{
    AROS_LIBFUNC_INIT
    (void)io;
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(struct PrintSpoolBase *PrintSpoolBase)
{
    InitSemaphore(&PrintSpoolBase->ps_Lock);
    NEWLIST((struct List *)&finished);
    PrintSpoolBase->ps_SpoolerSignal = SIGBREAKB_CTRL_F;
    PrintSpoolBase->ps_DOSBase = OpenLibrary("dos.library", 36);
    return PrintSpoolBase->ps_DOSBase != NULL;
}

static int GM_UNIQUENAME(Expunge)(struct PrintSpoolBase *PrintSpoolBase)
{
    CloseLibrary(PrintSpoolBase->ps_DOSBase);
    return TRUE;
}

static int GM_UNIQUENAME(Open)(struct PrintSpoolBase *PrintSpoolBase, struct IOStdReq *io,
    ULONG unitnum, ULONG flags)
{
    if (unitnum >= PRINTSPOOL_UNITS)
        return FALSE;
    if (flags & PRDOF_SETTINGS)
    {
        PrintSpoolBase->ps_Settings[unitnum].pss_Number = unitnum;
        io->io_Unit = (struct Unit *)&PrintSpoolBase->ps_Settings[unitnum];
        return TRUE;
    }
    ObtainSemaphore(&PrintSpoolBase->ps_Lock);
    PrintSpoolBase->ps_Unit[unitnum].pu_Openers++;
    ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
    io->io_Unit = (struct Unit *)&PrintSpoolBase->ps_Unit[unitnum];
    return TRUE;
}

/* The last opener closing ends the job; the spooler closes and publishes it,
   since DOS calls do not belong here. */
static int GM_UNIQUENAME(Close)(struct PrintSpoolBase *PrintSpoolBase, struct IOStdReq *io)
{
    struct PrintSpoolUnit *pu = (struct PrintSpoolUnit *)io->io_Unit;
    struct FinishedJob *fj = NULL;

    if (is_settings(PrintSpoolBase, io->io_Unit))
    {
        io->io_Unit = NULL;
        return TRUE;
    }
    io->io_Unit = NULL;
    ObtainSemaphore(&PrintSpoolBase->ps_Lock);
    if (pu->pu_Openers > 0)
        pu->pu_Openers--;
    if (pu->pu_Openers == 0 && pu->pu_Job == BNULL)
        pu->pu_Application[0] = pu->pu_Document[0] = '\0';
    if (pu->pu_Openers == 0 && pu->pu_Job != BNULL)
    {
        fj = AllocMem(sizeof(*fj), MEMF_ANY);
        /* Without memory for the hand-over the job stays open and the next
           writer appends to it. */
        if (fj != NULL)
        {
            fj->fj_File = pu->pu_Job;
            strcpy(fj->fj_Name, pu->pu_Name);
            strcpy(fj->fj_Application, pu->pu_Application);
            strcpy(fj->fj_Document, pu->pu_Document);
            ADDTAIL((struct List *)&finished, (struct Node *)fj);
            pu->pu_Job = BNULL;
            pu->pu_Application[0] = pu->pu_Document[0] = '\0';
        }
    }
    ReleaseSemaphore(&PrintSpoolBase->ps_Lock);
    if (fj != NULL && PrintSpoolBase->ps_Spooler != NULL)
        Signal((struct Task *)PrintSpoolBase->ps_Spooler, 1UL << PrintSpoolBase->ps_SpoolerSignal);
    return TRUE;
}

ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)
ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
