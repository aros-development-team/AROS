#include "native_worker_fanout.h"

#include <limits.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#else
#include <pthread.h>
#include <stdlib.h>
#endif

struct fanout_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct fanout_slot {
    struct CAMDNativeEventWorker *worker;
    uint32_t generation;
};

struct CAMDNativeWorkerFanout {
    struct fanout_lock lock;
    struct fanout_slot *slots;
    uint32_t capacity;
    uint32_t attached;
};

static void *fanout_alloc(size_t size)
{
#ifdef __AROS__
    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, MEMF_ANY | MEMF_CLEAR);
#else
    return calloc(1, size);
#endif
}

static void fanout_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int lock_init(struct fanout_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore);
    return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}

static void lock_destroy(struct fanout_lock *lock)
{
#ifdef __AROS__
    (void)lock;
#else
    pthread_mutex_destroy(&lock->mutex);
#endif
}

static void lock_acquire(struct fanout_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}

static void lock_release(struct fanout_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
#endif
}

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_create(
    uint32_t capacity,
    struct CAMDNativeWorkerFanout **fanout_out)
{
    struct CAMDNativeWorkerFanout *fanout;
    size_t bytes;

    if (!fanout_out)
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
    *fanout_out = NULL;
    if (capacity == 0)
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
#if SIZE_MAX <= UINT32_MAX
    if (capacity > SIZE_MAX / sizeof(struct fanout_slot))
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
#endif
    fanout = fanout_alloc(sizeof(*fanout));
    if (!fanout)
        return CAMD_NATIVE_WORKER_FANOUT_NOMEM;
    if (!lock_init(&fanout->lock)) {
        fanout_free(fanout);
        return CAMD_NATIVE_WORKER_FANOUT_NOMEM;
    }
    bytes = (size_t)capacity * sizeof(*fanout->slots);
    fanout->slots = fanout_alloc(bytes);
    if (!fanout->slots) {
        lock_destroy(&fanout->lock);
        fanout_free(fanout);
        return CAMD_NATIVE_WORKER_FANOUT_NOMEM;
    }
    fanout->capacity = capacity;
    *fanout_out = fanout;
    return CAMD_NATIVE_WORKER_FANOUT_OK;
}

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_attach(
    struct CAMDNativeWorkerFanout *fanout,
    struct CAMDNativeEventWorker *worker,
    struct CAMDNativeWorkerFanoutHandleV1 *handle)
{
    uint32_t i, free_slot = UINT32_MAX;

    if (!fanout || !worker || !handle)
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
    lock_acquire(&fanout->lock);
    for (i = 0; i < fanout->capacity; ++i) {
        if (fanout->slots[i].worker == worker) {
            lock_release(&fanout->lock);
            return CAMD_NATIVE_WORKER_FANOUT_INVALID;
        }
        if (!fanout->slots[i].worker && free_slot == UINT32_MAX)
            free_slot = i;
    }
    if (free_slot == UINT32_MAX) {
        lock_release(&fanout->lock);
        return CAMD_NATIVE_WORKER_FANOUT_FULL;
    }
    if (++fanout->slots[free_slot].generation == 0)
        ++fanout->slots[free_slot].generation;
    fanout->slots[free_slot].worker = worker;
    ++fanout->attached;
    handle->Slot = free_slot;
    handle->Generation = fanout->slots[free_slot].generation;
    lock_release(&fanout->lock);
    return CAMD_NATIVE_WORKER_FANOUT_OK;
}

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_detach(
    struct CAMDNativeWorkerFanout *fanout,
    struct CAMDNativeWorkerFanoutHandleV1 handle)
{
    struct fanout_slot *slot;

    if (!fanout || handle.Slot >= fanout->capacity ||
        handle.Generation == 0)
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
    lock_acquire(&fanout->lock);
    slot = &fanout->slots[handle.Slot];
    if (!slot->worker || slot->generation != handle.Generation) {
        lock_release(&fanout->lock);
        return CAMD_NATIVE_WORKER_FANOUT_STATE;
    }
    slot->worker = NULL;
    --fanout->attached;
    lock_release(&fanout->lock);
    return CAMD_NATIVE_WORKER_FANOUT_OK;
}

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_wake_all(
    struct CAMDNativeWorkerFanout *fanout,
    uint32_t *woken)
{
    enum CAMDNativeWorkerFanoutResult result =
        CAMD_NATIVE_WORKER_FANOUT_OK;
    uint32_t count = 0, i;

    if (!fanout)
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
    lock_acquire(&fanout->lock);
    for (i = 0; i < fanout->capacity; ++i) {
        if (fanout->slots[i].worker) {
            if (camd_native_worker_wake(fanout->slots[i].worker) ==
                CAMD_NATIVE_WORKER_OK)
                ++count;
            else
                result = CAMD_NATIVE_WORKER_FANOUT_STATE;
        }
    }
    lock_release(&fanout->lock);
    if (woken)
        *woken = count;
    return result;
}

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_destroy(
    struct CAMDNativeWorkerFanout *fanout)
{
    if (!fanout)
        return CAMD_NATIVE_WORKER_FANOUT_INVALID;
    lock_acquire(&fanout->lock);
    if (fanout->attached != 0) {
        lock_release(&fanout->lock);
        return CAMD_NATIVE_WORKER_FANOUT_STATE;
    }
    lock_release(&fanout->lock);
    fanout_free(fanout->slots);
    lock_destroy(&fanout->lock);
    fanout_free(fanout);
    return CAMD_NATIVE_WORKER_FANOUT_OK;
}
