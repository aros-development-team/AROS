/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/semaphores.h>
#include <exec/lists.h>

#include <stdio.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

#if defined(__AROS__)
#include <aros/config.h>
#else
#define _pthread_free_affinity(mask)
#endif

/*
 * Loosely-coupled C-runtime per-thread hook registry.  Kept in sync with the
 * identical declarations in compiler/crt/posixc/__threadhook.c and found by
 * name in the public semaphore list.
 *
 * A C runtime that uses per-task library bases (e.g. posixc.library, which is
 * "pertaskbase") reads the calling Task's own base out of per-task storage and
 * jumps through it directly.  A worker we create never runs the program's C
 * startup, so its slot is NULL and the first CRT call would bus-fault.  The
 * CRT registers enter/leave hooks here; we run them in every worker so it
 * acquires a valid CRT base of its own.  pthread stays C-runtime agnostic - it
 * neither knows nor names any CRT.
 */
#define CRT_THREADHOOKS_SEMNAME "crt.thread-hooks.v1"
#define CRT_MAX_THREADHOOKS     8

struct CRTThreadHook
{
    struct MinNode  th_Node;
    void          (*th_Enter)(void);
    void          (*th_Leave)(void);
};

struct CRTThreadHooks
{
    struct SignalSemaphore th_Sem;
    struct MinList         th_Hooks;
};

static void RunCRTThreadHooks(BOOL enter)
{
    struct CRTThreadHooks *reg;
    struct CRTThreadHook *h;
    void (*fns[CRT_MAX_THREADHOOKS])(void);
    int n = 0, i;

    reg = (struct CRTThreadHooks *)FindSemaphore((STRPTR)CRT_THREADHOOKS_SEMNAME);
    if (!reg)
        return;

    /* Snapshot the hook pointers under the lock, then call them unlocked: an
       enter hook opens a library and must not run under our semaphore. */
    ObtainSemaphoreShared(&reg->th_Sem);
    for (h = (struct CRTThreadHook *)reg->th_Hooks.mlh_Head;
         h->th_Node.mln_Succ != NULL && n < CRT_MAX_THREADHOOKS;
         h = (struct CRTThreadHook *)h->th_Node.mln_Succ)
    {
        void (*fn)(void) = enter ? h->th_Enter : h->th_Leave;
        if (fn)
            fns[n++] = fn;
    }
    ReleaseSemaphore(&reg->th_Sem);

    /* Leave hooks run in the reverse order of the enter hooks. */
    if (enter)
        for (i = 0; i < n; i++) fns[i]();
    else
        for (i = n - 1; i >= 0; i--) fns[i]();
}

