#include "native_event_worker.h"

#include <limits.h>
#include <string.h>

#ifdef __AROS__
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>
#else
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#endif

struct worker_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct CAMDNativeEventWorker {
    struct worker_lock lock;
    struct CAMDNativeEventPump *pump;
    uint32_t item_budget;
    uint32_t state;
    uint64_t wake_requests;
    uint64_t pump_runs;
    uint64_t processed_items;
    uint64_t processed_records;
    uint64_t blocked_runs;
    uint64_t downstream_failures;
    int stop_requested;
    int exited;
    int stop_completed;
    uint32_t retry_after_ms;    /* from camd_native_worker_wake_after() */
#ifdef __AROS__
    struct Process *process;
    LONG wake_signal;
#else
    pthread_cond_t condition;
    pthread_t thread;
    int thread_created;
    int wake_pending;
#endif
};

static void counter_add(uint64_t *counter, uint64_t amount)
{
    if (UINT64_MAX - *counter < amount)
        *counter = UINT64_MAX;
    else
        *counter += amount;
}

static void *worker_alloc(size_t size)
{
#ifdef __AROS__
    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, MEMF_ANY | MEMF_CLEAR);
#else
    return calloc(1, size);
#endif
}

static void worker_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int lock_init(struct worker_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore);
    return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}

static void lock_destroy(struct worker_lock *lock)
{
#ifdef __AROS__
    (void)lock;
#else
    pthread_mutex_destroy(&lock->mutex);
#endif
}

static void lock_acquire(struct worker_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}

static void lock_release(struct worker_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
#endif
}

static int pump_until_wait(struct CAMDNativeEventWorker *worker)
{
    for (;;) {
        enum CAMDNativePumpResult result;
        enum CAMDProviderResult downstream;
        size_t items, records;

        lock_acquire(&worker->lock);
        worker->state = CAMD_NATIVE_WORKER_RUNNING;
        lock_release(&worker->lock);
        result = camd_native_pump_run(worker->pump, worker->item_budget,
                                      &items, &records, &downstream);
        lock_acquire(&worker->lock);
        counter_add(&worker->pump_runs, 1);
        counter_add(&worker->processed_items, items);
        counter_add(&worker->processed_records, records);
        if (result == CAMD_NATIVE_PUMP_BLOCKED)
            counter_add(&worker->blocked_runs, 1);
        else if (result == CAMD_NATIVE_PUMP_DOWNSTREAM)
            counter_add(&worker->downstream_failures, 1);
        worker->state = CAMD_NATIVE_WORKER_IDLE;
        if (worker->stop_requested) {
            lock_release(&worker->lock);
            return 1;
        }
        lock_release(&worker->lock);
        if (result == CAMD_NATIVE_PUMP_OK)
            continue;
        if (result == CAMD_NATIVE_PUMP_EMPTY ||
            result == CAMD_NATIVE_PUMP_BLOCKED ||
            result == CAMD_NATIVE_PUMP_DOWNSTREAM ||
            result == CAMD_NATIVE_PUMP_BUSY)
            return 1;
        return 0;
    }
}

