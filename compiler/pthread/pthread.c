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

#ifdef __MORPHOS__
#include <sys/time.h>
#endif
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#ifdef __AROS__
#include <libraries/stdc.h>
#endif

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "pthread_intern.h"
#include "debug.h"

#if defined(__AROS__)
#include <proto/task.h>
#endif


//#define USE_ASYNC_CANCEL

// A ThreadInfo is about 1 KB: allocated when its slot is first used rather
// than 2 MB of bss in every program. Slots below threads_used are allocated.
ThreadInfo *threads[PTHREAD_THREADS_MAX];
pthread_t threads_used;
static ThreadInfo mainthread;
struct SignalSemaphore thread_sem;
TLSKey tlskeys[PTHREAD_KEYS_MAX];
struct SignalSemaphore tls_sem;
// set once the program exits: remaining threads end without TLS destructors
volatile BOOL __pthread_exiting;

#if defined(__AROS__)
// Tasks that are not pthreads keep their thread-specific values in a task
// storage slot: other programs calling into a shared library that links
// this, or a program's raw subtasks. The slot dies with the task, so a new
// task at the same address starts empty. Taken on first use only, so a
// program whose threads all come from pthread_create() never needs one.
static struct Library *tlstaskres;
static LONG tlsslot;
#endif

//
// Helper functions
//

int SemaphoreIsInvalid(struct SignalSemaphore *sem)
{
    DB2(bug("%s(%p)\n", __FUNCTION__, sem));

    return (!sem || sem->ss_Link.ln_Type != NT_SIGNALSEM || sem->ss_WaitQueue.mlh_Tail != NULL);
}

int SemaphoreIsMine(struct SignalSemaphore *sem)
{
    struct Task *me;

    DB2(bug("%s(%p)\n", __FUNCTION__, sem));

    me = GET_THIS_TASK;

    return (sem && sem->ss_NestCount > 0 && sem->ss_Owner == me);
}

ThreadInfo *GetThreadInfo(pthread_t thread)
{
    DB2(bug("%s(%u)\n", __FUNCTION__, thread));

    // TODO: more robust error handling?
    if (thread < __atomic_load_n(&threads_used, __ATOMIC_ACQUIRE))
        return threads[thread];

    return 0;
}

// The calling task's thread-specific values; NULL if it has none yet and
// create is FALSE, or if they cannot be allocated.
void **GetTLSValues(BOOL create)
{
    struct Task *task = GET_THIS_TASK;
    pthread_t thread = GetThreadId(task);

    if (thread != PTHREAD_THREADS_MAX)
        return GetThreadInfo(thread)->tlsvalues;

#if defined(__AROS__)
    {
        struct Library *TaskResBase;
        void **values = NULL;

        if (!tlsslot)
        {
            if (!create)
                return NULL;
            ObtainSemaphore(&tls_sem);
            if (!tlsslot && (TaskResBase = tlstaskres = OpenResource("task.resource")))
                tlsslot = AllocTaskStorageSlot();
            ReleaseSemaphore(&tls_sem);
            if (!tlsslot)
                return NULL;
        }

        TaskResBase = tlstaskres;
        values = (void **)GetTaskStorageSlot(tlsslot);
        if (!values && create)
        {
            values = AllocVec(PTHREAD_KEYS_MAX * sizeof(void *), MEMF_ANY | MEMF_CLEAR);
            if (values && !SetTaskStorageSlot(tlsslot, (IPTR)values))
            {
                FreeVec(values);
                values = NULL;
            }
        }
        return values;
    }
#else
    return GetThreadInfo(0)->tlsvalues;
#endif
}

pthread_t GetThreadId(struct Task *task)
{
    pthread_t i, used = __atomic_load_n(&threads_used, __ATOMIC_ACQUIRE);

    DB2(bug("%s(%p)\n", __FUNCTION__, task));

    // 0 is main task, First thread id will be 1 so that it is different than default value of pthread_t
    for (i = 0; i < used; i++)
    {
        // be sure not to select existing, "not joined" thread slot with the exec task pointer same as
        // this one (if a new exec task structure get allocated exactly at the same addres as the old one)
        if (threads[i]->task == task && !threads[i]->finished)
            return i;
    }

    // a free slot may also be a new one
    return task == NULL ? used : PTHREAD_THREADS_MAX;
}

#if defined __mc68000__
/* No CAS instruction on m68k */
static int __m68k_sync_val_compare_and_swap(int *v, int o, int n)
{
    int ret;

    Disable();
    if ((*v) == (o))
        (*v) = (n);
    ret = (*v);
    Enable();

    return ret;
}
#undef __sync_val_compare_and_swap
#define __sync_val_compare_and_swap(v, o, n) __m68k_sync_val_compare_and_swap(v, o, n)

static int __m68k_sync_lock_test_and_set(int *v, int n)
{
    int old;

    Disable();
    old = *v;
    (*v) = (n);
    Enable();

    return old;
}
#undef __sync_lock_test_and_set
#define __sync_lock_test_and_set(v, n) __m68k_sync_lock_test_and_set(v, n)
#undef __sync_lock_release
#define __sync_lock_release(v) __m68k_sync_lock_test_and_set(v, 0)
#endif

BOOL OpenTimerDevice(struct IORequest *io, struct MsgPort *mp, struct Task *task)
{
    BYTE signal;

    DB2(bug("%s(%p,%p,%p)\n", __FUNCTION__, io, mp, task));

    // prepare MsgPort
    // Zero the whole struct first: on SMP AROS a MsgPort carries an
    // mp_SpinLock that InternalPutMsg()/ReplyMsg() lock. The callers pass a
    // stack MsgPort, so leaving the spinlock as uninitialised garbage makes
    // the timer reply spin forever and hang the system.
    memset(mp, 0, sizeof(*mp));
    mp->mp_Node.ln_Type = NT_MSGPORT;
    mp->mp_Node.ln_Pri = 0;
    mp->mp_Node.ln_Name = NULL;
    mp->mp_Flags = PA_SIGNAL;
    mp->mp_SigTask = task;
    signal = AllocSignal(-1);
    if (signal == -1)
    {
        signal = SIGB_TIMER_FALLBACK;
        SetSignal(SIGF_TIMER_FALLBACK, 0);
    }
    mp->mp_SigBit = signal;
    NEWLIST(&mp->mp_MsgList);

    // prepare IORequest
    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->io_Message.mn_Node.ln_Pri = 0;
    io->io_Message.mn_Node.ln_Name = NULL;
    io->io_Message.mn_ReplyPort = mp;
    io->io_Message.mn_Length = sizeof(struct timerequest);

    // open timer.device
#if defined(__AMIGA__) && !defined(__MORPHOS__)
    io->io_Device = DOSBase->dl_TimeReq->tr_node.io_Device;
    io->io_Unit = DOSBase->dl_TimeReq->tr_node.io_Unit;
    io->io_Error = 0;
    return TRUE;
#else
    return !OpenDevice((STRPTR)TIMERNAME, UNIT_MICROHZ, io, 0);
#endif
}

void CloseTimerDevice(struct IORequest *io)
{
    struct MsgPort *mp;

    DB2(bug("%s(%p)\n", __FUNCTION__, io));

    if (!CheckIO(io))
    {
        AbortIO(io);
        WaitIO(io);
    }

#if defined(__AMIGA__) && !defined(__MORPHOS__)
    io->io_Device = (struct Device *)-1;
#else
    if (io->io_Device != NULL)
        CloseDevice(io);
#endif

    mp = io->io_Message.mn_ReplyPort;
    if (mp->mp_SigBit != SIGB_TIMER_FALLBACK)
        FreeSignal(mp->mp_SigBit);
}

//
// Thread specific data functions
//

/****************************************************************************
    NAME */
        int pthread_key_delete(

/*  SYNOPSIS */
        pthread_key_t key)

