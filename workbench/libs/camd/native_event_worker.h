#ifndef CAMD_NATIVE_EVENT_WORKER_H
#define CAMD_NATIVE_EVENT_WORKER_H

/* Private task/signal executor for the bounded native-event pump. */

#include "native_event_pump.h"

enum CAMDNativeWorkerResult {
    CAMD_NATIVE_WORKER_OK = 0,
    CAMD_NATIVE_WORKER_INVALID,
    CAMD_NATIVE_WORKER_NOMEM,
    CAMD_NATIVE_WORKER_STATE
};

enum CAMDNativeWorkerStateV1 {
    CAMD_NATIVE_WORKER_STARTING = 1,
    CAMD_NATIVE_WORKER_IDLE,
    CAMD_NATIVE_WORKER_RUNNING,
    CAMD_NATIVE_WORKER_STOPPED,
    CAMD_NATIVE_WORKER_FAILED
};

struct CAMDNativeWorkerConfigV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t ItemBudget;
};

struct CAMDNativeWorkerStatsV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t State;
    uint32_t Reserved;
    uint64_t WakeRequests;
    uint64_t PumpRuns;
    uint64_t ProcessedItems;
    uint64_t ProcessedRecords;
    uint64_t BlockedRuns;
    uint64_t DownstreamFailures;
};

struct CAMDNativeEventWorker;

enum CAMDNativeWorkerResult camd_native_worker_create(
    struct CAMDNativeEventPump *pump,
    const struct CAMDNativeWorkerConfigV1 *config,
    struct CAMDNativeEventWorker **worker);

/* Wake requests coalesce. A blocked pump remains asleep until a producer or
 * downstream-capacity notification explicitly wakes it again. */
enum CAMDNativeWorkerResult camd_native_worker_wake(
    struct CAMDNativeEventWorker *worker);

/* For the pump's downstream callback, which runs in the worker: when the
 * pump then reports that it is blocked, the worker runs it again after at
 * most this long, without a wake. Zero is taken as one millisecond. */
enum CAMDNativeWorkerResult camd_native_worker_wake_after(
    struct CAMDNativeEventWorker *worker, uint32_t milliseconds);

#ifdef __AROS__
/* Interrupt-safe AROS capacity notification.  Unlike the task-context wake,
 * this only calls Exec Signal() and does not acquire a semaphore or update
 * statistics.  The caller must prevent concurrent stop/destroy and keep the
 * worker alive until every interrupt source that can call this has quiesced. */
enum CAMDNativeWorkerResult camd_native_worker_wake_from_interrupt(
    struct CAMDNativeEventWorker *worker);
#endif

/* Synchronous and idempotent. No callback is active when it returns. */
enum CAMDNativeWorkerResult camd_native_worker_stop(
    struct CAMDNativeEventWorker *worker);

enum CAMDNativeWorkerResult camd_native_worker_stats(
    struct CAMDNativeEventWorker *worker,
    struct CAMDNativeWorkerStatsV1 *stats);

/* The worker must have been stopped. The pump and queue remain caller-owned. */
enum CAMDNativeWorkerResult camd_native_worker_destroy(
    struct CAMDNativeEventWorker *worker);

#endif
