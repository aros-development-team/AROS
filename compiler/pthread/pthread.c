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

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "pthread_intern.h"
#include "debug.h"


//#define USE_ASYNC_CANCEL

ThreadInfo threads[PTHREAD_THREADS_MAX];
struct SignalSemaphore thread_sem;
TLSKey tlskeys[PTHREAD_KEYS_MAX];
struct SignalSemaphore tls_sem;

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
    if (thread < PTHREAD_THREADS_MAX)
        return &threads[thread];

    return 0;
}

pthread_t GetThreadId(struct Task *task)
{
    pthread_t i;

    DB2(bug("%s(%p)\n", __FUNCTION__, task));

    // 0 is main task, First thread id will be 1 so that it is different than default value of pthread_t
    for (i = 0; i < PTHREAD_THREADS_MAX; i++)
    {
        // be sure not to select existing, "not joined" thread slot with the exec task pointer same as
        // this one (if a new exec task structure get allocated exactly at the same addres as the old one)
        if (threads[i].task == task && !threads[i].finished)
            break;
    }

    return i;
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
    Disable();
    (*v) = (n);
    Enable();

    return n;
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

int pthread_key_delete(pthread_key_t key)
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

int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *kind)
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
        timersub(&tvabstime, &starttime, &tvabstime);
        if (!timerisset(&tvabstime))
        {
            CloseTimerDevice((struct IORequest *)&timerio);
            return ETIMEDOUT;
        }
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

int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)
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

int pthread_condattr_init(pthread_condattr_t *attr)
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_condattr_t));

    return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
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

int pthread_cond_timedwait_relative_np(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *reltime)
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, reltime));

    return _pthread_cond_timedwait(cond, mutex, reltime, TRUE);
}


//
// Barrier functions
//

int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
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

int pthread_barrier_destroy(pthread_barrier_t *barrier)
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

int pthread_barrier_wait(pthread_barrier_t *barrier)
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

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_rwlockattr_t));

    return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
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

int pthread_rwlock_init(pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, lock, attr));

    if (lock == NULL)
        return EINVAL;

    InitSemaphore(&lock->semaphore);

    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *lock)
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

int pthread_rwlock_tryrdlock(pthread_rwlock_t *lock)
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

int pthread_rwlock_trywrlock(pthread_rwlock_t *lock)
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

int pthread_rwlock_rdlock(pthread_rwlock_t *lock)
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

int pthread_rwlock_timedrdlock(pthread_rwlock_t *lock, const struct timespec *abstime)
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

int pthread_rwlock_wrlock(pthread_rwlock_t *lock)
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

int pthread_rwlock_timedwrlock(pthread_rwlock_t *lock, const struct timespec *abstime)
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

int pthread_rwlock_unlock(pthread_rwlock_t *lock)
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

int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    D(bug("%s(%p, %d)\n", __FUNCTION__, lock, pshared));

    if (lock == NULL)
        return EINVAL;

    *lock = 0;

    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
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

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    D(bug("%s(%p)\n", __FUNCTION__, lock));

    if (lock == NULL)
        return EINVAL;

    if (__sync_lock_test_and_set((int *)lock, 1))
        return EBUSY;

    return 0;
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
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

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, detachstate));

    if (attr == NULL)
        return EINVAL;

    if (detachstate != NULL)
        *detachstate = attr->detachstate;

    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    D(bug("%s(%p, %d)\n", __FUNCTION__, attr, detachstate));

    if (attr == NULL || (detachstate != PTHREAD_CREATE_JOINABLE && detachstate != PTHREAD_CREATE_DETACHED))
        return EINVAL;

    attr->detachstate = detachstate;

    return 0;
}

int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize)
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

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize)
{
    D(bug("%s(%p, %p, %u)\n", __FUNCTION__, attr, stackaddr, stacksize));

    if (attr == NULL || (stackaddr != NULL && stacksize == 0))
        return EINVAL;

    attr->stackaddr = stackaddr;
    attr->stacksize = stacksize;

    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, stacksize));

    return pthread_attr_getstack(attr, NULL, stacksize);
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    D(bug("%s(%p, %u)\n", __FUNCTION__, attr, stacksize));

    return pthread_attr_setstack(attr, NULL, stacksize);
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, param));

    if (attr == NULL)
        return EINVAL;

    if (param != NULL)
        *param = attr->param;

    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
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

int pthread_detach(pthread_t thread)
{
    ThreadInfo *inf;

    D(bug("%s(%u, %p)\n", __FUNCTION__, thread, value_ptr));

    inf = GetThreadInfo(thread);

    if (inf == NULL || inf->task == NULL)
        return ESRCH;

    if (inf->detached)
        return EINVAL;

    inf->detached = TRUE;

    return 0;
}

void pthread_testcancel(void)
{
    pthread_t thread;
    ThreadInfo *inf;

    D(bug("%s()\n", __FUNCTION__));

    thread = pthread_self();
    inf = GetThreadInfo(thread);

    if (inf->canceled && (inf->cancelstate == PTHREAD_CANCEL_ENABLE))
        pthread_exit(PTHREAD_CANCELED);

    SetSignal(SIGBREAKF_CTRL_C, 0);
}

static void OnceCleanup(void *arg)
{
    pthread_once_t *once_control;

    DB2(bug("%s(%p)\n", __FUNCTION__, arg));

    once_control = (pthread_once_t *)arg;
    pthread_spin_unlock(&once_control->lock);
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
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

int pthread_setschedprio(pthread_t thread, int prio)
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

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param)
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

int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
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
int pthread_setname_np(pthread_t thread, const char *name)
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

int pthread_getname_np(pthread_t thread, char *name, size_t len)
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

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr)
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

void pthread_cleanup_push(void (*routine)(void *), void *arg)
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

void pthread_cleanup_pop(int execute)
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

int pthread_kill(pthread_t thread, int sig)
{
    D(bug("%s(%u, %d) not implemented\n", __FUNCTION__, thread, sig));

    return EINVAL;
}

//
// Constructors, destructors
//

int __pthread_Init_Func(void)
{
    DB2(bug("%s()\n", __FUNCTION__));

    //memset(&threads, 0, sizeof(threads));
    InitSemaphore(&thread_sem);
    InitSemaphore(&tls_sem);

    // reserve ID 0 for the main thread
    ThreadInfo *inf = &threads[0];

    inf->task = GET_THIS_TASK;

    NEWLIST((struct List *)&inf->cleanup);
    return TRUE;
}

void __pthread_Exit_Func(void)
{
    pthread_t i;
    ThreadInfo *inf;

    DB2(bug("%s()\n", __FUNCTION__));

    // if we don't do this we can easily end up with unloaded code being executed
    for (i = 1; i < PTHREAD_THREADS_MAX; i++)
    {
        inf = &threads[i];
        if (inf->detached)
        {
            D(bug("waiting for detached thread %d\n", i));
            // TODO longer delay between retries?
            while (inf->task)
                Delay(1);
        }
        else
        {
            pthread_join(i, NULL);
        }
    }
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