/*  FUNCTION
        Release a thread-specific storage key for future reuse.

    INPUTS
        key - Previously allocated key below PTHREAD_KEYS_MAX.

    RESULT
        0 if a used key is marked free; EINVAL if the key is out of range or
        already unused.

    NOTES
        The key table is protected by its internal semaphore. Deletion clears
        the registered destructor but does not invoke it or free values stored
        by individual threads.

    SEE ALSO
        pthread_key_create()
        pthread_setspecific()
        pthread_getspecific()

*****************************************************************************/
{
    TLSKey *tls;

    D(bug("%s(%u)\n", __FUNCTION__, key));

    if (key >= PTHREAD_KEYS_MAX)
        return EINVAL;

    tls = &tlskeys[key];

    ObtainSemaphore(&tls_sem);

    if (tls->used == FALSE)
    {
        ReleaseSemaphore(&tls_sem);
        return EINVAL;
    }

    tls->used = FALSE;
    tls->destructor = NULL;

    ReleaseSemaphore(&tls_sem);

    return 0;
}

//
// Mutex attribute functions
//

/****************************************************************************
    NAME */
        int pthread_mutexattr_gettype(

/*  SYNOPSIS */
        pthread_mutexattr_t *attr, int *kind)

/*  FUNCTION
        Read the mutex type stored in a mutex attribute object.

    INPUTS
        attr - Initialized mutex attributes.
        kind - Optional output receiving the stored mutex type.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        A NULL kind pointer is accepted. The function does not validate the
        current stored value.

    SEE ALSO
        pthread_mutexattr_settype()
        pthread_mutex_init()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, kind));

    if (attr == NULL)
        return EINVAL;

    if (kind)
        *kind = attr->kind;

    return 0;
}

//
// Mutex functions
//

static int _obtain_sema_timed(struct SignalSemaphore *sema, const struct timespec *abstime, int shared)
{
    struct MsgPort msgport;
    struct SemaphoreMessage msg;
    struct timerequest timerio;
    struct Task *task;
    struct timeval tvabstime;
#if defined(__AMIGA__) && !defined(__MORPHOS__)
    struct SemaphoreRequest sr;
    ULONG sigmask, sigs;
#else
    struct Message *m1, *m2;
#endif

    DB2(bug("%s(%p, %p, %d)\n", __FUNCTION__, sema, abstime, shared));

    task = GET_THIS_TASK;

    if (!OpenTimerDevice((struct IORequest *)&timerio, &msgport, task))
    {
        CloseTimerDevice((struct IORequest *)&timerio);
        return EINVAL;
    }

    timerio.tr_node.io_Command = TR_ADDREQUEST;
    timerio.tr_node.io_Flags = 0;
    TIMESPEC_TO_TIMEVAL(&tvabstime, abstime);
    //if (!relative)
    {
        struct timeval starttime;
        // absolute time has to be converted to relative
        // GetSysTime can't be used due to the timezone offset in abstime
        gettimeofday(&starttime, NULL);
        // tv_sec is unsigned: test for a passed deadline before subtracting
        if (!timercmp(&starttime, &tvabstime, <))
        {
            CloseTimerDevice((struct IORequest *)&timerio);
            return ETIMEDOUT;
        }
        timersub(&tvabstime, &starttime, &tvabstime);
    }
    timerio.tr_time.tv_secs = tvabstime.tv_sec;
    timerio.tr_time.tv_micro = tvabstime.tv_usec;
    SendIO((struct IORequest *)&timerio);

#if defined(__AMIGA__) && !defined(__MORPHOS__)
    // Procure is broken on older systems... hand made...
    sr.sr_Waiter = (struct Task *)((IPTR)task | shared);

    sigmask = SIGF_SINGLE | (1<<msgport.mp_SigBit);
    Forbid();
    task->tc_SigRecvd &= ~sigmask;
    AddTail((struct List *)&sema->ss_WaitQueue, (struct Node *)&sr.sr_Link);
    sigs = Wait(sigmask);
    Permit();

    if (sigs & SIGF_SINGLE)
        msg.ssm_Semaphore = sema;
    else
        msg.ssm_Semaphore = NULL;
#else
    msg.ssm_Message.mn_Node.ln_Type = NT_MESSAGE;
    msg.ssm_Message.mn_Node.ln_Name = (char *)(IPTR)shared;
    msg.ssm_Message.mn_ReplyPort = &msgport;
    Procure(sema, &msg);

    WaitPort(&msgport);
    m1 = GetMsg(&msgport);
    m2 = GetMsg(&msgport);
    if (m1 == &timerio.tr_node.io_Message || m2 == &timerio.tr_node.io_Message)
        Vacate(sema, &msg);
#endif

    CloseTimerDevice((struct IORequest *)&timerio);

    if (msg.ssm_Semaphore == NULL)
        return ETIMEDOUT;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_mutex_timedlock(

/*  SYNOPSIS */
        pthread_mutex_t *mutex, const struct timespec *abstime)

/*  FUNCTION
        Try to acquire an exclusive mutex until an absolute deadline.

    INPUTS
        mutex - Mutex to acquire.
        abstime - Absolute realtime deadline; NULL takes the ordinary
                  pthread_mutex_lock() path.

    RESULT
        0 when the timed helper reports success; EINVAL for NULL mutex,
        invalid nanoseconds, or timer-device failure; EDEADLK for detected
        nonrecursive self-lock; ETIMEDOUT if the deadline has passed. Other
        try-lock errors propagate.

    NOTES
        First attempts immediate acquisition. If busy, opens timer.device,
        converts the absolute timestamp against gettimeofday(), and waits for
        a semaphore acquisition or timer reply. Errors are returned directly.

    BUGS
        A NULL deadline is treated as an unbounded lock rather than rejected.
        An immediately successful pthread_mutex_trylock() is followed by
        another timed semaphore acquisition instead of an immediate return.
        This can increase the Exec semaphore nest count twice, so one
        pthread_mutex_unlock() may not fully release the mutex. No
        independently selected clock is supported.

    SEE ALSO
        pthread_mutex_lock()
        pthread_mutex_trylock()
        pthread_mutex_unlock()

*****************************************************************************/
{
    int result;

    D(bug("%s(%p, %p)\n", __FUNCTION__, mutex, abstime));

    if (mutex == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_mutex_lock(mutex);
    else if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
        return EINVAL;

    result = pthread_mutex_trylock(mutex);
    if (result != 0)
    {
        // pthread_mutex_trylock returns EBUSY when a deadlock would occur
        if (result != EBUSY)
            return result;
        else if (mutex->kind != PTHREAD_MUTEX_RECURSIVE && SemaphoreIsMine(&mutex->semaphore))
            return EDEADLK;
    }

    return _obtain_sema_timed(&mutex->semaphore, abstime, SM_EXCLUSIVE);
}

//
// Condition variable attribute functions
//

/****************************************************************************
    NAME */
        int pthread_condattr_init(

/*  SYNOPSIS */
        pthread_condattr_t *attr)

/*  FUNCTION
        Initialize condition-variable attributes with the default realtime
        clock.

    INPUTS
        attr - Writable condition attribute object.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        Clears the object and sets cond_clock to CLOCK_REALTIME when the
        symbol is available. Initialize attributes before passing them to
        pthread_cond_init().

    SEE ALSO
        pthread_condattr_setclock()
        pthread_cond_init()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_condattr_t));
#ifdef CLOCK_REALTIME
    /* POSIX default: absolute deadlines read against the realtime clock. */
    attr->cond_clock = CLOCK_REALTIME;
#endif

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_condattr_setclock(

/*  SYNOPSIS */
        pthread_condattr_t *attr, clockid_t clock_id)

/*  FUNCTION
        Select the clock used to interpret an absolute condition-wait
        deadline.

    INPUTS
        attr - Initialized condition attribute object.
        clock_id - CLOCK_REALTIME or CLOCK_MONOTONIC.

    RESULT
        0 on success; EINVAL if attr is NULL or clock_id is unsupported.

    NOTES
        Stores the chosen clock. pthread_cond_init() copies it into the
        condition variable; changing attributes later does not change existing
        conditions.

    SEE ALSO
        pthread_condattr_getclock()
        pthread_cond_timedwait()

*****************************************************************************/
{
    D(bug("%s(%p, %d)\n", __FUNCTION__, attr, (int)clock_id));

    if (attr == NULL)
        return EINVAL;

    /* Only the clocks an AROS condvar can wait on. posixc clock_gettime()
     * serves both; anything else (process/thread CPU time) has no
     * timer.device to wait on, so refusing is the honest answer. */
    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC)
        return EINVAL;

    attr->cond_clock = clock_id;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_condattr_getclock(

/*  SYNOPSIS */
        const pthread_condattr_t *attr, clockid_t *clock_id)

/*  FUNCTION
        Read the deadline clock selected in condition attributes.

    INPUTS
        attr - Initialized condition attributes.
        clock_id - Non-NULL writable output for the clock ID.

    RESULT
        0 on success; EINVAL if either pointer is NULL.

    NOTES
        Reports the stored field. It does not query timer.device or the clock
        currently used by a running wait.

    SEE ALSO
        pthread_condattr_setclock()
        pthread_cond_timedwait()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, clock_id));

    if (attr == NULL || clock_id == NULL)
        return EINVAL;

    *clock_id = attr->cond_clock;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_condattr_destroy(

/*  SYNOPSIS */
        pthread_condattr_t *attr)

