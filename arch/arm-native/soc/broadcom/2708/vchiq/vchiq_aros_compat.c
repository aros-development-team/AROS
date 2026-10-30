/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Semaphores, threads and memory for the VCHIQ core (see vchi_aros.h).
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <stdarg.h>

#include "interface/compat/vchi_aros.h"
#include "vchiq_aros.h"

struct sema_waiter {
    struct MinNode node;
    struct Task *task;
    ULONG sigmask;
};

void _sema_init(struct semaphore *s, int value)
{
    InitSemaphore(&s->lock);
    s->value = value;
    NEWLIST((struct List *)&s->waiters);
}

void _sema_destroy(struct semaphore *s)
{
    (void)s;
}

void down(struct semaphore *s)
{
    struct sema_waiter w;
    BYTE sig;

    ObtainSemaphore(&s->lock);
    if (s->value > 0)
    {
        s->value--;
        ReleaseSemaphore(&s->lock);
        return;
    }

    sig = AllocSignal(-1);
    w.task = FindTask(NULL);
    w.sigmask = (sig != -1) ? (1UL << sig) : SIGF_SINGLE;
    if (sig == -1)
        SetSignal(0, SIGF_SINGLE);
    AddTail((struct List *)&s->waiters, (struct Node *)&w.node);
    ReleaseSemaphore(&s->lock);

    /* up() unlinks us before signalling, under the lock */
    for (;;)
    {
        Wait(w.sigmask);
        ObtainSemaphore(&s->lock);
        if (w.node.mln_Succ == NULL)
            break;
        ReleaseSemaphore(&s->lock);
    }
    ReleaseSemaphore(&s->lock);

    if (sig != -1)
        FreeSignal(sig);
}

int down_interruptible(struct semaphore *s)
{
    down(s);
    return 0;
}

int down_trylock(struct semaphore *s)
{
    int busy = 1;

    ObtainSemaphore(&s->lock);
    if (s->value > 0)
    {
        s->value--;
        busy = 0;
    }
    ReleaseSemaphore(&s->lock);

    return busy;
}

void up(struct semaphore *s)
{
    struct sema_waiter *w;

    ObtainSemaphore(&s->lock);
    w = (struct sema_waiter *)RemHead((struct List *)&s->waiters);
    if (w)
    {
        w->node.mln_Succ = NULL;
        Signal(w->task, w->sigmask);
    }
    else
        s->value++;
    ReleaseSemaphore(&s->lock);
}

struct vchiq_thread_start {
    int (*fn)(void *);
    void *data;
};

static void vchiq_thread_entry(struct vchiq_thread_start *start)
{
    int (*fn)(void *) = start->fn;
    void *data = start->data;

    FreeVec(start);
    fn(data);
}

VCHIQ_THREAD_T vchiq_thread_create(int (*threadfn)(void *data),
                                   void *data, const char namefmt[], ...)
{
    struct vchiq_thread_start *start;
    struct Task *task;
    char *name;
    va_list ap;

    start = AllocVec(sizeof(*start), MEMF_ANY);
    if (!start)
        return NULL;

    /* exec keeps the name pointer, so it lives with the task for good */
    name = AllocVec(VCHIQ_THREAD_NAMELEN, MEMF_ANY);
    if (!name)
    {
        FreeVec(start);
        return NULL;
    }
    va_start(ap, namefmt);
    vsnprintf(name, VCHIQ_THREAD_NAMELEN, namefmt, ap);
    va_end(ap);

    start->fn = threadfn;
    start->data = data;

    task = NewCreateTask(TASKTAG_PC,        vchiq_thread_entry,
                         TASKTAG_NAME,      name,
                         TASKTAG_STACKSIZE, VCHIQ_THREAD_STACK,
                         TASKTAG_PRI,       VCHIQ_THREAD_PRI,
                         TASKTAG_ARG1,      start,
                         TAG_DONE);
    if (!task)
    {
        FreeVec(name);
        FreeVec(start);
    }

    return task;
}

void set_user_nice(VCHIQ_THREAD_T p, int nice)
{
    (void)p;
    (void)nice;
}

void wake_up_process(VCHIQ_THREAD_T p)
{
    (void)p;
}

void *kzalloc(size_t size, int flags)
{
    (void)flags;
    return AllocVec(size, MEMF_ANY | MEMF_CLEAR);
}

void kfree(const void *p)
{
    FreeVec((APTR)p);
}
