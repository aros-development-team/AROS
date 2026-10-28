/*
    Copyright (C) 2008-2026, The AROS Development Team. All rights reserved.

    4.4BSD function flock().
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <aros/symbolsets.h>

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>

#include "__fdesc.h"
#include "__posixc_intbase.h"

/* One registry per library, shared by all its bases (see __init_flocks()).
   fr_Lock guards the registry, every __flock_file and the flock_* fields
   of every fcb. */
struct __flock_registry
{
    struct SignalSemaphore fr_Lock;
    struct MinList fr_Files;
};

/* A file with at least one lock or one waiting request. */
struct __flock_file
{
    struct Node ff_Node;            /* ln_Name: full path of the file */
    struct MinList ff_Holders;      /* fcbs holding a lock (fcb->flock_node) */
    struct MinList ff_Waiters;      /* struct __flock_waiter */
    struct __flock_registry *ff_Registry;
};

/* A blocked request, on the waiting task's stack. */
struct __flock_waiter
{
    struct MinNode fw_Node;
    struct Task *fw_Task;
    ULONG fw_SigMask;
};

#define HOLDER(n) ((fcb *)((UBYTE *)(n) - offsetof(fcb, flock_node)))

static BOOL __flock_conflict(struct __flock_file *ff, fcb *f, int mode, BOOL *others);
static void __flock_wake(struct __flock_file *ff);
static void __flock_free_unused(struct __flock_file *ff);
static char *__flock_name(fcb *f);
static void __flock_drop(fcb *f);

/*****************************************************************************

    NAME */
#include <sys/file.h>

        int flock (

/*  SYNOPSIS */
        int fd,
        int operation)