/*  FUNCTION
        Clear a condition-variable attribute object after use.

    INPUTS
        attr - Initialized condition attribute object no longer needed.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        Zeros the attribute storage. It does not deallocate the caller-owned
        object or affect condition variables already initialized from it.

    SEE ALSO
        pthread_condattr_init()
        pthread_cond_init()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_condattr_t));

    return 0;
}

//
// Condition variable functions
//

/****************************************************************************
    NAME */
        int pthread_cond_timedwait_relative_np(

/*  SYNOPSIS */
        pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *reltime)

/*  FUNCTION
        Wait on a condition variable using a relative time interval.

    INPUTS
        cond - Initialized condition variable.
        mutex - Locked mutex associated with the condition.
        reltime - Relative duration passed to the shared condition-wait
                  implementation.

    RESULT
        Returns the status of _pthread_cond_timedwait(), including timeout and
        argument errors established by that implementation.

    NOTES
        Calls the existing condition-wait helper with relative mode enabled.
        The caller must hold the mutex and recheck its shared predicate after
        waking; cancellation can exit the worker.

    SEE ALSO
        pthread_cond_timedwait()
        pthread_cond_wait()
        pthread_condattr_setclock()

*****************************************************************************/
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, reltime));

    return _pthread_cond_timedwait(cond, mutex, reltime, TRUE);
}


//
// Barrier functions
//

/****************************************************************************
    NAME */
        int pthread_barrier_init(

/*  SYNOPSIS */
        pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)

/*  FUNCTION
        Initialize a reusable barrier for a fixed number of participating
        threads.

    INPUTS
        barrier - Writable barrier storage not currently in use.
        attr - Barrier attributes; currently ignored.
        count - Number of arrivals required to complete each generation; must
                be nonzero.

    RESULT
        0 on success; EINVAL if barrier is NULL or count is zero.

    NOTES
        Stores count and initializes an embedded condition variable and mutex.
        All participants must reach the barrier before they can proceed.
        Caller owns the barrier storage.

    BUGS
        The return values of the embedded mutex and condition initializers are
        not checked. No maximum participant count or overflow guard is
        enforced.

    SEE ALSO
        pthread_barrier_wait()
        pthread_barrier_destroy()

*****************************************************************************/
{
    D(bug("%s(%p, %p, %u)\n", __FUNCTION__, barrier, attr, count));

    if (barrier == NULL || count == 0)
        return EINVAL;

    barrier->curr_height = count;
    barrier->total_height = PTHREAD_BARRIER_FLAG;
    pthread_cond_init(&barrier->breeched, NULL);
    pthread_mutex_init(&barrier->lock, NULL);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_barrier_destroy(

/*  SYNOPSIS */
        pthread_barrier_t *barrier)

/*  FUNCTION
        Destroy a barrier that has no arriving or departing participants.

    INPUTS
        barrier - Initialized, externally quiescent barrier.

    RESULT
        0 if destruction proceeds; EINVAL for NULL; EBUSY if the mutex cannot
        be acquired, a generation is active, or the condition destroy fails.

    NOTES
        Attempts a nonblocking lock and checks internal generation state
        before destroying the embedded condition and mutex. Do not reuse the
        storage without initialization.

    BUGS
        A successful probe does not exclude new arrivals; external quiescence
        is required. The return value from the final mutex destruction is
        ignored.

    SEE ALSO
        pthread_barrier_init()
        pthread_barrier_wait()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, barrier));

    if (barrier == NULL)
        return EINVAL;

    if (pthread_mutex_trylock(&barrier->lock) != 0)
        return EBUSY;

    if (barrier->total_height > PTHREAD_BARRIER_FLAG)
    {
        pthread_mutex_unlock(&barrier->lock);
        return EBUSY;
    }

    pthread_mutex_unlock(&barrier->lock);

    if (pthread_cond_destroy(&barrier->breeched) != 0)
        return EBUSY;

    pthread_mutex_destroy(&barrier->lock);
    barrier->curr_height = barrier->total_height = 0;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_barrier_wait(

/*  SYNOPSIS */
        pthread_barrier_t *barrier)

/*  FUNCTION
        Wait for all members of a barrier generation and release them
        together.

    INPUTS
        barrier - Initialized barrier shared by the participating threads.

    RESULT
        PTHREAD_BARRIER_SERIAL_THREAD for the final arrival; 0 for other
        arrivals after the generation completes; EINVAL for NULL.

    NOTES
        Uses an embedded mutex and condition variable. The last arrival
        broadcasts to waiting peers. A departing generation must finish before
        the next generation is admitted.

    BUGS
        The implementation does not propagate embedded mutex/condition
        failures. If a participant exits or never arrives, other participants
        may wait indefinitely.

    SEE ALSO
        pthread_barrier_init()
        pthread_barrier_destroy()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, barrier));

    if (barrier == NULL)
        return EINVAL;

    pthread_mutex_lock(&barrier->lock);

    // wait until everyone exits the barrier
    while (barrier->total_height > PTHREAD_BARRIER_FLAG)
        pthread_cond_wait(&barrier->breeched, &barrier->lock);

    // are we the first to enter?
    if (barrier->total_height == PTHREAD_BARRIER_FLAG) barrier->total_height = 0;

    barrier->total_height++;

    if (barrier->total_height == barrier->curr_height)
    {
        barrier->total_height += PTHREAD_BARRIER_FLAG - 1;
        pthread_cond_broadcast(&barrier->breeched);

        pthread_mutex_unlock(&barrier->lock);

        return PTHREAD_BARRIER_SERIAL_THREAD;
    }
    else
    {
        // wait until enough threads enter the barrier
        while (barrier->total_height < PTHREAD_BARRIER_FLAG)
            pthread_cond_wait(&barrier->breeched, &barrier->lock);

        barrier->total_height--;

        // get entering threads to wake up
        if (barrier->total_height == PTHREAD_BARRIER_FLAG)
            pthread_cond_broadcast(&barrier->breeched);

        pthread_mutex_unlock(&barrier->lock);

        return 0;
    }
}

//
// Read-write lock attribute functions
//

/****************************************************************************
    NAME */
        int pthread_rwlockattr_init(

/*  SYNOPSIS */
        pthread_rwlockattr_t *attr)

/*  FUNCTION
        Initialize attributes for a reader-writer lock.

    INPUTS
        attr - Writable reader-writer lock attributes.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        Clears the attribute structure. No process-sharing policy is applied
        by pthread_rwlock_init().

    SEE ALSO
        pthread_rwlock_init()
        pthread_rwlockattr_destroy()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_rwlockattr_t));

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_rwlockattr_destroy(

/*  SYNOPSIS */
        pthread_rwlockattr_t *attr)

/*  FUNCTION
        Clear an unused reader-writer lock attribute object.

    INPUTS
        attr - Initialized attributes to invalidate.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        Zeros the caller-owned structure; existing locks are unaffected.

    SEE ALSO
        pthread_rwlockattr_init()
        pthread_rwlock_init()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_rwlockattr_t));

    return 0;
}

//
// Read-write lock functions
//

/****************************************************************************
    NAME */
        int pthread_rwlock_init(

/*  SYNOPSIS */
        pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr)

