/*
  Copyright (C) 2014 Szilard Biro

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
#include <aros/symbolsets.h>
#define    TIMESPEC_TO_TIMEVAL(tv, ts) {    \
    (tv)->tv_sec = (ts)->tv_sec;        \
    (tv)->tv_usec = (ts)->tv_nsec / 1000; }
#else
#include <constructor.h>
#define StackSwapArgs PPCStackSwapArgs
#define NewStackSwap NewPPCStackSwap
#endif

#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "pthread.h"
#include "debug.h"

#define SIGB_PARENT SIGBREAKB_CTRL_F
#define SIGF_PARENT (1 << SIGB_PARENT)
#define SIGB_COND_FALLBACK SIGBREAKB_CTRL_E
#define SIGF_COND_FALLBACK (1 << SIGB_COND_FALLBACK)
#define SIGB_TIMER_FALLBACK SIGBREAKB_CTRL_D
#define SIGF_TIMER_FALLBACK (1 << SIGB_TIMER_FALLBACK)

#define NAMELEN 32
#define PTHREAD_FIRST_THREAD_ID (1)
#define PTHREAD_BARRIER_FLAG (1UL << 31)

//#define USE_ASYNC_CANCEL

typedef struct
{
    struct MinNode node;
    struct Task *task;
    ULONG sigmask;
} CondWaiter;

typedef struct
{
    void (*destructor)(void *);
    BOOL used;
} TLSKey;

typedef struct
{
    struct MinNode node;
    void (*routine)(void *);
    void *arg;
} CleanupHandler;

typedef struct
{
    void *(*start)(void *);
    void *arg;
    struct Task *parent;
    int finished;
    struct Task *task;
    void *ret;
    jmp_buf jmp;
    pthread_attr_t attr;
    void *tlsvalues[PTHREAD_KEYS_MAX];
    struct MinList cleanup;
    int cancelstate;
    int canceltype;
    int canceled;
} ThreadInfo;

static ThreadInfo threads[PTHREAD_THREADS_MAX];
static struct SignalSemaphore thread_sem;
static TLSKey tlskeys[PTHREAD_KEYS_MAX];
static struct SignalSemaphore tls_sem;

//
// Helper functions
//

static int SemaphoreIsInvalid(struct SignalSemaphore *sem)
{
    DB2(bug("%s(%p)\n", __FUNCTION__, sem));

    return (!sem || sem->ss_Link.ln_Type != NT_SIGNALSEM || sem->ss_WaitQueue.mlh_Tail != NULL);
}

static int SemaphoreIsMine(struct SignalSemaphore *sem)
{
    struct Task *me;

    DB2(bug("%s(%p)\n", __FUNCTION__, sem));

    me = FindTask(NULL);

    return (sem && sem->ss_NestCount > 0 && sem->ss_Owner == me);
}

static ThreadInfo *GetThreadInfo(pthread_t thread)
{
    ThreadInfo *inf = NULL;

    DB2(bug("%s(%u)\n", __FUNCTION__, thread));

    // TODO: more robust error handling?
    if (thread < PTHREAD_THREADS_MAX)
        inf = &threads[thread];

    return inf;
}

static pthread_t GetThreadId(struct Task *task)
{
    pthread_t i;

    DB2(bug("%s(%p)\n", __FUNCTION__, task));

    ObtainSemaphoreShared(&thread_sem);

    // First thread id will be 1 so that it is different than default value of pthread_t
    for (i = PTHREAD_FIRST_THREAD_ID; i < PTHREAD_THREADS_MAX; i++)
    {
        if (threads[i].task == task)
            break;
    }

    ReleaseSemaphore(&thread_sem);

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

static int __m68k_sync_add_and_fetch(int *v, int n)
{
    int ret;

    Disable();
    (*v) += (n);
    ret = (*v);
    Enable();

    return ret;
}
#undef __sync_add_and_fetch
#define __sync_add_and_fetch(v, n) __m68k_sync_add_and_fetch(v, n)
#undef __sync_sub_and_fetch
#define __sync_sub_and_fetch(v, n) __m68k_sync_add_and_fetch(v, -(n))
#endif

//
// Thread specific data functions
//

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    TLSKey *tls;
    int i;

    D(bug("%s(%p, %p)\n", __FUNCTION__, key, destructor));

    if (key == NULL)
        return EINVAL;

    ObtainSemaphore(&tls_sem);

    for (i = 0; i < PTHREAD_KEYS_MAX; i++)
    {
        if (tlskeys[i].used == FALSE)
            break;
    }

    if (i == PTHREAD_KEYS_MAX)
    {
        ReleaseSemaphore(&tls_sem);
        return EAGAIN;
    }

    tls = &tlskeys[i];
    tls->used = TRUE;
    tls->destructor = destructor;

    ReleaseSemaphore(&tls_sem);

    *key = i;

    return 0;
}

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

int pthread_setspecific(pthread_key_t key, const void *value)
{
    pthread_t thread;
    ThreadInfo *inf;
    TLSKey *tls;

    D(bug("%s(%u)\n", __FUNCTION__, key));

    if (key >= PTHREAD_KEYS_MAX)
        return EINVAL;

    thread = pthread_self();
    tls = &tlskeys[key];

    ObtainSemaphoreShared(&tls_sem);

    if (tls->used == FALSE)
    {
        ReleaseSemaphore(&tls_sem);
        return EINVAL;
    }

    ReleaseSemaphore(&tls_sem);

    inf = GetThreadInfo(thread);
    inf->tlsvalues[key] = (void *)value;

    return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
    pthread_t thread;
    ThreadInfo *inf;
    void *value = NULL;

    D(bug("%s(%u)\n", __FUNCTION__, key));

    if (key >= PTHREAD_KEYS_MAX)
        return NULL;

    thread = pthread_self();
    inf = GetThreadInfo(thread);
    value = inf->tlsvalues[key];

    return value;
}

//
// Mutex attribute functions
//

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    attr->kind = PTHREAD_MUTEX_DEFAULT;

    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_mutexattr_t));

    return 0;
}

int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *kind)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, attr, kind));

    if (attr == NULL)
        return EINVAL;

    if (kind)
        *kind = attr->kind;

    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL || !(kind >= PTHREAD_MUTEX_NORMAL && kind <= PTHREAD_MUTEX_ERRORCHECK))
        return EINVAL;

    attr->kind = kind;

    return 0;
}

//
// Mutex functions
//

static int _pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr, BOOL staticinit)
{
    DB2(bug("%s(%p, %p)\n", __FUNCTION__, mutex, attr));

    if (mutex == NULL)
        return EINVAL;

    if (attr)
        mutex->kind = attr->kind;
    else if (!staticinit)
        mutex->kind = PTHREAD_MUTEX_DEFAULT;
    InitSemaphore(&mutex->semaphore);
    mutex->incond = 0;

    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, mutex, attr));

    return _pthread_mutex_init(mutex, attr, FALSE);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // probably a statically allocated mutex
    if (SemaphoreIsInvalid(&mutex->semaphore))
        return 0;

    if (/*mutex->incond ||*/ AttemptSemaphore(&mutex->semaphore) == FALSE)
        return EBUSY;

    if (mutex->incond)
    {
        ReleaseSemaphore(&mutex->semaphore);
        return EBUSY;
    }

    ReleaseSemaphore(&mutex->semaphore);
    memset(mutex, 0, sizeof(pthread_mutex_t));

    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // initialize static mutexes
    if (SemaphoreIsInvalid(&mutex->semaphore))
        _pthread_mutex_init(mutex, NULL, TRUE);

    // normal mutexes would simply deadlock here
    if (mutex->kind == PTHREAD_MUTEX_ERRORCHECK && SemaphoreIsMine(&mutex->semaphore))
        return EDEADLK;

    ObtainSemaphore(&mutex->semaphore);

    return 0;
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)
{
    struct timeval end, now;
    int result;

    D(bug("%s(%p, %p)\n", __FUNCTION__, mutex, abstime));

    if (mutex == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_mutex_lock(mutex); 
    /*else if (abstime.tv_nsec < 0 || abstime.tv_nsec >= 1000000000)
        return EINVAL;*/

    TIMESPEC_TO_TIMEVAL(&end, abstime);

    // busy waiting is not very nice, but ObtainSemaphore doesn't support timeouts
    while ((result = pthread_mutex_trylock(mutex)) == EBUSY)
    {
        sched_yield();
        gettimeofday(&now, NULL);
        if (timercmp(&end, &now, <))
            return ETIMEDOUT;
    }

    return result;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    ULONG ret;

    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // initialize static mutexes
    if (SemaphoreIsInvalid(&mutex->semaphore))
        _pthread_mutex_init(mutex, NULL, TRUE);

    if (mutex->kind != PTHREAD_MUTEX_RECURSIVE && SemaphoreIsMine(&mutex->semaphore))
        return EBUSY;

    ret = AttemptSemaphore(&mutex->semaphore);

    return (ret == TRUE) ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // initialize static mutexes
    if (SemaphoreIsInvalid(&mutex->semaphore))
        _pthread_mutex_init(mutex, NULL, TRUE);

    if (mutex->kind != PTHREAD_MUTEX_NORMAL && !SemaphoreIsMine(&mutex->semaphore))
        return EPERM;

    ReleaseSemaphore(&mutex->semaphore);

    return 0;
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

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, cond, attr));

    if (cond == NULL)
        return EINVAL;

    InitSemaphore(&cond->semaphore);
    NEWLIST((struct List *)&cond->waiters);

    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    if (cond == NULL)
        return EINVAL;

    // probably a statically allocated condition
    if (SemaphoreIsInvalid(&cond->semaphore))
        return 0;

    if (AttemptSemaphore(&cond->semaphore) == FALSE)
        return EBUSY;

    if (!IsListEmpty((struct List *)&cond->waiters))
    {
        ReleaseSemaphore(&cond->semaphore);
        return EBUSY;
    }

    ReleaseSemaphore(&cond->semaphore);
    memset(cond, 0, sizeof(pthread_cond_t));

    return 0;
}

