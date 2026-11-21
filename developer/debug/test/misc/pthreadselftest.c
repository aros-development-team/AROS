/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    pthread_self() in tasks that pthread_create() did not start: every such
    task has to get an identifier of its own, the same one on every call.
    A lock that records its owner as a pthread_t (PhysicsFS does) gives no
    exclusion between two tasks that both read 0.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/tasks.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#define SUBTASKS 3

struct report
{
    struct Task *parent;
    ULONG donemask;
    pthread_t first;
    pthread_t second;
    int equal;
    int join;
    int cancelstate;
};

static struct report reports[SUBTASKS];
static pthread_t workerself;
static int workeroldstate = -1, workercleaned;
static int failed;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok)
        failed = 1;
}

/* A plain DOS process. It stays until the program has read every report,
   so that all the tasks compared exist at the same time. */
static void subtask(void)
{
    struct report *r = (struct report *)FindTask(NULL)->tc_UserData;

    r->first = pthread_self();
    r->second = pthread_self();
    r->equal = pthread_equal(r->first, r->second);
    r->join = pthread_join(r->first, NULL);
    r->cancelstate = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    Signal(r->parent, r->donemask);
    Wait(SIGBREAKF_CTRL_C);

    Forbid();
    Signal(r->parent, r->donemask);
}

static void cleaned(void *arg)
{
    *(int *)arg = 1;
}

/* The calls that work on the calling thread's own record. */
static void *worker(void *arg)
{
    (void)arg;
    workerself = pthread_self();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &workeroldstate);
    pthread_setcancelstate(workeroldstate, NULL);
    pthread_cleanup_push(cleaned, &workercleaned);
    pthread_cleanup_pop(1);
    pthread_exit((void *)0x1234);
    return NULL;
}

int main(void)
{
    struct Process *procs[SUBTASKS];
    pthread_t mainself, thread;
    void *workerret = NULL;
    BYTE sigbit;
    int started = 0, i, j;

    sigbit = AllocSignal(-1);
    if (sigbit == -1)
    {
        printf("FAILED: no signal bit\n");
        return RETURN_FAIL;
    }

    mainself = pthread_self();
    check(pthread_self() == mainself, "the program's task reads the same value twice");

    if (pthread_create(&thread, NULL, worker, NULL) == 0)
    {
        pthread_join(thread, &workerret);
        check(workerself == thread, "a pthread reads the identifier pthread_create() gave");
        check(workerself != mainself, "a pthread differs from the program's task");
        check(workeroldstate == PTHREAD_CANCEL_ENABLE, "a pthread reads its cancel state");
        check(workercleaned, "a pthread runs its cleanup handler");
        check(workerret == (void *)0x1234, "pthread_exit() hands its value to pthread_join()");
    }
    else
        check(0, "pthread_create()");

    for (i = 0; i < SUBTASKS; i++)
    {
        reports[i].parent = FindTask(NULL);
        reports[i].donemask = 1UL << sigbit;
        procs[i] = CreateNewProcTags(NP_Entry, (IPTR)subtask,
                                     NP_Name, (IPTR)"pthreadselftest subtask",
                                     NP_UserData, (IPTR)&reports[i],
                                     TAG_DONE);
        if (!procs[i])
            break;
        Wait(1UL << sigbit);
        started++;
    }
    check(started == SUBTASKS, "subtasks started");

    for (i = 0; i < started; i++)
    {
        check(reports[i].first == reports[i].second && reports[i].equal,
              "a subtask reads the same value twice");
        check(reports[i].first != mainself, "a subtask differs from the program's task");
        check(reports[i].first != thread, "a subtask differs from the pthread");
        check(reports[i].join == ESRCH, "pthread_join() on a subtask's identifier is ESRCH");
        check(reports[i].cancelstate == 0, "pthread_setcancelstate() in a subtask");
        for (j = 0; j < i; j++)
            check(reports[i].first != reports[j].first, "two subtasks differ");
    }

    for (i = 0; i < started; i++)
    {
        Signal(&procs[i]->pr_Task, SIGBREAKF_CTRL_C);
        Wait(1UL << sigbit);
    }
    FreeSignal(sigbit);

    printf("%s\n", failed ? "pthreadselftest: FAILED" : "pthreadselftest: passed");
    return failed ? RETURN_FAIL : RETURN_OK;
}