/*  FUNCTION
        Initialize the Exec shared/exclusive semaphore used by a reader-writer
        lock.

    INPUTS
        lock - Writable lock storage not currently used by another task.
        attr - Attributes; accepted but ignored.

    RESULT
        0 on success; EINVAL if lock is NULL.

    NOTES
        Initializes the embedded Exec SignalSemaphore. The caller owns the
        lock storage; destroy only when no holders or waiters remain.

    BUGS
        No process-sharing attributes, fairness policy, or other optional lock
        attributes are consumed.

    SEE ALSO
        pthread_rwlock_rdlock()
        pthread_rwlock_wrlock()
        pthread_rwlock_destroy()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, lock, attr));

    if (lock == NULL)
        return EINVAL;

    InitSemaphore(&lock->semaphore);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_rwlock_destroy(

/*  SYNOPSIS */
        pthread_rwlock_t *lock)

/*  FUNCTION
        Clear an unused reader-writer lock after a nonblocking semaphore
        probe.

    INPUTS
        lock - Initialized, idle lock that no task can acquire concurrently.

    RESULT
        0 if the semaphore is uninitialized or the probe succeeds; EINVAL if
        lock is NULL; EBUSY if the exclusive probe fails.

    NOTES
        If the semaphore is initialized, attempts exclusive acquisition,
        releases it, and zeros the lock storage.

    BUGS
        The probe is not a synchronization barrier against subsequent
        acquisitions. A lock held by the calling owner may pass Exec recursive
        acquisition checks. External quiescence is required.

    SEE ALSO
        pthread_rwlock_init()
        pthread_rwlock_unlock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    // probably a statically allocated rwlock
    if (SemaphoreIsInvalid(&lock->semaphore))
        return 0;

    if (AttemptSemaphore(&lock->semaphore) == FALSE)
        return EBUSY;

    ReleaseSemaphore(&lock->semaphore);
    memset(lock, 0, sizeof(pthread_rwlock_t));

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_rwlock_tryrdlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock)

/*  FUNCTION
        Attempt a shared reader lock without waiting.

    INPUTS
        lock - Reader-writer lock to acquire.

    RESULT
        0 on acquisition; EINVAL if lock is NULL; EBUSY if the shared attempt
        fails.

    NOTES
        Lazily initializes a statically initialized lock and calls
        AttemptSemaphoreShared(). Unlock once for each successful acquisition.

    BUGS
        Lazy initialization is not serialized; initialize explicitly before
        concurrent first use. Deadlock checks for a caller already holding the
        write lock are absent.

    SEE ALSO
        pthread_rwlock_rdlock()
        pthread_rwlock_unlock()

*****************************************************************************/
{
    ULONG ret;

    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    // initialize static rwlocks
    if (SemaphoreIsInvalid(&lock->semaphore))
        pthread_rwlock_init(lock, NULL);

    ret = AttemptSemaphoreShared(&lock->semaphore);

    return (ret == TRUE) ? 0 : EBUSY;
}

/****************************************************************************
    NAME */
        int pthread_rwlock_trywrlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock)

/*  FUNCTION
        Attempt exclusive writer acquisition without blocking.

    INPUTS
        lock - Reader-writer lock to acquire.

    RESULT
        0 on acquisition; EINVAL for NULL; EBUSY if exclusive acquisition
        fails.

    NOTES
        Initializes a static semaphore lazily, then uses Exec
        AttemptSemaphore().

    BUGS
        Exec semaphore ownership can make recursive acquisition succeed; this
        wrapper does not explicitly reject a writer already owning the lock.
        Concurrent lazy initialization is also unguarded.

    SEE ALSO
        pthread_rwlock_wrlock()
        pthread_rwlock_unlock()

*****************************************************************************/
{
    ULONG ret;

    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    // initialize static rwlocks
    if (SemaphoreIsInvalid(&lock->semaphore))
        pthread_rwlock_init(lock, NULL);

    ret = AttemptSemaphore(&lock->semaphore);

    return (ret == TRUE) ? 0 : EBUSY;
}

/****************************************************************************
    NAME */
        int pthread_rwlock_rdlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock)

/*  FUNCTION
        Acquire a shared reader lock, waiting if exclusive ownership prevents
        it.

    INPUTS
        lock - Reader-writer lock to acquire.

    RESULT
        0 after acquisition; EINVAL if lock is NULL. Cancellation at the
        initial check may terminate the calling thread.

    NOTES
        Calls pthread_testcancel(), lazily initializes static locks, then
        blocks in ObtainSemaphoreShared(). Release with
        pthread_rwlock_unlock().

    BUGS
        No explicit deadlock test is active for a caller already holding a
        write lock; the source contains a disabled ownership check. Concurrent
        lazy initialization is unguarded.

    SEE ALSO
        pthread_rwlock_tryrdlock()
        pthread_rwlock_timedrdlock()
        pthread_rwlock_unlock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    pthread_testcancel();

    // initialize static rwlocks
    if (SemaphoreIsInvalid(&lock->semaphore))
        pthread_rwlock_init(lock, NULL);

    // "Results are undefined if the calling thread holds a write lock on rwlock at the time the call is made."
    /*
    // we might already have a write lock
    if (SemaphoreIsMine(&lock->semaphore))
        return EDEADLK;
    */

    ObtainSemaphoreShared(&lock->semaphore);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_rwlock_timedrdlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock, const struct timespec *abstime)

/*  FUNCTION
        Acquire a shared reader lock subject to an absolute deadline.

    INPUTS
        lock - Reader-writer lock to acquire.
        abstime - Absolute realtime deadline; NULL selects an unbounded read
                  lock.

    RESULT
        0 on success; EINVAL for NULL lock, malformed nanoseconds, or
        timer-device failure; ETIMEDOUT after the deadline; other try-lock
        errors propagate.

    NOTES
        Checks cancellation, tries immediate shared acquisition, then waits
        using _obtain_sema_timed() and the Exec shared-semaphore mode.

    BUGS
        NULL abstime is accepted as an unbounded operation. The timed helper
        uses gettimeofday(), not a condition-variable clock attribute.
        Concurrent lazy initialization is unguarded.

    SEE ALSO
        pthread_rwlock_rdlock()
        pthread_rwlock_tryrdlock()

*****************************************************************************/
{
    int result;

    D(bug("%s(%p, %p)\n", __FUNCTION__, lock, abstime));

    if (lock == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_rwlock_rdlock(lock);
    else if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
        return EINVAL;

    pthread_testcancel();

    result = pthread_rwlock_tryrdlock(lock);
    if (result != EBUSY)
        return result;

    return _obtain_sema_timed(&lock->semaphore, abstime, SM_SHARED);
}

/****************************************************************************
    NAME */
        int pthread_rwlock_wrlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock)