static int _pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime, BOOL relative)
{
    CondWaiter waiter;
    BYTE signal;
    ULONG sigs = 0;
    ULONG timermask = 0;
    struct MsgPort timermp;
    struct timerequest timerio;
    struct Task *task;

    DB2(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, abstime));

    if (cond == NULL || mutex == NULL)
        return EINVAL;

    pthread_testcancel();

    // initialize static conditions
    if (SemaphoreIsInvalid(&cond->semaphore))
        pthread_cond_init(cond, NULL);

    task = FindTask(NULL);

    if (abstime)
    {
        // prepare MsgPort
        timermp.mp_Node.ln_Type = NT_MSGPORT;
        timermp.mp_Node.ln_Pri = 0;
        timermp.mp_Node.ln_Name = NULL;
        timermp.mp_Flags = PA_SIGNAL;
        timermp.mp_SigTask = task;
        signal = AllocSignal(-1);
        if (signal == -1)
        {
            signal = SIGB_TIMER_FALLBACK;
            SetSignal(SIGF_TIMER_FALLBACK, 0);
        }
        timermp.mp_SigBit = signal;
        NEWLIST(&timermp.mp_MsgList);

        // prepare IORequest
        timerio.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        timerio.tr_node.io_Message.mn_Node.ln_Pri = 0;
        timerio.tr_node.io_Message.mn_Node.ln_Name = NULL;
        timerio.tr_node.io_Message.mn_ReplyPort = &timermp;
        timerio.tr_node.io_Message.mn_Length = sizeof(struct timerequest);

        // open timer.device
        if (OpenDevice((STRPTR)TIMERNAME, UNIT_MICROHZ, &timerio.tr_node, 0) != 0)
        {
            if (timermp.mp_SigBit != SIGB_TIMER_FALLBACK)
                FreeSignal(timermp.mp_SigBit);

            return EINVAL;
        }

        // prepare the device command and send it
        timerio.tr_node.io_Command = TR_ADDREQUEST;
        timerio.tr_node.io_Flags = 0;
        TIMESPEC_TO_TIMEVAL(&timerio.tr_time, abstime);
        if (!relative)
        {
            struct timeval starttime;
            // absolute time has to be converted to relative
            // GetSysTime can't be used due to the timezone offset in abstime
            gettimeofday(&starttime, NULL);
            timersub(&timerio.tr_time, &starttime, &timerio.tr_time);
        }
        timermask = 1 << timermp.mp_SigBit;
        sigs |= timermask;
        SendIO((struct IORequest *)&timerio);
    }

    // prepare a waiter node
    waiter.task = task;
    signal = AllocSignal(-1);
    if (signal == -1)
    {
        signal = SIGB_COND_FALLBACK;
        SetSignal(SIGF_COND_FALLBACK, 0);
    }
    waiter.sigmask = 1 << signal;
    sigs |= waiter.sigmask;

    // add it to the end of the list
    ObtainSemaphore(&cond->semaphore);
    AddTail((struct List *)&cond->waiters, (struct Node *)&waiter);
    ReleaseSemaphore(&cond->semaphore);

    // wait for the condition to be signalled or the timeout
    mutex->incond++;
    pthread_mutex_unlock(mutex);
    sigs = Wait(sigs);
    pthread_mutex_lock(mutex);
    mutex->incond--;

    // remove the node from the list
    ObtainSemaphore(&cond->semaphore);
    Remove((struct Node *)&waiter);
    ReleaseSemaphore(&cond->semaphore);

    if (signal != SIGB_COND_FALLBACK)
        FreeSignal(signal);

    if (abstime)
    {
        // clean up the timerequest
        if (!CheckIO((struct IORequest *)&timerio))
        {
            AbortIO((struct IORequest *)&timerio);
            WaitIO((struct IORequest *)&timerio);
        }
        CloseDevice((struct IORequest *)&timerio);

        if (timermp.mp_SigBit != SIGB_TIMER_FALLBACK)
            FreeSignal(timermp.mp_SigBit);

        // did we timeout?
        if (sigs & timermask)
            return ETIMEDOUT;
    }

    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, abstime));

    return _pthread_cond_timedwait(cond, mutex, abstime, FALSE);
}

