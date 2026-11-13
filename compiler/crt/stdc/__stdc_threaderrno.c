/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Per-thread errno.

    errno has to belong to the thread (on AROS, the task) that reads and
    writes it. stdc.library
    gives every opener a base of its own, and a pthread worker opens its own
    (posixc's thread hook), but code reaches errno through whichever base its
    stubs name: in a worker, posixc writes the worker's base while a program
    linked with the plain stubs reads the program's (the main task's) base.
    A thread can also hold several bases (the program's, and the one posixc
    opened for itself). So errno is one location per calling thread, whatever
    base it is reached through: the errno of the first base the thread opened,
    and for a thread with no base of its own an int that lives as long as the
    thread.
*/

#include <proto/exec.h>
#include <proto/task.h>
#include <exec/memory.h>
#include <aros/symbolsets.h>

#include "__stdc_intbase.h"

#include <aros/debug.h>

extern APTR TaskResBase;

/* Per thread: the thread's errno (an int *). 0 when allocating one failed. */
static LONG __stdc_errnoslot;

/* A thread without a base of its own: its errno, freed with the thread */
struct __stdc_threaderrno
{
    struct MemList  ml;
    int             err;
};

/*****i**********************************************************************
    NAME
        int *__stdc_threaderrnoptr(

    SYNOPSIS
        void)

    FUNCTION
        Return the calling thread's errno: that of the first base the
        thread opened, otherwise one kept for the thread until it exits.
        The same location whichever base it is called through.

    INPUTS
        None.

    RESULT
        Pointer to the calling thread's errno.

    NOTES
        errno is *__stdc_geterrnoptr(), which calls this.

    SEE ALSO
        __stdc_geterrnoptr()

*****************************************************************************/
int *__stdc_threaderrnoptr(void)
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct Task *me = FindTask(NULL);
    struct __stdc_threaderrno *te;
    int *err;

    /* A base nobody opened (the root base, library init) or no slot: the
       base's own errno */
    if (!StdCBase->opener_task || __stdc_errnoslot <= 0)
        return &StdCBase->StdCBase._errno;

    err = (int *)GetTaskStorageSlot(__stdc_errnoslot);
    if (err)
        return err;

    /* No base of its own: an errno that exec frees with the thread */
    te = AllocMem(sizeof(*te), MEMF_PUBLIC | MEMF_CLEAR);
    if (!te)
        return &StdCBase->StdCBase._errno;
    te->ml.ml_NumEntries = 1;
    te->ml.ml_ME[0].me_Addr = te;
    te->ml.ml_ME[0].me_Length = sizeof(*te);
    Forbid();
    AddHead(&me->tc_MemEntry, &te->ml.ml_Node);
    Permit();
    SetTaskStorageSlot(__stdc_errnoslot, (IPTR)&te->err);

    return &te->err;
}

static int __init_errnoslot(struct StdCIntBase *StdCBase)
{
    __stdc_errnoslot = AllocTaskStorageSlot();
    D(bug("[stdc] errno task storage slot %ld\n", (long)__stdc_errnoslot));

    return 1;
}

static int __exit_errnoslot(struct StdCIntBase *StdCBase)
{
    if (__stdc_errnoslot > 0)
        FreeTaskStorageSlot(__stdc_errnoslot);
    __stdc_errnoslot = 0;

    return 1;
}

/* A thread's first base holds the errno it uses through every base. */
static int __open_threaderrno(struct StdCIntBase *StdCBase)
{
    StdCBase->opener_task = FindTask(NULL);
    if (__stdc_errnoslot > 0 && !GetTaskStorageSlot(__stdc_errnoslot))
        SetTaskStorageSlot(__stdc_errnoslot, (IPTR)&StdCBase->StdCBase._errno);

    return 1;
}

static void __close_threaderrno(struct StdCIntBase *StdCBase)
{
    if (__stdc_errnoslot > 0 &&
        GetTaskStorageSlot(__stdc_errnoslot) == (IPTR)&StdCBase->StdCBase._errno)
        SetTaskStorageSlot(__stdc_errnoslot, 0);
}

ADD2INITLIB(__init_errnoslot, 0);
ADD2EXPUNGELIB(__exit_errnoslot, 0);
ADD2OPENLIB(__open_threaderrno, 0);
ADD2CLOSELIB(__close_threaderrno, 0);
