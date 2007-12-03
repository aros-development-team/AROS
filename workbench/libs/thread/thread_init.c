/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/thread.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Open)(struct ThreadBase *ThreadBase) {
    InitSemaphore(&ThreadBase->lock);

    NEWLIST(&ThreadBase->threads);

    ThreadBase->nextid = 1;

    return TRUE;
}

static int GM_UNIQUENAME(Close)(struct ThreadBase *ThreadBase) {
    int nthreads, nattached;
    _Thread thread, next;
    struct Task *task;

    /* we're most likely here because main() exited. if there are remaining
     * threads, we need to do something with them. we have the following
     * options:
     * 
     * 1. leave them alone
     * 2. wait for them to finish
     * 3. kill them
     *
     * [1] is close to impossible. the main task exiting will cause all the
     * resources that DOS has open for the task, including the program code
     * itself, to be deallocated.
     *
     * [2] is fine, but there's no guarantee that they ever will finish;
     * furthermore this Close function is inside Forbid() right now, which
     * means we'd have to re-enable task switches. that's safe because this is
     * a per-opener base, but its just a little bit tricky.
     *
     * [3] ensures that the threads are gone and the main task can exit right
     * now, but AROS really doesn't provide a way to safely kill a process.
     * RemTask() will make sure it never gets scheduled again and will free
     * the memory it allocated, but it may have open libraries, filehandles,
     * etc which will get leaked. This can't be fixed without proper task
     * resource tracking.
     *
     * I've chosen [2] for now, because its really the only option that
     * doesn't cause either a system crash (executing code that no longer
     * exists) or at least instability (leaked files, libraries, etc). They
     * all suck though. The main task should arrange (or wait) for the threads
     * to exit before it exits itself.
     */

    nthreads = nattached = 0;
    ForeachNode(&ThreadBase->threads, thread) {
        nthreads++;
        if (!thread->detached) nattached++;
    }

    if (nthreads > 0) {
        task = FindTask(NULL);

        kprintf("[thread] %d thread%s still running, waiting for %s to finish.\n", nthreads,
                                                                                   nthreads > 1 ? "s" : "",
                                                                                   nthreads > 1 ? "them" : "it");

        if (nattached > 0) {
            kprintf("         %d thread%s still attached at main task exit!\n", nthreads,
                                                                                nthreads > 1 ? "s" : "");
            kprintf("         This probably means a bug in the main task '%s'.\n", task->tc_Node.ln_Name);
            kprintf("         Please report this to the author of that program.\n");
        }

        /* re-enable task switches. we can do this safely because this is a
         * per-opener library base */
        Permit();

        ForeachNodeSafe(&ThreadBase->threads, thread, next) {
            /* re-attach the thread so that WaitThread() can work */
            ObtainSemaphore(&thread->lock);
            thread->detached = FALSE;
            ReleaseSemaphore(&thread->lock);

            WaitThread(thread->id, NULL);
        }

        Forbid();
    }

    return TRUE;
}

ADD2OPENLIB(GM_UNIQUENAME(Open), 0)
ADD2CLOSELIB(GM_UNIQUENAME(Close), 0)