/*  FUNCTION
        Acquire an exclusive writer lock on a reader-writer semaphore.

    INPUTS
        lock - Reader-writer lock to acquire.

    RESULT
        0 after exclusive acquisition; EINVAL if lock is NULL; EDEADLK if the
        calling task already owns the semaphore.

    NOTES
        Checks cancellation, initializes static locks lazily, and waits in
        ObtainSemaphore(). Release with pthread_rwlock_unlock().

    BUGS
        No fairness, writer-preference, or lock-upgrade guarantee is
        established. Concurrent lazy initialization is unguarded.

    SEE ALSO
        pthread_rwlock_trywrlock()
        pthread_rwlock_timedwrlock()
        pthread_rwlock_unlock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    pthread_testcancel();

    // initialize static rwlocks
    if (SemaphoreIsInvalid(&lock->semaphore))
        pthread_rwlock_init(lock, NULL);

    if (SemaphoreIsMine(&lock->semaphore))
        return EDEADLK;

    ObtainSemaphore(&lock->semaphore);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_rwlock_timedwrlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock, const struct timespec *abstime)

/*  FUNCTION
        Attempt exclusive writer acquisition until an absolute deadline.

    INPUTS
        lock - Reader-writer lock to acquire.
        abstime - Absolute realtime deadline; NULL takes the ordinary
                  writer-lock path.

    RESULT
        0 on success; EINVAL for NULL lock, invalid nanoseconds, or timer
        setup failure; ETIMEDOUT on expiry; immediate try-lock errors
        propagate.

    NOTES
        Checks cancellation, first attempts a nonblocking exclusive lock, and
        waits through the timer/semaphore helper only on EBUSY.

    BUGS
        A NULL deadline is not rejected. Unlike pthread_rwlock_wrlock(), this
        path does not explicitly detect a self-owned write lock before
        entering timed acquisition.

    SEE ALSO
        pthread_rwlock_wrlock()
        pthread_rwlock_trywrlock()

*****************************************************************************/
{
    int result;

    D(bug("%s(%p, %p)\n", __FUNCTION__, lock, abstime));

    if (lock == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_rwlock_wrlock(lock);
    else if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
        return EINVAL;

    pthread_testcancel();

    result = pthread_rwlock_trywrlock(lock);
    if (result != EBUSY)
        return result;

    return _obtain_sema_timed(&lock->semaphore, abstime, SM_EXCLUSIVE);
}

/****************************************************************************
    NAME */
        int pthread_rwlock_unlock(

/*  SYNOPSIS */
        pthread_rwlock_t *lock)

/*  FUNCTION
        Release a reader or writer acquisition of a reader-writer lock.

    INPUTS
        lock - Reader-writer lock previously acquired by the calling task.

    RESULT
        0 if ReleaseSemaphore() is called; EINVAL for NULL; EPERM if the
        semaphore nest count is below one.

    NOTES
        Lazily initializes an invalid static semaphore and uses Exec
        ReleaseSemaphore() to release one acquisition.

    BUGS
        The check examines only ss_NestCount, not a reader ownership list. It
        cannot reliably reject a thread that does not own a shared
        acquisition; unlocking from the wrong thread has undefined effects.

    SEE ALSO
        pthread_rwlock_rdlock()
        pthread_rwlock_wrlock()
        pthread_rwlock_destroy()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    // initialize static rwlocks
    if (SemaphoreIsInvalid(&lock->semaphore))
        pthread_rwlock_init(lock, NULL);

    //if (!SemaphoreIsMine(&lock->semaphore))
    // if no one has obtained the semaphore don't unlock the rwlock
    // this can be a leap of faith because we don't maintain a separate list of readers
    if (lock->semaphore.ss_NestCount < 1)
        return EPERM;

    ReleaseSemaphore(&lock->semaphore);

    return 0;
}

//
// Spinlock functions
//

/****************************************************************************
    NAME */
        int pthread_spin_init(

/*  SYNOPSIS */
        pthread_spinlock_t *lock, int pshared)

/*  FUNCTION
        Initialize a spinlock in its unlocked state.

    INPUTS
        lock - Writable pthread_spinlock_t storage.
        pshared - Process-sharing request; ignored.

    RESULT
        0 on success; EINVAL if lock is NULL.

    NOTES
        Stores zero in the lock word. Storage is caller-owned and must remain
        valid while any thread uses it.

    BUGS
        No interprocess sharing policy is enforced; reinitializing a held or
        contended lock is not detected.

    SEE ALSO
        pthread_spin_lock()
        pthread_spin_destroy()

*****************************************************************************/
{
    D(bug("%s(%p, %d)\n", __FUNCTION__, lock, pshared));

    if (lock == NULL)
        return EINVAL;

    *lock = 0;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_spin_destroy(

/*  SYNOPSIS */
        pthread_spinlock_t *lock)

/*  FUNCTION
        Finish using a spinlock after all users have stopped accessing it.

    INPUTS
        lock - Previously initialized, externally quiescent spinlock.

    RESULT
        Always returns 0, including for a NULL pointer.

    NOTES
        The implementation does not read or modify the lock word and allocates
        no memory to release.

    BUGS
        No busy, validity, or NULL-pointer check is performed. A successful
        result does not prove the lock was idle.

    SEE ALSO
        pthread_spin_init()
        pthread_spin_unlock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_spin_lock(

/*  SYNOPSIS */
        pthread_spinlock_t *lock)

/*  FUNCTION
        Acquire a spinlock using an atomic test-and-set loop.

    INPUTS
        lock - Initialized spinlock to acquire.

    RESULT
        0 once the lock is acquired; EINVAL if lock is NULL.

    NOTES
        Uses __sync_lock_test_and_set(). On AROS the contended loop yields the
        processor with sched_yield(); the call can wait indefinitely.

    BUGS
        Non-recursive ownership is not tracked. Acquiring a lock already held
        by the same task can deadlock. The source contains an explicit TODO
        concerning yield frequency.

    SEE ALSO
        pthread_spin_trylock()
        pthread_spin_unlock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

#ifdef __MORPHOS__
    {
    unsigned int cnt = 0;
    while (__sync_lock_test_and_set((int *)lock, 1))
    {
        asm volatile("" ::: "memory");
        if ((cnt++ & 255) == 0)
            sched_yield();
    }
    }
#else
    while (__sync_lock_test_and_set((int *)lock, 1))
        sched_yield(); // TODO: don't yield the CPU every iteration
                        // SBF: if yield is implemented correctly there's nothing else to do.
#endif

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_spin_trylock(

/*  SYNOPSIS */
        pthread_spinlock_t *lock)

/*  FUNCTION
        Attempt an atomic spinlock acquisition without waiting.

    INPUTS
        lock - Initialized spinlock to acquire.

    RESULT
        0 on acquisition; EINVAL for NULL; EBUSY if test-and-set finds the
        lock already held.

    NOTES
        Does not yield, sleep, or queue a waiter. Each successful acquisition
        requires a matching unlock.

    SEE ALSO
        pthread_spin_lock()
        pthread_spin_unlock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    if (__sync_lock_test_and_set((int *)lock, 1))
        return EBUSY;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_spin_unlock(

/*  SYNOPSIS */
        pthread_spinlock_t *lock)

/*  FUNCTION
        Clear an acquired spinlock using an atomic release operation.

    INPUTS
        lock - Lock held by the calling thread.

    RESULT
        0 after the atomic release; EINVAL if lock is NULL.

    NOTES
        Uses __sync_lock_release(). The caller must establish ownership and
        the lifetime of the lock word.

    BUGS
        No ownership or held-state check is performed. Unlocking an unowned or
        unlocked spinlock may break mutual exclusion.

    SEE ALSO
        pthread_spin_lock()
        pthread_spin_trylock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    __sync_lock_release((int *)lock);

    return 0;
}

//
// Thread attribute functions
//

/****************************************************************************
    NAME */
        int pthread_attr_getdetachstate(

/*  SYNOPSIS */
        const pthread_attr_t *attr, int *detachstate)

/*  FUNCTION
        Read the detach state stored in a thread attribute object.

    INPUTS
        attr - Initialized thread attributes.
        detachstate - Optional output for PTHREAD_CREATE_JOINABLE or
                      PTHREAD_CREATE_DETACHED; NULL suppresses the write.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        Reads the stored field; it does not inspect any running thread. Errors
        are returned directly.

    SEE ALSO
        pthread_attr_setdetachstate()
        pthread_create()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, detachstate));

    if (attr == NULL)
        return EINVAL;

    if (detachstate != NULL)
        *detachstate = attr->detachstate;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_attr_setdetachstate(

/*  SYNOPSIS */
        pthread_attr_t *attr, int detachstate)

/*  FUNCTION
        Choose whether new threads are joinable or detached.

    INPUTS
        attr - Initialized attributes to update.
        detachstate - PTHREAD_CREATE_JOINABLE or PTHREAD_CREATE_DETACHED.

    RESULT
        0 on success; EINVAL for a NULL attr or any other detach state.

    NOTES
        The selection takes effect when pthread_create() copies these
        attributes. Changing an attribute object does not detach an existing
        thread.

    SEE ALSO
        pthread_attr_getdetachstate()
        pthread_detach()
        pthread_create()

*****************************************************************************/
{
    D(bug("%s(%p, %d)\n", __FUNCTION__, attr, detachstate));

    if (attr == NULL || (detachstate != PTHREAD_CREATE_JOINABLE && detachstate != PTHREAD_CREATE_DETACHED))
        return EINVAL;

    attr->detachstate = detachstate;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_attr_getstack(

/*  SYNOPSIS */
        const pthread_attr_t *attr, void **stackaddr, size_t *stacksize)

/*  FUNCTION
        Read the requested stack address and size from thread attributes.

    INPUTS
        attr - Initialized attributes.
        stackaddr - Optional output receiving the stored stack pointer; may be
                    NULL.
        stacksize - Optional output receiving the stored size in bytes; may be
                    NULL.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        The values describe an attribute object, not the current stack of a
        live thread. No allocation or validation occurs.

    SEE ALSO
        pthread_attr_setstack()
        pthread_getattr_np()

*****************************************************************************/
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, attr, stackaddr, stacksize));

    if (attr == NULL)
        return EINVAL;

    if (stackaddr != NULL)
        *stackaddr = attr->stackaddr;

    if (stacksize != NULL)
        *stacksize = attr->stacksize;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_attr_setstack(

/*  SYNOPSIS */
        pthread_attr_t *attr, void *stackaddr, size_t stacksize)