static void StarterFunc(void)
{
    ThreadInfo *inf;
    struct Task *waiter;
    int i, j;
    int foundkey = TRUE;
#ifdef USE_ASYNC_CANCEL
    APTR oldexcept;
#endif

    DB2(bug("%s()\n", __FUNCTION__));

#if defined(__AMIGA__) && !defined(__MORPHOS__)
    struct Process * proc = (struct Process *)SysBase->ThisTask;
    inf = (ThreadInfo *)proc->pr_CIS;
    proc->pr_CIS = 0;
#else
    inf = (ThreadInfo *)FindTask(NULL)->tc_UserData;
#endif

    // wait until the parent has set inf->task. Not by signal: a SIGF_SINGLE
    // arriving while DosEntry waits in ObtainSemaphore ends that wait unowned
    ObtainSemaphore(&thread_sem);
    ReleaseSemaphore(&thread_sem);

    // trim the name
    //inf->task->tc_Node.ln_Name[inf->oldlen];

    // we have to set the priority here to avoid race conditions
    SetTaskPri(inf->task, inf->attr.param.sched_priority);

#ifdef USE_ASYNC_CANCEL
    // set the exception handler for async cancellation
    oldexcept = inf->task->tc_ExceptCode;
#ifdef __AROS__
    inf->task->tc_ExceptCode = &AROS_ASMSYMNAME(CancelHandler);
#else
    inf->task->tc_ExceptCode = &CancelHandler;
#endif
    SetExcept(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
#endif

    // give this worker its own C runtime base(s) before running any user or
    // CRT code; run once, before the pthread_exit jump point
    RunCRTThreadHooks(TRUE);

    // set a jump point for pthread_exit
    if (!setjmp(inf->jmp))
    {
        // custom stack requires special handling
        if (inf->attr.stackaddr != NULL && inf->attr.stacksize > 0)
        {
#if defined(__AMIGA__) && !defined(__MORPHOS__)
            struct StackSwapStruct stack;
            stack.stk_Lower = inf->attr.stackaddr;
            stack.stk_Upper = (ULONG)((char *)stack.stk_Lower + inf->attr.stacksize);
            stack.stk_Pointer = (APTR)stack.stk_Upper;

            StackSwap(&stack);

            inf->ret = inf->start(inf->arg);
#else
            struct StackSwapArgs swapargs;
            struct StackSwapStruct stack;

            swapargs.Args[0] = (IPTR)inf->arg;
            stack.stk_Lower = inf->attr.stackaddr;
#ifdef __MORPHOS__
            stack.stk_Upper = (IPTR)stack.stk_Lower + inf->attr.stacksize;
            stack.stk_Pointer = (APTR)stack.stk_Upper;
#else
            stack.stk_Upper = (APTR)((IPTR)stack.stk_Lower + inf->attr.stacksize);
            stack.stk_Pointer = stack.stk_Upper;
#endif

            inf->ret = (void *)NewStackSwap(&stack, inf->start, &swapargs);
#endif
        }
        else
        {
            inf->ret = inf->start(inf->arg);
        }
    }

#ifdef USE_ASYNC_CANCEL
    // remove the exception handler
    SetExcept(0, SIGBREAKF_CTRL_C);
    inf->task->tc_ExceptCode = oldexcept;
#endif

    // destroy all non-NULL TLS key values
    // since the destructors can set the keys themselves, we have to do multiple iterations
    // (not when cancelled by the program's exit: as with POSIX exit() the thread
    // just ends, its destructors may wait for the code it was cancelled in)
    ObtainSemaphoreShared(&tls_sem);
    for (j = 0; !__pthread_exiting && foundkey && j < PTHREAD_DESTRUCTOR_ITERATIONS; j++)
    {
        foundkey = FALSE;
        for (i = 0; i < PTHREAD_KEYS_MAX; i++)
        {
            if (tlskeys[i].used && tlskeys[i].destructor && inf->tlsvalues[i])
            {
                void *oldvalue = inf->tlsvalues[i];
                inf->tlsvalues[i] = NULL;
                tlskeys[i].destructor(oldvalue);
                foundkey = TRUE;
            }
        }
    }
    ReleaseSemaphore(&tls_sem);

    // release the C runtime base(s) acquired above, after all user code
    // (including TLS destructors) has run
    RunCRTThreadHooks(FALSE);

    // decide under thread_sem: pthread_detach() and pthread_join() may run
    // concurrently. A joiner is woken even if we were detached meanwhile.
    ObtainSemaphore(&thread_sem);
    waiter = inf->waiter;
    if (!inf->detached)
        inf->finished = TRUE;
    else
        memset(inf, 0, sizeof(ThreadInfo));
    ReleaseSemaphore(&thread_sem);
    // the joiner may unload our code: let it run once we are gone
    Forbid();
    if (waiter)
        Signal(waiter, SIGF_PARENT);
}

/****************************************************************************
    NAME */
        int pthread_create(

/*  SYNOPSIS */
        pthread_t *thread,
        const pthread_attr_t *attr,
        void *(*start)(void *),
        void *arg)

/*  FUNCTION
        Start a worker running start(arg). The worker may execute before this
        call returns. A normal return from start stores its pointer result for
        pthread_join(); pthread_exit() also runs registered cleanup handlers.

        The attributes are copied. With NULL attributes, the worker is joinable,
        uses the creator's priority and stack extent, and starts with deferred
        cancellation enabled. A joinable worker retains its slot until joined;
        a detached worker releases its slot when it finishes.

    INPUTS
        thread - Non-NULL writable output for the new thread identifier.
        attr - Initialized attributes, or NULL for defaults. May be destroyed
            after this call. A supplied stack remains caller-owned and must
            remain valid until the worker has finished using it.
        start - Non-NULL worker entry point. Receives arg unchanged.
        arg - Borrowed argument pointer, which may be NULL. Its storage and
            everything reachable through it must outlive the worker's use.

    RESULT
        0 on success; *thread receives the identifier. EINVAL if thread or start
        is NULL. EAGAIN if no slot is available, a thread record cannot be
        allocated, or the process-creation result is observed as NULL.
        On error, *thread is unchanged. Errors are returned directly.

    NOTES
        Include <pthread.h> and link with the pthread link library. Call from
        task context with its runtime initialized. Creation can wait for the
        internal thread semaphore and system allocation; it has no timeout.

        The worker runs in a DOS process and applies attr's scheduling priority.
        Scheduling inheritance and contention-scope fields are not implemented
        by this creation path. The start routine must return a pointer whose
        lifetime permits its eventual use by the joiner, not worker-stack data.

        Worker exit runs non-NULL thread-specific destructors before releasing
        its C runtime. Destructors are skipped during program-wide teardown.
        Join or detach every successfully created joinable worker. Identifiers
        are reused; they are not permanent handles.

    BUGS
        An immediately finishing detached worker can clear its record before
        the creator tests inf->task after releasing the thread semaphore. Thus
        an EAGAIN result does not establish that the entry point never ran.

        Returning from start bypasses the cleanup-handler list in this
        implementation. Use balanced cleanup operations or pthread_exit()
        when those handlers must run.

    SEE ALSO
        pthread_join(),
        pthread_exit(),
        pthread_attr_init(),
        pthread_cancel()

*****************************************************************************/
{
    ThreadInfo *inf;
    char name[NAMELEN];
    size_t oldlen;
    pthread_t threadnew;
#if defined(__AROS__)
    APTR affinity = NULL;
#endif

    D(bug("%s(%p, %p, %p, %p)\n", __FUNCTION__, thread, attr, start, arg));

    if (thread == NULL || start == NULL)
        return EINVAL;

#if defined(__AROS__)
    /* pthread_attr_setaffinity_np(): the CPUs the new thread may run on */
    if (attr && attr->affinityset)
    {
        int err = _pthread_cpuset_to_mask(sizeof(cpu_set_t), &attr->affinity, &affinity);

        if (err)
            return err;
    }
#endif

    ObtainSemaphore(&thread_sem);

    // grab an empty thread slot
    threadnew = GetThreadId(NULL);
    if (threadnew == PTHREAD_THREADS_MAX)
    {
        ReleaseSemaphore(&thread_sem);
        _pthread_free_affinity(affinity);
        return EAGAIN;
    }

    if (threadnew == threads_used)
    {
        // AllocVec: memory from a thread's C runtime goes away with the thread
        if (!(threads[threadnew] = AllocVec(sizeof(ThreadInfo), MEMF_ANY)))
        {
            ReleaseSemaphore(&thread_sem);
            _pthread_free_affinity(affinity);
            return EAGAIN;
        }
        __atomic_store_n(&threads_used, threadnew + 1, __ATOMIC_RELEASE);
    }

    // prepare the ThreadInfo structure
    inf = GetThreadInfo(threadnew);
    memset(inf, 0, sizeof(ThreadInfo));
    inf->start = start;
    inf->arg = arg;
    inf->parent = GET_THIS_TASK;
    if (attr)
        inf->attr = *attr;
    else
        pthread_attr_init(&inf->attr);
    NEWLIST((struct List *)&inf->cleanup);
    inf->cancelstate = PTHREAD_CANCEL_ENABLE;
    inf->canceltype = PTHREAD_CANCEL_DEFERRED;
    inf->detached = inf->attr.detachstate == PTHREAD_CREATE_DETACHED;

    // let's trick CreateNewProc into allocating a larger buffer for the name
    snprintf(name, sizeof(name), "pthread thread #%d", threadnew);
    oldlen = strlen(name);
    memset(name + oldlen, ' ', sizeof(name) - oldlen - 1);
    name[sizeof(name) - 1] = '\0';

    // start the child thread
    inf->task = (struct Task *)CreateNewProcTags(NP_Entry, (IPTR)StarterFunc,
#ifdef __MORPHOS__
        NP_CodeType, CODETYPE_PPC,
        (inf->attr.stackaddr == NULL && inf->attr.stacksize > 0) ? NP_PPCStackSize : TAG_IGNORE, inf->attr.stacksize,
        inf->attr.stacksize68k > 0 ? NP_StackSize : TAG_IGNORE, inf->attr.stacksize68k,
#else
        (inf->attr.stackaddr == NULL && inf->attr.stacksize > 0) ? NP_StackSize : TAG_IGNORE, inf->attr.stacksize,
#endif
#if defined(__AMIGA__) && !defined(__MORPHOS__)
        NP_Input, (IPTR)inf,
#else
        NP_UserData, inf,
#endif
        NP_Name, (IPTR)name,
#if defined(__AROS__)
        /* the mask becomes the system's once the process exists */
        affinity ? NP_Affinity : TAG_IGNORE, (IPTR)affinity,
#endif
        TAG_DONE);

    ReleaseSemaphore(&thread_sem);

    if (!inf->task)
    {
        inf->parent = NULL;
        _pthread_free_affinity(affinity);
        return EAGAIN;
    }

    *thread = threadnew;

    return 0;
}