int pthread_cond_timedwait_relative_np(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *reltime)
{
    D(bug("%s(%p, %p, %p)\n", __FUNCTION__, cond, mutex, reltime));

    return _pthread_cond_timedwait(cond, mutex, reltime, TRUE);
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    return _pthread_cond_timedwait(cond, mutex, NULL, FALSE);
}

static int _pthread_cond_broadcast(pthread_cond_t *cond, BOOL onlyfirst)
{
    CondWaiter *waiter;

    DB2(bug("%s(%p, %d)\n", __FUNCTION__, cond, onlyfirst));

    if (cond == NULL)
        return EINVAL;

    // initialize static conditions
    if (SemaphoreIsInvalid(&cond->semaphore))
        pthread_cond_init(cond, NULL);

    // signal the waiting threads 
    ObtainSemaphore(&cond->semaphore);
    ForeachNode(&cond->waiters, waiter)
    {
        Signal(waiter->task, waiter->sigmask);
        if (onlyfirst) break;
    }
    ReleaseSemaphore(&cond->semaphore);

    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    return _pthread_cond_broadcast(cond, TRUE);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    return _pthread_cond_broadcast(cond, FALSE);
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

    // we might already have a write lock
    if (SemaphoreIsMine(&lock->semaphore))
        return EDEADLK;

    ObtainSemaphoreShared(&lock->semaphore);

    return 0;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *lock, const struct timespec *abstime)
{
    struct timeval end, now;
    int result;

    D(bug("%s(%p, %p)\n", __FUNCTION__, lock, abstime));

    if (lock == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_rwlock_rdlock(lock);

    pthread_testcancel();

    TIMESPEC_TO_TIMEVAL(&end, abstime);

    // busy waiting is not very nice, but ObtainSemaphore doesn't support timeouts
    while ((result = pthread_rwlock_tryrdlock(lock)) == EBUSY)
    {
        sched_yield();
        gettimeofday(&now, NULL);
        if (timercmp(&end, &now, <))
            return ETIMEDOUT;
    }

    return result;
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
    struct timeval end, now;
    int result;

    D(bug("%s(%p, %p)\n", __FUNCTION__, lock, abstime));

    if (lock == NULL)
        return EINVAL;

    if (abstime == NULL)
        return pthread_rwlock_wrlock(lock);

    pthread_testcancel();

    TIMESPEC_TO_TIMEVAL(&end, abstime);

    // busy waiting is not very nice, but ObtainSemaphore doesn't support timeouts
    while ((result = pthread_rwlock_trywrlock(lock)) == EBUSY)
    {
        sched_yield();
        gettimeofday(&now, NULL);
        if (timercmp(&end, &now, <))
            return ETIMEDOUT;
    }

    return result;
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

    while (__sync_lock_test_and_set((int *)lock, 1))
        sched_yield(); // TODO: don't yield the CPU every iteration

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

int pthread_attr_init(pthread_attr_t *attr)
{
    struct Task *task;

    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_attr_t));
    // inherit the priority and stack size of the parent thread
    task = FindTask(NULL);
    attr->param.sched_priority = task->tc_Node.ln_Pri;
    attr->stacksize = (UBYTE *)task->tc_SPUpper - (UBYTE *)task->tc_SPLower;

    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_attr_t));

    return 0;
}

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

    if (attr == NULL || detachstate != PTHREAD_CREATE_JOINABLE)
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

