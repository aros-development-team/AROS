/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/libraries.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/thread.h>
#include <assert.h>

struct trampoline_data {
    struct ThreadBase   *ThreadBase;
    struct Task         *parent;
    ThreadEntryFunction entry;
    void                *data;
    uint32_t            id;
};

static void entry_trampoline(void);

/*****************************************************************************

    NAME */
        AROS_LH2(uint32_t, CreateThread,

/*  SYNOPSIS */
        AROS_LHA(ThreadEntryFunction, entry, A0),
        AROS_LHA(void *,              data,  A1),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 5, Thread)

/*  FUNCTION
        Creates a new thread.

    INPUTS
        entry - pointer to a function to run in the new thread
        data  - pointer to pass in the first in the first argument to function
                pointed to by entry

    RESULT
        Numeric thread ID, or 0 if the thread could not be started.

    NOTES

    EXAMPLE
        uint32_t id = CreateThread(entry, data);
        if (id < 0)
            printf("thread creation failed\n");
        else
            printf("thread %d created\n", id);

    BUGS

    SEE ALSO
        CurrentThread(), DetachThread(), WaitThread(), WaitAllThreads()

    INTERNALS
        Each thread gets its own instance of arosc.library, so it can safely
        call functions like printf() without conflicting with other threads.
        Similarly, each thread gets its own standard I/O streams, though they
        start attached to the same place as the task that created the thread.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct trampoline_data *td;
    struct Task *task;
    uint32_t id;

    assert(entry != NULL);

    /* allocate some space for the thread and stuff the trampoline needs */
    if ((td = AllocVec(sizeof(struct trampoline_data), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        return 0;

    td->ThreadBase = ThreadBase;

    /* let the thread find us */
    td->parent = FindTask(NULL);

    /* entry point info for the trampoline */
    td->entry = entry;
    td->data = data;

    /* create the new process and hand control to the trampoline. it will wait
     * for us to finish setting up because we have the thread lock */
    task = (struct Task *) CreateNewProcTags(
        NP_Name,        (IPTR) "thread.library thread",
        NP_Entry,       (IPTR) entry_trampoline,
        NP_UserData,    (IPTR) td,
        NP_Input,       (IPTR) OpenFromLock(DupLockFromFH(Input())),
        NP_Output,      (IPTR) OpenFromLock(DupLockFromFH(Output())),
        NP_Error,       (IPTR) OpenFromLock(DupLockFromFH(Error())),
        TAG_DONE);

    /* failure, shut it down */
    if (task == NULL) {
        FreeMem(td, sizeof(struct trampoline_data));
        return 0;
    }

    /* signal the task to kick it off */
    Signal(task, SIGF_SINGLE);

    /* wait for them to tell us that they're ready */
    Wait(SIGF_SINGLE);

    /* get the new id of the thread that they passed back */
    id = td->id;

    /* free the trampoline data */
    FreeVec(td);

    /* done */
    return id;

    AROS_LIBFUNC_EXIT
} /* CreateThread */

static void entry_trampoline(void) {
    struct Task *task = FindTask(NULL);
    struct trampoline_data *td = task->tc_UserData;
    struct ThreadBase *ThreadBase = td->ThreadBase;
    struct Library *aroscbase;
    struct _Thread *thread;
    void *result;

    /* wait for the parent to let us go */
    Wait(SIGF_SINGLE);

    /* allocate space for the thread */
    if ((thread = AllocVec(sizeof(struct _Thread), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) {

        /* in case of failure, set a negative id and tell the parent so they
         * can inform the caller */
        Signal(td->parent, SIGF_SINGLE);
        return;
    }

    /* give each thread its own C library, so it can reliably printf() etc */
    if ((aroscbase = OpenLibrary("arosc.library", 0)) == NULL) {
        FreeVec(thread);
        Signal(td->parent, SIGF_SINGLE);
        return;
    }

    /* setup the thread */
    InitSemaphore(&thread->lock);
    thread->task = task;

    /* make a condition so other threads can wait for us to exit */
    thread->exit = CreateCondition();
    thread->exit_mutex = CreateMutex();

    ObtainSemaphore(&ThreadBase->lock);

    /* get an id */
    td->id = thread->id = ThreadBase->nextid++;

    /* add the thread to the list */
    ADDTAIL(&ThreadBase->threads, thread);

    ReleaseSemaphore(&ThreadBase->lock);

    /* inform the parent that we're ready to go */
    Signal(td->parent, SIGF_SINGLE);

    /* call the actual thread entry */
    result = AROS_UFC1(void *, td->entry,
                       AROS_UFCA(void *, td->data, A0));

    CloseLibrary(aroscbase);

    /* its over, update its state */
    ObtainSemaphore(&thread->lock);

    /* if its detached, then we close it down right here and now */
    if (thread->detached) {
        /* remove it from the thread list */
        ObtainSemaphore(&ThreadBase->lock);
        REMOVE(thread);
        ReleaseSemaphore(&ThreadBase->lock);

        /* and clean it up */
        DestroyCondition(thread->exit);
        DestroyMutex(thread->exit_mutex);
        FreeVec(td);
        
        return;
    }

    /* save the result */
    thread->result = result;

    /* mark it as done */
    thread->task = NULL;

    ReleaseSemaphore(&thread->lock);

    /* tell anyone that cares. we'll be cleaned up in WaitThread() */
    LockMutex(thread->exit_mutex);
    BroadcastCondition(thread->exit);
    UnlockMutex(thread->exit_mutex);
}