#ifdef __AROS__
static void worker_entry(void)
{
    struct CAMDNativeEventWorker *worker = FindTask(NULL)->tc_UserData;
    LONG signal_number = AllocSignal(-1);
    struct MsgPort *timer_port = CreateMsgPort();
    struct timerequest *timer = NULL;
    BOOL timer_open = FALSE;

    /* The timer is for camd_native_worker_wake_after(). */
    if (timer_port)
        timer = (struct timerequest *)CreateIORequest(timer_port,
                                                      sizeof(*timer));
    if (timer &&
        OpenDevice((CONST_STRPTR)"timer.device", UNIT_MICROHZ,
                   (struct IORequest *)timer, 0) == 0)
        timer_open = TRUE;
    lock_acquire(&worker->lock);
    if (signal_number == -1 || !timer_open) {
        worker->state = CAMD_NATIVE_WORKER_FAILED;
        worker->exited = 1;
        lock_release(&worker->lock);
        if (timer_open)
            CloseDevice((struct IORequest *)timer);
        if (timer)
            DeleteIORequest((struct IORequest *)timer);
        if (timer_port)
            DeleteMsgPort(timer_port);
        if (signal_number != -1)
            FreeSignal(signal_number);
        return;
    }
    worker->wake_signal = signal_number;
    worker->state = CAMD_NATIVE_WORKER_IDLE;
    lock_release(&worker->lock);
    for (;;) {
        ULONG wake_mask = 1UL << signal_number;
        ULONG timer_mask = 1UL << timer_port->mp_SigBit;
        ULONG signals;
        uint32_t retry;

        lock_acquire(&worker->lock);
        retry = worker->retry_after_ms;
        worker->retry_after_ms = 0;
        lock_release(&worker->lock);
        if (retry != 0) {
            timer->tr_node.io_Command = TR_ADDREQUEST;
            timer->tr_time.tv_secs = retry / 1000;
            timer->tr_time.tv_micro = (retry % 1000) * 1000;
            SendIO((struct IORequest *)timer);
            signals = Wait(wake_mask | timer_mask | SIGBREAKF_CTRL_C);
            if (!CheckIO((struct IORequest *)timer))
                AbortIO((struct IORequest *)timer);
            WaitIO((struct IORequest *)timer);
            SetSignal(0, timer_mask);
            /* Its time has come: run the pump as for a wake. */
            if (signals & timer_mask)
                signals |= wake_mask;
        } else {
            signals = Wait(wake_mask | SIGBREAKF_CTRL_C);
        }

        if (signals & SIGBREAKF_CTRL_C)
            break;
        if ((signals & (1UL << signal_number)) && !pump_until_wait(worker)) {
            lock_acquire(&worker->lock);
            worker->state = CAMD_NATIVE_WORKER_FAILED;
            lock_release(&worker->lock);
            break;
        }
        lock_acquire(&worker->lock);
        if (worker->stop_requested) {
            lock_release(&worker->lock);
            break;
        }
        lock_release(&worker->lock);
    }
    CloseDevice((struct IORequest *)timer);
    DeleteIORequest((struct IORequest *)timer);
    DeleteMsgPort(timer_port);
    FreeSignal(signal_number);
    lock_acquire(&worker->lock);
    if (worker->state != CAMD_NATIVE_WORKER_FAILED)
        worker->state = CAMD_NATIVE_WORKER_STOPPED;
    worker->exited = 1;
    lock_release(&worker->lock);
}
#else
static void *worker_entry(void *opaque)
{
    struct CAMDNativeEventWorker *worker = opaque;

    lock_acquire(&worker->lock);
    worker->state = CAMD_NATIVE_WORKER_IDLE;
    for (;;) {
        while (!worker->wake_pending && !worker->stop_requested) {
            struct timespec until;
            uint32_t retry = worker->retry_after_ms;

            worker->retry_after_ms = 0;
            if (retry == 0) {
                pthread_cond_wait(&worker->condition, &worker->lock.mutex);
                continue;
            }
            clock_gettime(CLOCK_REALTIME, &until);
            until.tv_sec += retry / 1000;
            until.tv_nsec += (long)(retry % 1000) * 1000000L;
            if (until.tv_nsec >= 1000000000L) {
                until.tv_nsec -= 1000000000L;
                ++until.tv_sec;
            }
            /* Its time has come: run the pump as for a wake. */
            if (pthread_cond_timedwait(&worker->condition,
                                       &worker->lock.mutex, &until) != 0)
                worker->wake_pending = 1;
        }
        if (worker->stop_requested)
            break;
        worker->wake_pending = 0;
        lock_release(&worker->lock);
        if (!pump_until_wait(worker)) {
            lock_acquire(&worker->lock);
            worker->state = CAMD_NATIVE_WORKER_FAILED;
            worker->exited = 1;
            lock_release(&worker->lock);
            return NULL;
        }
        lock_acquire(&worker->lock);
    }
    worker->state = CAMD_NATIVE_WORKER_STOPPED;
    worker->exited = 1;
    lock_release(&worker->lock);
    return NULL;
}
#endif

enum CAMDNativeWorkerResult camd_native_worker_create(
    struct CAMDNativeEventPump *pump,
    const struct CAMDNativeWorkerConfigV1 *config,
    struct CAMDNativeEventWorker **worker_out)
{
    struct CAMDNativeEventWorker *worker;

    if (!worker_out)
        return CAMD_NATIVE_WORKER_INVALID;
    *worker_out = NULL;
    if (!pump || !config || config->Size != sizeof(*config) ||
        config->Version != 1 || config->ItemBudget == 0)
        return CAMD_NATIVE_WORKER_INVALID;
    worker = worker_alloc(sizeof(*worker));
    if (!worker || !lock_init(&worker->lock)) {
        worker_free(worker);
        return CAMD_NATIVE_WORKER_NOMEM;
    }
    worker->pump = pump;
    worker->item_budget = config->ItemBudget;
    worker->state = CAMD_NATIVE_WORKER_STARTING;
#ifdef __AROS__
    worker->wake_signal = -1;
    worker->process = CreateNewProcTags(
        NP_Entry, (IPTR)worker_entry,
        NP_Name, (IPTR)"CAMD native event worker",
        NP_Priority, 0,
        NP_UserData, (IPTR)worker,
        TAG_END);
    if (!worker->process)
        goto fail;
    for (;;) {
        uint32_t state;

        lock_acquire(&worker->lock);
        state = worker->state;
        lock_release(&worker->lock);
        if (state != CAMD_NATIVE_WORKER_STARTING)
            break;
        Delay(1);
    }
    lock_acquire(&worker->lock);
    if (worker->state == CAMD_NATIVE_WORKER_FAILED) {
        lock_release(&worker->lock);
        goto fail;
    }
    lock_release(&worker->lock);
#else
    if (pthread_cond_init(&worker->condition, NULL) != 0)
        goto fail;
    if (pthread_create(&worker->thread, NULL, worker_entry, worker) != 0) {
        pthread_cond_destroy(&worker->condition);
        goto fail;
    }
    worker->thread_created = 1;
#endif
    *worker_out = worker;
    return CAMD_NATIVE_WORKER_OK;

fail:
    lock_destroy(&worker->lock);
    worker_free(worker);
    return CAMD_NATIVE_WORKER_NOMEM;
}