/*  FUNCTION
        Apply or remove an advisory lock on open file descriptor fd. Operation
        argument can be one of the following constants:
        
        LOCK_SH - Place a shared lock on the file specified by fd. More that
                  one process can hold a shared lock on a given file at a
                  time.
        
        LOCK_EX - Place an exclusive lock on the file specified by fd. Only
                  one process can hold an exclusive lock on a given file at
                  a time.
        
        LOCK_UN - Remove an existing lock from the file specified by fd.

        LOCK_EX operation blocks if another description holds a lock on the
        file. LOCK_SH blocks if another description holds an exclusive lock
        on the file. If you want to do a non-blocking request, OR the
        operation specifier with LOCK_NB constant. In this case flock() will
        return -1 instead of blocking and set errno to EWOULDBLOCK.
        
        A lock belongs to the open file description: descriptors made with
        dup(), dup2() or inherited from vfork() share it, and any of them
        can convert or remove it. A separate open() of the same file, in the
        same process or another one, is a separate description whose locks
        conflict with it. The lock is released when the last descriptor of
        its description is closed, which includes the program's exit.

        Calling flock() on a description that already holds a lock converts
        it. A conversion from shared to exclusive that has to wait first
        releases the shared lock; with LOCK_NB it fails and the shared lock
        is kept.

    INPUTS
        fd - File descriptor of the file you want to place or remove lock from.
        operation - Lock operation to be performed.

    RESULT
        0 on success, -1 on error. In case of error a global errno variable
        is set:

        EBADF       - fd is not an open file descriptor.
        EINVAL      - operation is not valid.
        EWOULDBLOCK - LOCK_NB was given and the lock is held in a
                      conflicting mode.
        EDEADLK     - the request would block and every conflicting lock was
                      obtained by the calling task through another
                      description, so waiting could not end.
        ENOLCK      - no memory or no free signal bit for the request.

    NOTES
        Locks placed with flock() are only advisory, they place no
        restrictions to any file or file descriptor operations.

    EXAMPLE

    BUGS
        A blocked request cannot be interrupted. A file is identified by the
        name its handler returns for the open handle (NameFromFH()). Locks
        are kept per instance of posixc.library.

    SEE ALSO

    INTERNALS
        exec.library semaphores cannot be used: they belong to a task, nest
        for their owner and can only be released by it, while a flock()
        lock belongs to an open file description. The library keeps a
        registry of locked files, each with the descriptions (fcb) holding
        a lock and the requests waiting for one. close() releases the lock
        when the last descriptor of a description goes, and the descriptors
        of a program are closed when it exits.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct __flock_registry *reg = PosixCBase->flock_registry;
    fdesc *desc = __getfdesc(fd);
    int mode = operation & ~LOCK_NB;
    struct Task *me = FindTask(NULL);
    struct __flock_waiter waiter;
    struct __flock_file *ff;
    BYTE sigbit = -1;
    char *name = NULL;
    int error = 0;
    fcb *f;

    D(bug("flock(%d, %d)\n", fd, operation));
    if (!desc)
    {
        errno = EBADF;
        return -1;
    }

    if (mode != LOCK_SH && mode != LOCK_EX && mode != LOCK_UN)
    {
        errno = EINVAL;
        return -1;
    }

    f = desc->fcb;

    if (mode == LOCK_UN)
    {
        ObtainSemaphore(&reg->fr_Lock);
        __flock_drop(f);
        ReleaseSemaphore(&reg->fr_Lock);
        return 0;
    }

    /* Looked up before taking the registry lock: the handler is not called
       with it held. */
    name = __flock_name(f);
    if (!name)
        return -1;

    ObtainSemaphore(&reg->fr_Lock);

    ff = f->flock_file;
    if (ff)
    {
        if (f->flock_mode == mode)
            goto done;

        if (mode == LOCK_SH)
        {
            /* Downgrade: nothing can conflict */
            f->flock_mode = LOCK_SH;
            __flock_wake(ff);
            goto done;
        }

        /* Upgrade */
        if (!__flock_conflict(ff, f, LOCK_EX, NULL))
        {
            f->flock_mode = LOCK_EX;
            f->flock_task = me;
            goto done;
        }
        if (operation & LOCK_NB)
        {
            error = EWOULDBLOCK;
            goto done;
        }
        /* Drop the shared lock before waiting, as BSD does, so that two
           descriptions upgrading at the same time do not wait on each
           other. ff stays: it is used right below. */
        Remove((struct Node *)&f->flock_node);
        f->flock_file = NULL;
        __flock_wake(ff);
    }
    else
    {
        ff = (struct __flock_file *)FindName((struct List *)&reg->fr_Files, name);
        if (!ff)
        {
            ff = AllocVec(sizeof(struct __flock_file), MEMF_ANY | MEMF_CLEAR);
            if (!ff)
            {
                error = ENOLCK;
                goto done;
            }
            ff->ff_Node.ln_Name = name;
            name = NULL;
            NEWLIST(&ff->ff_Holders);
            NEWLIST(&ff->ff_Waiters);
            ff->ff_Registry = reg;
            AddTail((struct List *)&reg->fr_Files, &ff->ff_Node);
        }
    }

    for (;;)
    {
        BOOL others = FALSE;

        if (!__flock_conflict(ff, f, mode, &others))
        {
            AddTail((struct List *)&ff->ff_Holders, (struct Node *)&f->flock_node);
            f->flock_file = ff;
            f->flock_mode = mode;
            f->flock_task = me;
            break;
        }
        if (operation & LOCK_NB)
        {
            error = EWOULDBLOCK;
            break;
        }
        /* Only this task could release the conflicting locks, and it is
           about to wait: a single-threaded program would hang for good. */
        if (!others)
        {
            error = EDEADLK;
            break;
        }

        /* A private signal: SIGF_SINGLE belongs to exec's semaphores, and a
           late wakeup must not reach a task waiting in ObtainSemaphore(). */
        if (sigbit == -1)
        {
            sigbit = AllocSignal(-1);
            if (sigbit == -1)
            {
                error = ENOLCK;
                break;
            }
            waiter.fw_Task = me;
            waiter.fw_SigMask = 1UL << sigbit;
        }
        SetSignal(0, waiter.fw_SigMask);
        AddTail((struct List *)&ff->ff_Waiters, (struct Node *)&waiter.fw_Node);

        D(bug("[flock] Waiting for %s\n", ff->ff_Node.ln_Name));
        ReleaseSemaphore(&reg->fr_Lock);
        Wait(waiter.fw_SigMask);
        ObtainSemaphore(&reg->fr_Lock);
        Remove((struct Node *)&waiter.fw_Node);
    }

    if (error)
        __flock_free_unused(ff);

done:
    ReleaseSemaphore(&reg->fr_Lock);

    if (sigbit != -1)
    {
        /* No one signals it any more: the request left the list above */
        SetSignal(0, waiter.fw_SigMask);
        FreeSignal(sigbit);
    }
    FreeVec(name);

    if (error)
    {
        D(bug("[flock] Failed with %d\n", error));
        errno = error;
        return -1;
    }
    return 0;
}

/* Is a lock in mode on ff blocked by a description other than f? *others is
   set when a conflicting lock was obtained by another task. */
static BOOL __flock_conflict(struct __flock_file *ff, fcb *f, int mode, BOOL *others)
{
    struct Task *me = FindTask(NULL);
    struct MinNode *n;
    BOOL conflict = FALSE;

    ForeachNode(&ff->ff_Holders, n)
    {
        fcb *h = HOLDER(n);

        if (h == f)
            continue;
        if (mode == LOCK_EX || h->flock_mode == LOCK_EX)
        {
            conflict = TRUE;
            if (h->flock_task != me && others)
                *others = TRUE;
        }
    }
    return conflict;
}