static void StarterFunc(void)
{
    ThreadInfo *inf;
    int i, j;
    int foundkey = TRUE;
#ifdef USE_ASYNC_CANCEL
    APTR oldexcept;
#endif

    DB2(bug("%s()\n", __FUNCTION__));

    inf = (ThreadInfo *)FindTask(NULL)->tc_UserData;
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

    // set a jump point for pthread_exit
    if (!setjmp(inf->jmp))
    {
        // custom stack requires special handling
        if (inf->attr.stackaddr != NULL && inf->attr.stacksize > 0)
        {
            struct StackSwapArgs swapargs;
            struct StackSwapStruct stack; 

            swapargs.Args[0] = (IPTR)inf->arg;
            stack.stk_Lower = inf->attr.stackaddr;
            stack.stk_Upper = (APTR)((IPTR)stack.stk_Lower + inf->attr.stacksize);
            stack.stk_Pointer = stack.stk_Upper; 

            inf->ret = (void *)NewStackSwap(&stack, inf->start, &swapargs);
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
    ObtainSemaphoreShared(&tls_sem);
    for (j = 0; foundkey && j < PTHREAD_DESTRUCTOR_ITERATIONS; j++)
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

    // tell the parent thread that we are done
    Forbid();
    inf->finished = TRUE;
    Signal(inf->parent, SIGF_PARENT);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg)
{
    ThreadInfo *inf;
    char name[NAMELEN];
    size_t oldlen;
    pthread_t threadnew;

    D(bug("%s(%p, %p, %p, %p)\n", __FUNCTION__, thread, attr, start, arg));

    if (thread == NULL || start == NULL)
        return EINVAL;

    ObtainSemaphore(&thread_sem);

    // grab an empty thread slot
    threadnew = GetThreadId(NULL);
    if (threadnew == PTHREAD_THREADS_MAX)
    {
        ReleaseSemaphore(&thread_sem);
        return EAGAIN;
    }

    // prepare the ThreadInfo structure
    inf = GetThreadInfo(threadnew);
    memset(inf, 0, sizeof(ThreadInfo));
    inf->start = start;
    inf->arg = arg;
    inf->parent = FindTask(NULL);
    if (attr)
        inf->attr = *attr;
    else
        pthread_attr_init(&inf->attr);
    NEWLIST((struct List *)&inf->cleanup);
    inf->cancelstate = PTHREAD_CANCEL_ENABLE;
    inf->canceltype = PTHREAD_CANCEL_DEFERRED;

    // let's trick CreateNewProc into allocating a larger buffer for the name
    snprintf(name, sizeof(name), "pthread thread #%d", threadnew);
    oldlen = strlen(name);
    memset(name + oldlen, ' ', sizeof(name) - oldlen - 1);
    name[sizeof(name) - 1] = '\0';

    // start the child thread
    inf->task = (struct Task *)CreateNewProcTags(NP_Entry, StarterFunc,
#ifdef __MORPHOS__
        NP_CodeType, CODETYPE_PPC,
        (inf->attr.stackaddr == NULL && inf->attr.stacksize > 0) ? NP_PPCStackSize : TAG_IGNORE, inf->attr.stacksize,
#else
        (inf->attr.stackaddr == NULL && inf->attr.stacksize > 0) ? NP_StackSize : TAG_IGNORE, inf->attr.stacksize,
#endif
        NP_UserData, inf,
        NP_Name, name,
        TAG_DONE);

    if (!inf->task)
    {
        inf->parent = NULL;
        ReleaseSemaphore(&thread_sem);
        return EAGAIN;
    }

    ReleaseSemaphore(&thread_sem);

    *thread = threadnew;

    return 0;
}

int pthread_detach(pthread_t thread)
{
    D(bug("%s(%u) not implemented\n", __FUNCTION__, thread));

    return ESRCH;
}

int pthread_join(pthread_t thread, void **value_ptr)
{
    ThreadInfo *inf;

    D(bug("%s(%u, %p)\n", __FUNCTION__, thread, value_ptr));

    inf = GetThreadInfo(thread);

    if (inf == NULL || inf->parent == NULL)
        return ESRCH;

    pthread_testcancel();

    while (!inf->finished)
        Wait(SIGF_PARENT);

    if (value_ptr)
        *value_ptr = inf->ret;

    ObtainSemaphore(&thread_sem);
    memset(inf, 0, sizeof(ThreadInfo));
    ReleaseSemaphore(&thread_sem);

    return 0;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    D(bug("%s(%u, %u)\n", __FUNCTION__, t1, t2));

    return (t1 == t2);
}

pthread_t pthread_self(void)
{
    struct Task *task;
    pthread_t thread;

    D(bug("%s()\n", __FUNCTION__));

    task = FindTask(NULL);
    thread = GetThreadId(task);

    // add non-pthread processes to our list, so we can handle the main thread
    if (thread == PTHREAD_THREADS_MAX)
    {
        ThreadInfo *inf;

        ObtainSemaphore(&thread_sem);
        thread = GetThreadId(NULL);
        if (thread == PTHREAD_THREADS_MAX)
        {
            // TODO: pthread_self is supposed to always succeed, but we can fail
            // here if we run out of thread slots
            // this can only happen if too many non-pthread processes call
            // this function
            //ReleaseSemaphore(&thread_sem);
            //return EAGAIN;
            abort();
        }
        inf = GetThreadInfo(thread);
        memset(inf, 0, sizeof(ThreadInfo));
        NEWLIST((struct List *)&inf->cleanup);
        inf->task = task;
        ReleaseSemaphore(&thread_sem);
    }

    return thread;
}

int pthread_cancel(pthread_t thread)
{
    ThreadInfo *inf;

    D(bug("%s(%u)\n", __FUNCTION__, thread));

    inf = GetThreadInfo(thread);

    if (inf == NULL || inf->parent == NULL || inf->canceled == TRUE)
        return ESRCH;

    inf->canceled = TRUE;

    // we might have to cancel the thread immediately
    if (inf->canceltype == PTHREAD_CANCEL_ASYNCHRONOUS && inf->cancelstate == PTHREAD_CANCEL_ENABLE)
    {
        struct Task *task;

        task = FindTask(NULL);

        if (inf->task == task)
            pthread_testcancel(); // cancel ourselves
        else
            Signal(inf->task, SIGBREAKF_CTRL_C); // trigger the exception handler 
    }

    return 0;
}

int pthread_setcancelstate(int state, int *oldstate)
{
    pthread_t thread;
    ThreadInfo *inf;

    D(bug("%s(%d, %p)\n", __FUNCTION__, state, oldstate));

    if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE)
        return EINVAL;

    thread = pthread_self();
    inf = GetThreadInfo(thread);

    if (oldstate)
        *oldstate = inf->cancelstate;

    inf->cancelstate = state;

    return 0;
}

int pthread_setcanceltype(int type, int *oldtype)
{
    pthread_t thread;
    ThreadInfo *inf;

    D(bug("%s(%d, %p)\n", __FUNCTION__, type, oldtype));

    if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS)
        return EINVAL;

    thread = pthread_self();
    inf = GetThreadInfo(thread);

    if (oldtype)
        *oldtype = inf->canceltype;

    inf->canceltype = type;

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
}