enum CAMDNativeWorkerResult camd_native_worker_wake(
    struct CAMDNativeEventWorker *worker)
{
    if (!worker)
        return CAMD_NATIVE_WORKER_INVALID;
    lock_acquire(&worker->lock);
    if (worker->state == CAMD_NATIVE_WORKER_STOPPED ||
        worker->state == CAMD_NATIVE_WORKER_FAILED) {
        lock_release(&worker->lock);
        return CAMD_NATIVE_WORKER_STATE;
    }
    counter_add(&worker->wake_requests, 1);
#ifdef __AROS__
    Signal(&worker->process->pr_Task, 1UL << worker->wake_signal);
#else
    worker->wake_pending = 1;
    pthread_cond_signal(&worker->condition);
#endif
    lock_release(&worker->lock);
    return CAMD_NATIVE_WORKER_OK;
}

enum CAMDNativeWorkerResult camd_native_worker_wake_after(
    struct CAMDNativeEventWorker *worker, uint32_t milliseconds)
{
    if (!worker)
        return CAMD_NATIVE_WORKER_INVALID;
    lock_acquire(&worker->lock);
    worker->retry_after_ms = milliseconds ? milliseconds : 1;
    lock_release(&worker->lock);
    return CAMD_NATIVE_WORKER_OK;
}

#ifdef __AROS__
enum CAMDNativeWorkerResult camd_native_worker_wake_from_interrupt(
    struct CAMDNativeEventWorker *worker)
{
    if (!worker || !worker->process || worker->wake_signal < 0)
        return CAMD_NATIVE_WORKER_INVALID;
    Signal(&worker->process->pr_Task, 1UL << worker->wake_signal);
    return CAMD_NATIVE_WORKER_OK;
}
#endif

enum CAMDNativeWorkerResult camd_native_worker_stop(
    struct CAMDNativeEventWorker *worker)
{
    if (!worker)
        return CAMD_NATIVE_WORKER_INVALID;
    lock_acquire(&worker->lock);
    if (worker->exited) {
        worker->stop_completed = 1;
        lock_release(&worker->lock);
        return CAMD_NATIVE_WORKER_OK;
    }
    worker->stop_requested = 1;
#ifdef __AROS__
    Signal(&worker->process->pr_Task, SIGBREAKF_CTRL_C);
    lock_release(&worker->lock);
    for (;;) {
        lock_acquire(&worker->lock);
        if (worker->exited) {
            worker->stop_completed = 1;
            lock_release(&worker->lock);
            return CAMD_NATIVE_WORKER_OK;
        }
        lock_release(&worker->lock);
        Delay(1);
    }
#else
    pthread_cond_signal(&worker->condition);
    lock_release(&worker->lock);
    if (pthread_join(worker->thread, NULL) != 0)
        return CAMD_NATIVE_WORKER_STATE;
    worker->thread_created = 0;
    lock_acquire(&worker->lock);
    worker->stop_completed = 1;
    lock_release(&worker->lock);
    return CAMD_NATIVE_WORKER_OK;
#endif
}

enum CAMDNativeWorkerResult camd_native_worker_stats(
    struct CAMDNativeEventWorker *worker,
    struct CAMDNativeWorkerStatsV1 *stats)
{
    if (!worker || !stats || stats->Size != sizeof(*stats) ||
        stats->Version != 1)
        return CAMD_NATIVE_WORKER_INVALID;
    lock_acquire(&worker->lock);
    stats->State = worker->state;
    stats->Reserved = 0;
    stats->WakeRequests = worker->wake_requests;
    stats->PumpRuns = worker->pump_runs;
    stats->ProcessedItems = worker->processed_items;
    stats->ProcessedRecords = worker->processed_records;
    stats->BlockedRuns = worker->blocked_runs;
    stats->DownstreamFailures = worker->downstream_failures;
    lock_release(&worker->lock);
    return CAMD_NATIVE_WORKER_OK;
}

enum CAMDNativeWorkerResult camd_native_worker_destroy(
    struct CAMDNativeEventWorker *worker)
{
    if (!worker)
        return CAMD_NATIVE_WORKER_INVALID;
    lock_acquire(&worker->lock);
    if (!worker->stop_completed || !worker->exited ||
        (worker->state != CAMD_NATIVE_WORKER_STOPPED &&
         worker->state != CAMD_NATIVE_WORKER_FAILED)) {
        lock_release(&worker->lock);
        return CAMD_NATIVE_WORKER_STATE;
    }
    lock_release(&worker->lock);
#ifndef __AROS__
    pthread_cond_destroy(&worker->condition);
#endif
    lock_destroy(&worker->lock);
    worker_free(worker);
    return CAMD_NATIVE_WORKER_OK;
}