/* Wake every waiting request on ff; each one checks again. A woken request
   stays in the list until it removes itself, which keeps ff from being freed
   while it still refers to it. Registry lock held. */
static void __flock_wake(struct __flock_file *ff)
{
    struct __flock_waiter *w;

    ForeachNode(&ff->ff_Waiters, w)
        Signal(w->fw_Task, w->fw_SigMask);
}

/* Registry lock held. */
static void __flock_free_unused(struct __flock_file *ff)
{
    if (IsListEmpty((struct List *)&ff->ff_Holders) &&
        IsListEmpty((struct List *)&ff->ff_Waiters))
    {
        D(bug("[flock] No locks left on %s\n", ff->ff_Node.ln_Name));
        Remove(&ff->ff_Node);
        FreeVec(ff->ff_Node.ln_Name);
        FreeVec(ff);
    }
}

/* Full path of the file or directory of f, or NULL with errno set. */
static char *__flock_name(fcb *f)
{
    ULONG size = 256;
    char *buffer;
    BOOL ok;

    for (;;)
    {
        buffer = AllocVec(size, MEMF_ANY);
        if (!buffer)
        {
            errno = ENOLCK;
            return NULL;
        }

        __fcb_lock(f);
        if (f->privflags & _FCB_ISDIR)
            ok = NameFromLock(f->handle, (STRPTR)buffer, size);
        else
            ok = NameFromFH(f->handle, (STRPTR)buffer, size);
        __fcb_unlock(f);
        if (ok)
            return buffer;

        FreeVec(buffer);
        if (IoErr() != ERROR_LINE_TOO_LONG)
        {
            errno = __stdc_ioerr2errno(IoErr());
            return NULL;
        }
        size *= 2;
    }
}

/* Remove the lock of description f, if it holds one. Registry lock held. */
static void __flock_drop(fcb *f)
{
    struct __flock_file *ff = f->flock_file;

    if (ff)
    {
        D(bug("[flock] Releasing %s\n", ff->ff_Node.ln_Name));
        Remove((struct Node *)&f->flock_node);
        f->flock_file = NULL;
        f->flock_task = NULL;
        f->flock_mode = 0;
        __flock_wake(ff);
        __flock_free_unused(ff);
    }
}

/* Called by close() and closedir() when the last descriptor of f goes. No
   other descriptor of f is left, so no flock() can change f meanwhile and
   flock_file can be read before the lock is taken; the registry is the one
   that holds the lock, which matters only when a descriptor table crossed
   library instances (vfork). */
void __flock_release(fcb *f)
{
    struct __flock_registry *reg;

    if (!f->flock_file)
        return;

    reg = f->flock_file->ff_Registry;
    ObtainSemaphore(&reg->fr_Lock);
    __flock_drop(f);
    ReleaseSemaphore(&reg->fr_Lock);
}

/* __init_flocks is called during library init. The registry pointer is
   copied into every base opened from it, so all programs using this
   library share one registry: locks are between processes. */
int __init_flocks(struct PosixCIntBase *PosixCBase)
{
    struct __flock_registry *reg;

    reg = AllocMem(sizeof(struct __flock_registry), MEMF_PUBLIC | MEMF_CLEAR);
    if (!reg)
        return 0;
    InitSemaphore(&reg->fr_Lock);
    NEWLIST(&reg->fr_Files);
    PosixCBase->flock_registry = reg;

    D(bug("[flock] Initialized lock registry at 0x%p\n", reg));

    return 1;
}

/* Called once before the root libbase is freed, when no program has the
   library open: every descriptor, and so every lock, is gone. */
int __unlock_flocks(struct PosixCIntBase *PosixCBase)
{
    struct __flock_registry *reg = PosixCBase->flock_registry;
    struct __flock_file *ff;

    if (!reg)
        return TRUE;

    while ((ff = (struct __flock_file *)REMHEAD(&reg->fr_Files)))
    {
        bug("[flock] Lock registry not empty at expunge: %s\n", ff->ff_Node.ln_Name);
        FreeVec(ff->ff_Node.ln_Name);
        FreeVec(ff);
    }
    FreeMem(reg, sizeof(struct __flock_registry));
    PosixCBase->flock_registry = NULL;
    return TRUE;
}

ADD2INITLIB(__init_flocks, 1);
ADD2EXPUNGELIB(__unlock_flocks, 1);
