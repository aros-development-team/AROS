#ifndef CAMD_NATIVE_WORKER_FANOUT_H
#define CAMD_NATIVE_WORKER_FANOUT_H

/* Bounded task-context fan-out for workers sharing one physical port. */

#include "native_event_worker.h"

enum CAMDNativeWorkerFanoutResult {
    CAMD_NATIVE_WORKER_FANOUT_OK = 0,
    CAMD_NATIVE_WORKER_FANOUT_INVALID,
    CAMD_NATIVE_WORKER_FANOUT_NOMEM,
    CAMD_NATIVE_WORKER_FANOUT_FULL,
    CAMD_NATIVE_WORKER_FANOUT_STATE
};

struct CAMDNativeWorkerFanoutHandleV1 {
    uint32_t Slot;
    uint32_t Generation;
};

struct CAMDNativeWorkerFanout;

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_create(
    uint32_t capacity,
    struct CAMDNativeWorkerFanout **fanout);

enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_attach(
    struct CAMDNativeWorkerFanout *fanout,
    struct CAMDNativeEventWorker *worker,
    struct CAMDNativeWorkerFanoutHandleV1 *handle);

/* The worker must remain running until detach. Synchronous: after detach
 * returns, wake_all no longer references it. */
enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_detach(
    struct CAMDNativeWorkerFanout *fanout,
    struct CAMDNativeWorkerFanoutHandleV1 handle);

/* Task context only. The interrupt handler must signal a stable relay task. */
enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_wake_all(
    struct CAMDNativeWorkerFanout *fanout,
    uint32_t *woken);

/* Fails with STATE until every worker has been detached. The caller must also
 * have quiesced concurrent attach/detach/wake calls before destruction. */
enum CAMDNativeWorkerFanoutResult camd_native_worker_fanout_destroy(
    struct CAMDNativeWorkerFanout *fanout);

#endif