/*  FUNCTION
        Store a stack buffer address and requested size for future thread
        creation.

    INPUTS
        attr - Attributes to modify.
        stackaddr - Caller-owned stack pointer; NULL selects the
                    runtime-provided stack.
        stacksize - Stack extent in bytes. With a non-NULL stackaddr this must
                    be nonzero.

    RESULT
        0 on success; EINVAL if attr is NULL or a non-NULL stackaddr is paired
        with zero size.

    NOTES
        The function stores the two values without allocating, checking
        alignment, enforcing PTHREAD_STACK_MIN, or checking actual accessible
        memory. Keep a supplied buffer valid throughout the worker lifetime.

    BUGS
        Not all invalid stack sizes and pointers are rejected by the setter. A
        successful result does not establish that subsequent thread creation
        can use the buffer.

    SEE ALSO
        pthread_attr_getstack()
        pthread_attr_setstacksize()
        pthread_create()

*****************************************************************************/
{
    D(bug("%s(%p, %p, %u)\n", __FUNCTION__, attr, stackaddr, stacksize));

    if (attr == NULL || (stackaddr != NULL && stacksize == 0))
        return EINVAL;

    attr->stackaddr = stackaddr;
    attr->stacksize = stacksize;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_attr_getstacksize(

/*  SYNOPSIS */
        const pthread_attr_t *attr, size_t *stacksize)

/*  FUNCTION
        Read the stack size stored in thread attributes.

    INPUTS
        attr - Initialized attributes.
        stacksize - Optional output for the stored byte count; NULL is
                    accepted.

    RESULT
        The result of pthread_attr_getstack(attr, NULL, stacksize): 0 or
        EINVAL.

    NOTES
        This function does not query a running task or determine a minimum
        usable stack size.

    SEE ALSO
        pthread_attr_getstack()
        pthread_attr_setstacksize()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, stacksize));

    return pthread_attr_getstack(attr, NULL, stacksize);
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    D(bug("%s(%p, %u)\n", __FUNCTION__, attr, stacksize));

    return pthread_attr_setstack(attr, NULL, stacksize);
}

/****************************************************************************
    NAME */
        int pthread_attr_getschedparam(

/*  SYNOPSIS */
        const pthread_attr_t *attr, struct sched_param *param)

/*  FUNCTION
        Read the scheduling parameters stored in a thread attribute object.

    INPUTS
        attr - Initialized thread attributes.
        param - Optional writable struct sched_param receiving a copy of the
                stored parameters.

    RESULT
        0 on success; EINVAL if attr is NULL.

    NOTES
        This is a copy of the attribute field, not a query of the task
        scheduler. A NULL output pointer is accepted.

    SEE ALSO
        pthread_attr_setschedparam()
        pthread_getschedparam()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, param));

    if (attr == NULL)
        return EINVAL;

    if (param != NULL)
        *param = attr->param;

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_attr_setschedparam(

/*  SYNOPSIS */
        pthread_attr_t *attr, const struct sched_param *param)

/*  FUNCTION
        Store scheduling parameters for a subsequently created thread.

    INPUTS
        attr - Initialized attributes to update.
        param - Non-NULL scheduling parameters to copy.

    RESULT
        0 on success; EINVAL if attr or param is NULL.

    NOTES
        The entire struct sched_param is copied. The setter does not validate
        its priority; thread creation uses the stored priority.

    BUGS
        The pthread_create() path does not implement scheduling inheritance or
        contention-scope controls.

    SEE ALSO
        pthread_attr_getschedparam()
        pthread_create()
        pthread_setschedparam()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, param));

    if (attr == NULL || param == NULL)
        return EINVAL;

    attr->param = *param;

    return 0;
}

//
// Thread functions
//

#ifdef USE_ASYNC_CANCEL
#ifdef __MORPHOS__
static ULONG CancelHandlerFunc(void);
static struct EmulLibEntry CancelHandler =
{
    TRAP_LIB, 0, (void (*)(void))CancelHandlerFunc
};
static ULONG CancelHandlerFunc(void)
{
    ULONG signals = (ULONG)REG_D0;
    APTR data = (APTR)REG_A1;
    struct ExecBase *SysBase = (struct ExecBase *)REG_A6;
#else
AROS_UFH3S(ULONG, CancelHandler,
    AROS_UFHA(ULONG, signals, D0),
    AROS_UFHA(APTR, data, A1),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
#endif

    DB2(bug("%s(%u, %p, %p)\n", __FUNCTION__, signals, data, SysBase));

    pthread_testcancel();

    return signals;
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}
#endif

/****************************************************************************
    NAME */
        int pthread_detach(

/*  SYNOPSIS */
        pthread_t thread)

/*  FUNCTION
        Detach a thread so that its identifier slot can be released without
        joining.

    INPUTS
        thread - Identifier of the worker to detach.

    RESULT
        0 on success; ESRCH if the slot or its task is unavailable; EINVAL if
        the thread is already detached.

    NOTES
        A running detached worker releases its slot on exit. An
        already-finished joinable worker has its slot cleared immediately. No
        implicit pthread_join() is performed.

    BUGS
        Identifier records may be reused, and the initial program task is not
        excluded by an explicit worker-only check. Synchronize lifecycle
        operations and avoid stale identifiers.

    SEE ALSO
        pthread_create()
        pthread_join()
        pthread_attr_setdetachstate()

*****************************************************************************/
{
    ThreadInfo *inf;

    D(bug("%s(%u, %p)\n", __FUNCTION__, thread, value_ptr));

    inf = GetThreadInfo(thread);

    if (inf == NULL || inf->task == NULL)
        return ESRCH;

    ObtainSemaphore(&thread_sem);
    if (inf->detached)
    {
        ReleaseSemaphore(&thread_sem);
        return EINVAL;
    }

    // a thread that already finished waits for a join that will never
    // come: free its slot now, or __pthread_Exit_Func waits forever
    if (inf->finished)
        memset(inf, 0, sizeof(ThreadInfo));
    else
        inf->detached = TRUE;
    ReleaseSemaphore(&thread_sem);

    return 0;
}

/****************************************************************************
    NAME */
        void pthread_testcancel(

/*  SYNOPSIS */
        void)

/*  FUNCTION
        Act on a pending cancellation request at an explicit cancellation
        point.

    RESULT
        Returns no value if cancellation is absent or disabled. If pending and
        enabled, calls pthread_exit(PTHREAD_CANCELED) and does not return.

    NOTES
        A disabled request remains pending; its cancellation wake-up signal is
        cleared. Use from a task registered with the pthread runtime.

    BUGS
        This path dereferences the result of thread lookup without checking
        for an unregistered caller. It cannot safely provide cancellation for
        arbitrary Exec tasks.

    SEE ALSO
        pthread_cancel()
        pthread_setcancelstate()
        pthread_exit()

*****************************************************************************/
{
    pthread_t thread;
    ThreadInfo *inf;

    D(bug("%s()\n", __FUNCTION__));

    thread = pthread_self();
    inf = GetThreadInfo(thread);

    if (inf->canceled)
    {
        if (inf->cancelstate == PTHREAD_CANCEL_ENABLE)
            pthread_exit(PTHREAD_CANCELED);
        // pending but disabled: drop the wake-up pthread_cancel() sent. Only
        // then, or a user's Ctrl-C would be lost at every cancellation point
        SetSignal(0, SIGBREAKF_CTRL_C);
    }
}

static void OnceCleanup(void *arg)
{
    pthread_once_t *once_control;

    DB2(bug("%s(%p)\n", __FUNCTION__, arg));

    once_control = (pthread_once_t *)arg;
    pthread_spin_unlock(&once_control->lock);
}

