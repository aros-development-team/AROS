/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#ifndef THREAD_INTERN_H
#define THREAD_INTERN_H 1

#define DEBUG 1
#include <aros/debug.h>

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <libraries/thread.h>
#include <stdint.h>

/* a single thread */
struct _Thread {
    struct Node             node;       /* node for ThreadBase->threads */

    struct SignalSemaphore  lock;       /* lock for the this thread data */

    uint32_t                id;         /* numerical thread id. read only,
                                         * no need to acquire the lock */

    struct Task             *task;      /* the exec task for this thread, or 
                                           NULL if the thread has completed */

    void                    *result;    /* storage for the thread exit value
                                         * for thread completion waiters */

    struct _Condition       *exit;      /* condition for threads waiting for
                                         * this thread to finish */
    void                    *exit_mutex; /* associated mutex */
    int                     exit_count; /* number of threads waitering */

    BOOL                    detached;   /* flag, thread is detached */
};

/* a condition variable */
struct _Condition {
    struct SignalSemaphore  lock;       /* lock for this condition data */

    struct List             waiters;    /* list of _CondWaiters */
    int                     count;      /* number of waiters in the list */
};

/* a waiter for a condition */
struct _CondWaiter {
    struct Node             node;       /* node for cond->waiters */
    struct Task             *task;      /* task to signal when the condition
                                         * is met */
};

/* the library base. this is a per-opener base */
struct ThreadBase {
    struct Library          library;

    struct ThreadBase       *rootbase;  /* pointer to the global base */

    struct SignalSemaphore  lock;       /* lock for this base */

    uint32_t                nextid;     /* numeric identifier to be issued to
                                         * the next thread created */

    struct List             threads;    /* list of threads */
};

/* helper functions for finding thread data */
static inline struct _Thread *_getthreadbyid(uint32_t id, struct ThreadBase *ThreadBase) {
    struct _Thread *thread, *next;
    ForeachNodeSafe(&ThreadBase->threads, thread, next) {
        if (thread->id == id)
            return thread;
    }
    return NULL;
}

static inline struct _Thread *_getthreadbytask(struct Task *task, struct ThreadBase *ThreadBase) {
    struct _Thread *thread, *next;
    ForeachNodeSafe(&ThreadBase->threads, thread, next) {
        if (thread->task == task)
            return thread;
    }
    return NULL;
}

#endif