void pthread_exit(void *value_ptr)
{
    pthread_t thread;
    ThreadInfo *inf;
    CleanupHandler *handler;

    D(bug("%s(%p)\n", __FUNCTION__, value_ptr));

    thread = pthread_self();
    inf = GetThreadInfo(thread);
    inf->ret = value_ptr;

    // execute the clean-up handlers
    while ((handler = (CleanupHandler *)RemTail((struct List *)&inf->cleanup)))
        if (handler->routine)
            handler->routine(handler->arg);

    longjmp(inf->jmp, 1);
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

    currentname = GetNodeName(inf->task);

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

    currentname = GetNodeName(inf->task);

    if (strlen(currentname) + 1 > len)
        return ERANGE;

    // TODO: partially copy the name?
    strncpy(name, currentname, len);

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

static int _Init_Func(void)
{
    DB2(bug("%s()\n", __FUNCTION__));

    //memset(&threads, 0, sizeof(threads));
    InitSemaphore(&thread_sem);
    InitSemaphore(&tls_sem);
    // reserve ID 0 for the main thread
    //pthread_self();

    return TRUE;
}

static void _Exit_Func(void)
{
#if 0
    pthread_t i;
#endif

    DB2(bug("%s()\n", __FUNCTION__));

    // wait for the threads?
#if 0
    for (i = 0; i < PTHREAD_THREADS_MAX; i++)
        pthread_join(i, NULL);
#endif
}

#ifdef __AROS__
ADD2INIT(_Init_Func, 0);
ADD2EXIT(_Exit_Func, 0);
#else
static CONSTRUCTOR_P(_Init_Func, 100)
{
    return !_Init_Func();
}

static DESTRUCTOR_P(_Exit_Func, 100)
{
    _Exit_Func();
}
#endif