/****************************************************************************
    NAME */
        int pthread_once(

/*  SYNOPSIS */
        pthread_once_t *once_control, void (*init_routine)(void))

/*  FUNCTION
        Execute an initialization routine at most once per correctly
        initialized control object.

    INPUTS
        once_control - Shared control object initialized with
                       PTHREAD_ONCE_INIT.
        init_routine - Non-NULL callback invoked without arguments when
                       initialization is required.

    RESULT
        0 on the implemented paths; EINVAL if either pointer is NULL.

    NOTES
        Competing callers synchronize through the control object spinlock. The
        initializer must be valid for the lifetime of concurrent callers.

    BUGS
        A zero-initialized control object is not equivalent to
        PTHREAD_ONCE_INIT: the initial compare-and-swap path can skip
        initialization. Cancellation during the callback also has nontrivial
        state and cleanup implications; do not assume a POSIX retry guarantee
        without testing.

    SEE ALSO
        pthread_spin_lock()
        pthread_cleanup_push()
        pthread_cleanup_pop()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, once_control, init_routine));

    if (once_control == NULL || init_routine == NULL)
        return EINVAL;

    if (__sync_val_compare_and_swap(&once_control->started, FALSE, TRUE))
    {
        pthread_spin_lock(&once_control->lock);
        if (!once_control->done)
        {
            pthread_cleanup_push(OnceCleanup, once_control);
            (*init_routine)();
            pthread_cleanup_pop(0);
            once_control->done = TRUE;
        }
        pthread_spin_unlock(&once_control->lock);
    }

    return 0;
}

//
// Scheduling functions
//

/****************************************************************************
    NAME */
        int pthread_setschedprio(

/*  SYNOPSIS */
        pthread_t thread, int prio)

/*  FUNCTION
        Set the Exec task priority associated with a pthread identifier.

    INPUTS
        thread - Existing pthread identifier.
        prio - Requested task priority.

    RESULT
        0 when SetTaskPri() is called; EINVAL if the range comparison rejects
        prio; ESRCH if the identifier is outside the allocated record range.

    NOTES
        The implementation evaluates the value against
        sched_get_priority_max/min(SCHED_NORMAL), then calls SetTaskPri(). It
        does not negotiate a distinct POSIX scheduling class.

    BUGS
        The comparison uses prio < max or prio > min, so actual accepted
        values depend on the ordering returned by those helpers. A record
        within range is not checked for a live task pointer.

    SEE ALSO
        pthread_setschedparam()
        pthread_getschedparam()

*****************************************************************************/
{
    ThreadInfo *inf;

    D(bug("%s(%u, %d)\n", __FUNCTION__, thread, prio));

    if (prio < sched_get_priority_max(SCHED_NORMAL) || prio > sched_get_priority_min(SCHED_NORMAL))
        return EINVAL;

    inf = GetThreadInfo(thread);

    if (inf == NULL)
        return ESRCH;

    SetTaskPri(inf->task, prio);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_setschedparam(

/*  SYNOPSIS */
        pthread_t thread, int policy, const struct sched_param *param)

/*  FUNCTION
        Apply a scheduling priority to the Exec task named by a pthread
        identifier.

    INPUTS
        thread - Allocated pthread identifier.
        policy - Requested POSIX policy; accepted but ignored by this
                 implementation.
        param - Non-NULL struct sched_param containing sched_priority.

    RESULT
        0 after calling SetTaskPri(); EINVAL if param is NULL; ESRCH if thread
        lookup fails.

    NOTES
        Only param->sched_priority is passed to Exec. The function does not
        validate policy or the requested priority.

    BUGS
        A record may exist without a live task. No NULL task check protects
        the SetTaskPri() call.

    SEE ALSO
        pthread_getschedparam()
        pthread_setschedprio()

*****************************************************************************/
{
    ThreadInfo *inf;

    D(bug("%s(%u, %d, %p)\n", __FUNCTION__, thread, policy, param));

    if (param == NULL)
        return EINVAL;

    inf = GetThreadInfo(thread);

    if (inf == NULL)
        return ESRCH;

    SetTaskPri(inf->task, param->sched_priority);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_getschedparam(

/*  SYNOPSIS */
        pthread_t thread, int *policy, struct sched_param *param)

/*  FUNCTION
        Report the current Exec priority for a pthread and a fixed scheduling
        policy.

    INPUTS
        thread - Allocated pthread identifier.
        policy - Non-NULL output; receives SCHED_NORMAL.
        param - Non-NULL output; sched_priority receives the Exec task
                priority.

    RESULT
        0 on success; EINVAL if either output is NULL; ESRCH if thread lookup
        fails.

    NOTES
        Reads tc_Node.ln_Pri directly from the associated task. Other fields
        of *param are not initialized by this function.

    BUGS
        The task pointer is not checked for NULL before dereference. A
        finished or invalidated slot can therefore cause invalid memory
        access.

    SEE ALSO
        pthread_setschedparam()
        pthread_setschedprio()

*****************************************************************************/
{
    ThreadInfo *inf;

    D(bug("%s(%u, %d, %p)\n", __FUNCTION__, thread, policy, param));

    if (param == NULL || policy == NULL)
        return EINVAL;

    inf = GetThreadInfo(thread);

    if (inf == NULL)
        return ESRCH;

    param->sched_priority = inf->task->tc_Node.ln_Pri;
    *policy = SCHED_NORMAL;

    return 0;
}

//
// Non-portable functions
//
/****************************************************************************
    NAME */
        int pthread_setname_np(

/*  SYNOPSIS */
        pthread_t thread, const char *name)

/*  FUNCTION
        Replace the Exec task name associated with a pthread identifier.

    INPUTS
        thread - Registered pthread identifier.
        name - Non-NULL NUL-terminated replacement name.

    RESULT
        0 on success; ERANGE if name is NULL, the identifier cannot be
        located, or the replacement exceeds the available name space.

    NOTES
        For a worker, the name must fit NAMELEN including its terminator. For
        the initial task, the destination capacity is taken from the length of
        its existing name plus one.

    BUGS
        Writes into the existing Exec task name storage rather than allocating
        a new buffer. No thread-lifetime or concurrent name-access
        synchronization is provided.

    SEE ALSO
        pthread_getname_np()
        pthread_self()

*****************************************************************************/
{
    ThreadInfo *inf;
    char *currentname;
    size_t namelen;

    D(bug("%s(%u, %s)\n", __FUNCTION__, thread, name));

    if (name == NULL)
        return ERANGE;

    inf = GetThreadInfo(thread);

    if (inf == NULL)
        return ERANGE;

    currentname = inf->task->tc_Node.ln_Name;

    if (inf->parent == NULL)
        namelen = strlen(currentname) + 1;
    else
        namelen = NAMELEN;

    if (strlen(name) + 1 > namelen)
        return ERANGE;

    strncpy(currentname, name, namelen);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_getname_np(

/*  SYNOPSIS */
        pthread_t thread, char *name, size_t len)

/*  FUNCTION
        Copy the current Exec task name for a pthread into a caller-owned
        buffer.

    INPUTS
        thread - Registered pthread identifier.
        name - Non-NULL writable output buffer.
        len - Capacity of name in bytes, including the trailing NUL.

    RESULT
        0 on success; ERANGE if name is NULL, len is zero, the identifier
        cannot be located, or the destination is too small.

    NOTES
        Copies the NUL-terminated task name after comparing strlen(name)+1
        against len. No destination bytes are written when that size check
        fails.

    BUGS
        The task pointer and source string are not protected against
        concurrent task completion or renaming.

    SEE ALSO
        pthread_setname_np()
        pthread_self()

*****************************************************************************/
{
    ThreadInfo *inf;
    char *currentname;

    D(bug("%s(%u, %p, %u)\n", __FUNCTION__, thread, name, len));

    if (name == NULL || len == 0)
        return ERANGE;

    inf = GetThreadInfo(thread);

    if (inf == NULL)
        return ERANGE;

    currentname = inf->task->tc_Node.ln_Name;

    if (strlen(currentname) + 1 > len)
        return ERANGE;

    // length check passed - strcpy is ok.
    strcpy(name, currentname);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_getattr_np(

/*  SYNOPSIS */
        pthread_t thread, pthread_attr_t *attr)

/*  FUNCTION
        Snapshot the attributes of a registered pthread and its current stack
        extent.

    INPUTS
        thread - Registered pthread identifier.
        attr - Non-NULL writable output attributes.

    RESULT
        0 on success; EINVAL if attr is NULL; ESRCH if thread lookup fails.

    NOTES
        Copies the attributes captured at creation, then replaces stackaddr
        with tc_SPLower and stacksize with tc_SPUpper - tc_SPLower from the
        current Exec task. The result is a snapshot, not a live object.

    BUGS
        No live-task guard or synchronization protects the task stack fields.
        A stale identifier can dereference an invalid task. No atomic snapshot
        against an in-progress stack swap is guaranteed.

    SEE ALSO
        pthread_attr_getstack()
        pthread_create()

*****************************************************************************/
{
    ThreadInfo *inf;

    D(bug("%s(%u, %p)\n", __FUNCTION__, thread, attr));

    if (attr == NULL)
        return EINVAL;

    inf = GetThreadInfo(thread);

    if (inf == NULL)
        return ESRCH; // TODO

    *attr = inf->attr; // Copy information at creation time

    // Add current stack information
    // (can't cache this during creation, as the thread might have executed NewStackSwap afterwards)
    attr->stackaddr = inf->task->tc_SPLower;
    attr->stacksize = inf->task->tc_SPUpper - inf->task->tc_SPLower;

    return 0;
}

//
// Cancellation cleanup
//

/****************************************************************************
    NAME */
        void pthread_cleanup_push(

/*  SYNOPSIS */
        void (*routine)(void *), void *arg)

/*  FUNCTION
        Register a callback to run when the current pthread unwinds its
        cleanup stack.

    INPUTS
        routine - Callback that accepts one void * argument; NULL is silently
                  ignored.
        arg - Pointer passed to the callback; must remain valid until
              invocation or removal.

    RESULT
        No return value. On allocation failure the handler is silently not
        registered.

    NOTES
        Allocates a cleanup record and appends it to the calling thread's
        list. A matching pthread_cleanup_pop() normally removes the most
        recently pushed record. These are implemented as callable functions,
        not lexical pairing macros.

    BUGS
        Allocation failure is not reported. If registration fails, a later pop
        may remove an older handler. Normal return from the thread start
        routine does not run the cleanup list in this implementation.

    SEE ALSO
        pthread_cleanup_pop()
        pthread_exit()
        pthread_testcancel()

*****************************************************************************/
{
    pthread_t thread;
    ThreadInfo *inf;
    CleanupHandler *handler;

    D(bug("%s(%p, %p)\n", __FUNCTION__, routine, arg));

    if (routine == NULL)
        return;

    handler = malloc(sizeof(CleanupHandler));

    if (handler == NULL)
        return;

    thread = pthread_self();
    inf = GetThreadInfo(thread);

    handler->routine = routine;
    handler->arg = arg;
    AddTail((struct List *)&inf->cleanup, (struct Node *)handler);
}

/****************************************************************************
    NAME */
        void pthread_cleanup_pop(

/*  SYNOPSIS */
        int execute)

/*  FUNCTION
        Remove the most recently registered cancellation cleanup handler.

    INPUTS
        execute - Nonzero to invoke the removed handler immediately; zero to
                  discard it without invocation.

    RESULT
        No return value. A missing handler is treated as an empty pop.

    NOTES
        Removes the tail record from the current pthread cleanup list,
        optionally invokes its callback with the stored argument, then frees
        the record.

    BUGS
        No dynamic balancing or thread-identity validation is performed. A
        previous failed push can leave the pop operating on another registered
        handler.

    SEE ALSO
        pthread_cleanup_push()
        pthread_exit()

*****************************************************************************/
{
    pthread_t thread;
    ThreadInfo *inf;
    CleanupHandler *handler;

    D(bug("%s(%d)\n", __FUNCTION__, execute));

    thread = pthread_self();
    inf = GetThreadInfo(thread);
    handler = (CleanupHandler *)RemTail((struct List *)&inf->cleanup);

    if (handler && handler->routine && execute)
        handler->routine(handler->arg);

    free(handler);
}

//
// Signalling
//

/****************************************************************************
    NAME */
        int pthread_kill(

/*  SYNOPSIS */
        pthread_t thread, int sig)

/*  FUNCTION
        Provide the pthread_kill entry point without signal delivery support.

    INPUTS
        thread - Thread identifier; ignored.
        sig - Signal number; ignored.

    RESULT
        Always returns EINVAL. No signal is delivered.

    NOTES
        The present implementation contains only a diagnostic trace and the
        EINVAL return. Do not use it to probe thread existence or deliver
        POSIX signals.

    BUGS
        Signal delivery is not implemented for this entry point.

    SEE ALSO
        pthread_cancel()
        pthread_self()

*****************************************************************************/
{
    D(bug("%s(%u, %d) not implemented\n", __FUNCTION__, thread, sig));

    return EINVAL;
}

//
// Constructors, destructors
//

#ifdef __AROS__
// exit(), abort() or an unhandled signal in a thread: the program's exit code
// has to run in the program's task (stdc.library asks it to break), so only
// this thread ends
static void __pthread_TaskExit(int returncode)
{
    pthread_t thread = GetThreadId(GET_THIS_TASK);

    (void)returncode;
    if (thread != 0 && thread < PTHREAD_THREADS_MAX)
        pthread_exit(PTHREAD_CANCELED);
}
#endif

int __pthread_Init_Func(void)
{
    DB2(bug("%s()\n", __FUNCTION__));

    InitSemaphore(&thread_sem);
    InitSemaphore(&tls_sem);

    // reserve ID 0 for the main thread
    ThreadInfo *inf = &mainthread;
    threads[0] = inf;
    threads_used = 1;

    inf->task = GET_THIS_TASK;

    NEWLIST((struct List *)&inf->cleanup);
#ifdef __AROS__
    __stdc_set_taskexit(__pthread_TaskExit);
#endif
    return TRUE;
}

void __pthread_Exit_Func(void)
{
    pthread_t i;
    ThreadInfo *inf;

    DB2(bug("%s()\n", __FUNCTION__));

    // an earlier init function failed: the exit set still runs, but
    // __pthread_Init_Func never did and thread_sem is not initialised
    if (threads_used == 0)
        return;

    __pthread_exiting = TRUE;
#ifdef __AROS__
    __stdc_set_taskexit(NULL);
#endif

    // if we don't do this we can easily end up with unloaded code being executed
    for (i = 1; i < threads_used; i++)
    {
        inf = threads[i];
        // idle workers never return: cancel them. A finished thread's task
        // is gone; thread_sem keeps a running one from finishing meanwhile
        ObtainSemaphore(&thread_sem);
        if (inf->task && !inf->finished)
            pthread_cancel(i);
        ReleaseSemaphore(&thread_sem);
        // wait for it to end, but leave the join to the program: a thread
        // still tearing down may be about to join it
        while (inf->task && !inf->finished)
            Delay(1);
    }

    // the main thread's slot stays usable for later exit code
    ObtainSemaphore(&thread_sem);
    for (i = 1; i < threads_used; i++)
        FreeVec(threads[i]);
    threads_used = 1;
    ReleaseSemaphore(&thread_sem);

#if defined(__AROS__)
    // Freed slots are not cleared in the tasks that used them, so give
    // back what this task holds before the slot can be handed out again.
    if (tlsslot)
    {
        struct Library *TaskResBase = tlstaskres;
        void **values = (void **)GetTaskStorageSlot(tlsslot);

        if (values)
        {
            SetTaskStorageSlot(tlsslot, 0);
            FreeVec(values);
        }
        FreeTaskStorageSlot(tlsslot);
        tlsslot = 0;
    }
#endif
}

#if defined(__AROS__) || (defined(__AMIGA__) && !defined(__MORPHOS__))
ADD2INIT(__pthread_Init_Func, 0);
ADD2EXIT(__pthread_Exit_Func, 0);
#else
static CONSTRUCTOR_P(__pthread_Init_Func, 100)
{
    return !__pthread_Init_Func();
}

static DESTRUCTOR_P(__pthread_Exit_Func, 100)
{
    __pthread_Exit_Func();
}
#endif
