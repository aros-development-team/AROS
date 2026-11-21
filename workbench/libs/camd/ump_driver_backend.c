#include "ump_driver_backend.h"

#include <limits.h>
#include <string.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#else
#include <pthread.h>
#include <stdlib.h>
#endif

struct ump_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct CAMDUMPDriverBackend;
struct ump_session {
    struct CAMDUMPDriverBackend *backend;
    struct CAMDUMPBackendPortV1 *port;
    uint32_t direction;
    struct CAMDNativeEventQueue *queue;
    struct CAMDNativeEventPump *pump;
    struct CAMDNativeEventWorker *worker;
    struct CAMDNativeWorkerFanoutHandleV1 fanout;
    int fanout_attached;
    int acquired;
    struct ump_session *next_input;
    struct CAMDProviderReceiveSinkV1 sink;
    int receiving;
};

struct CAMDUMPDriverBackend {
    struct ump_lock lock;
    struct CAMDUMPBackendCallbacksV1 callbacks;
    struct CAMDUMPBackendPortV1 *ports;
    struct ump_session **inputs;
    size_t port_count;
    uint32_t protocol;
    uint32_t capacity;
    uint32_t budget;
    uint32_t active;
    int retiring;
    struct CAMDProviderOpsV1 ops;
};

static void *ualloc(size_t size)
{
#ifdef __AROS__
    return size <= (size_t)ULONG_MAX ? AllocVec((ULONG)size,
                                                MEMF_ANY | MEMF_CLEAR) : NULL;
#else
    return calloc(1, size);
#endif
}

static void ufree(void *p)
{
#ifdef __AROS__
    if (p) FreeVec(p);
#else
    free(p);
#endif
}

static int lock_init(struct ump_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore); return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}
static void lock_done(struct ump_lock *lock)
{
#ifndef __AROS__
    pthread_mutex_destroy(&lock->mutex);
#else
    (void)lock;
#endif
}
static void lock_get(struct ump_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}
static void lock_put(struct ump_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
#endif
}

static int id_equal(const struct CAMDEndpointIDV1 *a,
                    const struct CAMDEndpointIDV1 *b)
{ return memcmp(a, b, sizeof(*a)) == 0; }

static struct CAMDUMPBackendPortV1 *find_port(
    struct CAMDUMPDriverBackend *backend, const struct CAMDEndpointIDV1 *id)
{
    size_t i;
    for (i = 0; i < backend->port_count; ++i)
        if (id_equal(&backend->ports[i].EndpointID, id)) return &backend->ports[i];
    return NULL;
}

static enum CAMDProviderResult qresult(enum CAMDNativeQueueResult result)
{
    if (result == CAMD_NATIVE_QUEUE_OK) return CAMD_PROVIDER_OK;
    if (result == CAMD_NATIVE_QUEUE_FULL) return CAMD_PROVIDER_QUEUE_FULL;
    if (result == CAMD_NATIVE_QUEUE_INVALID ||
        result == CAMD_NATIVE_QUEUE_WRONG_FORMAT ||
        result == CAMD_NATIVE_QUEUE_RANGE) return CAMD_PROVIDER_INVALID;
    return CAMD_PROVIDER_STATE;
}

static enum CAMDProviderResult pump_submit(void *context,
    const struct CAMDUMPEventV1 *events, size_t count)
{
    struct ump_session *session = context;
    return session->backend->callbacks.Submit(session->port->Context,
                                               events, count);
}

static void release_active(struct CAMDUMPDriverBackend *backend)
{ lock_get(&backend->lock); --backend->active; lock_put(&backend->lock); }

static void destroy_pipeline(struct ump_session *session)
{
    size_t cancelled;
    if (session->worker) {
        camd_native_worker_stop(session->worker);
        camd_native_worker_destroy(session->worker);
    }
    if (session->queue) camd_native_queue_cancel(session->queue, &cancelled);
    if (session->pump) camd_native_pump_destroy(session->pump);
    if (session->queue) camd_native_queue_destroy(session->queue);
    session->worker = NULL; session->pump = NULL; session->queue = NULL;
}

static enum CAMDProviderResult open_session(void *context,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDProviderOpenResultV1 *result)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct CAMDUMPBackendPortV1 *port;
    struct ump_session *session;
    struct CAMDNativeQueueConfigV1 qc;
    struct CAMDNativePumpConfigV1 pc;
    struct CAMDNativeWorkerConfigV1 wc;
    struct CAMDProviderReceiveSinkV1 sink;
    enum CAMDProviderResult acquired;
    size_t index;

    if (!backend || !request || !result ||
        request->DataFormat != CAMD_PROVIDER_FORMAT_UMP ||
        request->Protocol != backend->protocol || request->QueueCapacity == 0 ||
        request->QueueCapacity > backend->capacity ||
        (request->Direction != CAMD_PROVIDER_DIRECTION_INPUT &&
         request->Direction != CAMD_PROVIDER_DIRECTION_OUTPUT))
        return CAMD_PROVIDER_UNSUPPORTED;
    port = find_port(backend, &request->EndpointID);
    if (!port) return CAMD_PROVIDER_INVALID;
    lock_get(&backend->lock);
    if (backend->retiring || backend->active == UINT32_MAX) {
        lock_put(&backend->lock);
        return backend->retiring ? CAMD_PROVIDER_RETIRED : CAMD_PROVIDER_STATE;
    }
    ++backend->active;
    lock_put(&backend->lock);
    session = ualloc(sizeof(*session));
    if (!session) { release_active(backend); return CAMD_PROVIDER_CALLBACK_FAILED; }
    session->backend = backend; session->port = port;
    session->direction = request->Direction;
    acquired = backend->callbacks.Acquire(port->Context, request->Direction);
    if (acquired != CAMD_PROVIDER_OK) goto fail;
    session->acquired = 1;
    if (request->Direction == CAMD_PROVIDER_DIRECTION_INPUT) {
        index = (size_t)(port - backend->ports);
        lock_get(&backend->lock);
        session->next_input = backend->inputs[index];
        backend->inputs[index] = session;
        lock_put(&backend->lock);
    } else {
        memset(&qc, 0, sizeof(qc)); qc.Size = sizeof(qc); qc.Version = 1;
        qc.DataFormat = CAMD_PROVIDER_FORMAT_UMP;
        qc.Capacity = request->QueueCapacity;
        if (camd_native_queue_create(&qc, &session->queue) != CAMD_NATIVE_QUEUE_OK)
            goto fail;
        memset(&sink, 0, sizeof(sink)); sink.Size = sizeof(sink); sink.Version = 1;
        sink.Context = session; sink.SubmitUMP = pump_submit;
        memset(&pc, 0, sizeof(pc)); pc.Size = sizeof(pc); pc.Version = 1;
        pc.DataFormat = CAMD_PROVIDER_FORMAT_UMP;
        pc.MaxBatchRecords = request->QueueCapacity;
        if (camd_native_pump_create(session->queue, &pc, &sink,
                                    &session->pump) != CAMD_NATIVE_PUMP_OK) goto fail;
        memset(&wc, 0, sizeof(wc)); wc.Size = sizeof(wc); wc.Version = 1;
        wc.ItemBudget = backend->budget;
        if (backend->callbacks.Drained) {
            wc.Idle = backend->callbacks.Drained; wc.IdleContext = port->Context;
        }
        if (camd_native_worker_create(session->pump, &wc,
                                      &session->worker) != CAMD_NATIVE_WORKER_OK) goto fail;
        if (camd_native_worker_fanout_attach(port->CapacityFanout,
                                             session->worker, &session->fanout) !=
            CAMD_NATIVE_WORKER_FANOUT_OK) goto fail;
        session->fanout_attached = 1;
    }
    result->SessionContext = session;
    result->EffectiveQueueCapacity = request->QueueCapacity;
    result->MaxSysExBytes = 0;
    return CAMD_PROVIDER_OK;
fail:
    if (session->fanout_attached)
        camd_native_worker_fanout_detach(port->CapacityFanout, session->fanout);
    destroy_pipeline(session);
    if (session->acquired) backend->callbacks.Release(port->Context,
                                                       request->Direction);
    ufree(session); release_active(backend); return CAMD_PROVIDER_CALLBACK_FAILED;
}

static enum CAMDProviderResult start_receive(void *context, void *session_context,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct ump_session *session = session_context;
    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_INPUT || !sink ||
        sink->Size != sizeof(*sink) || sink->Version != 1 || !sink->SubmitUMP)
        return CAMD_PROVIDER_INVALID;
    lock_get(&backend->lock);
    if (session->receiving) { lock_put(&backend->lock); return CAMD_PROVIDER_STATE; }
    session->sink = *sink; session->receiving = 1;
    lock_put(&backend->lock); return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult stop_receive(void *context, void *session_context)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct ump_session *session = session_context;
    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_INPUT) return CAMD_PROVIDER_INVALID;
    lock_get(&backend->lock); session->receiving = 0; lock_put(&backend->lock);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult close_session(void *context, void *session_context)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct ump_session *session = session_context;
    struct ump_session **link;
    enum CAMDProviderResult released;
    if (!backend || !session || session->backend != backend) return CAMD_PROVIDER_INVALID;
    if (session->direction == CAMD_PROVIDER_DIRECTION_INPUT) {
        lock_get(&backend->lock);
        link = &backend->inputs[session->port - backend->ports];
        while (*link && *link != session) link = &(*link)->next_input;
        if (*link) *link = session->next_input;
        session->receiving = 0; lock_put(&backend->lock);
    } else {
        if (session->fanout_attached) {
            if (camd_native_worker_fanout_detach(session->port->CapacityFanout,
                                                 session->fanout) !=
                CAMD_NATIVE_WORKER_FANOUT_OK) return CAMD_PROVIDER_STATE;
            session->fanout_attached = 0;
        }
        destroy_pipeline(session);
    }
    released = backend->callbacks.Release(session->port->Context,
                                           session->direction);
    if (released != CAMD_PROVIDER_OK) return released;
    ufree(session); release_active(backend); return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult send_ump(void *context, void *session_context,
    const struct CAMDUMPEventV1 *events, size_t count)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct ump_session *session = session_context;
    enum CAMDNativeQueueResult result;
    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT)
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_ump(session->queue, events, count);
    if (result != CAMD_NATIVE_QUEUE_OK) return qresult(result);
    camd_native_worker_wake(session->worker); return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult drain_session(void *context, void *session_context)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct ump_session *session = session_context;
    struct CAMDNativeQueueStatsV1 stats;
    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT) return CAMD_PROVIDER_INVALID;
    memset(&stats, 0, sizeof(stats)); stats.Size = sizeof(stats); stats.Version = 1;
    if (camd_native_queue_stats(session->queue, &stats) != CAMD_NATIVE_QUEUE_OK)
        return CAMD_PROVIDER_STATE;
    if (stats.PendingRecords == 0) return CAMD_PROVIDER_OK;
    camd_native_worker_wake(session->worker); return CAMD_PROVIDER_QUEUE_FULL;
}

static enum CAMDProviderResult cancel_session(void *context, void *session_context)
{
    struct CAMDUMPDriverBackend *backend = context;
    struct ump_session *session = session_context; size_t cancelled;
    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT) return CAMD_PROVIDER_INVALID;
    return qresult(camd_native_queue_cancel(session->queue, &cancelled));
}

static enum CAMDProviderResult begin_shutdown(void *context)
{
    struct CAMDUMPDriverBackend *backend = context;
    if (!backend) return CAMD_PROVIDER_INVALID;
    lock_get(&backend->lock); backend->retiring = 1; lock_put(&backend->lock);
    return CAMD_PROVIDER_OK;
}
static int shutdown_ready(void *context)
{
    struct CAMDUMPDriverBackend *backend = context; int ready;
    if (!backend) return 0;
    lock_get(&backend->lock); ready = backend->retiring && backend->active == 0;
    lock_put(&backend->lock); return ready;
}

enum CAMDProviderResult camd_ump_driver_backend_receive(
    struct CAMDUMPDriverBackend *backend, size_t port,
    const struct CAMDUMPEventV1 *events, size_t count, uint32_t *dropped)
{
    struct ump_session *session; uint32_t lost = 0;
    if (dropped) *dropped = 0;
    if (!backend || port >= backend->port_count || !events || count == 0)
        return CAMD_PROVIDER_INVALID;
    lock_get(&backend->lock);
    for (session = backend->inputs[port]; session; session = session->next_input)
        if (session->receiving &&
            session->sink.SubmitUMP(session->sink.Context, events, count) !=
                CAMD_PROVIDER_OK) ++lost;
    lock_put(&backend->lock); if (dropped) *dropped = lost;
    return CAMD_PROVIDER_OK;
}

static int valid_config(const struct CAMDUMPBackendConfigV1 *config)
{
    size_t i;
    if (!config || config->Size != sizeof(*config) || config->Version != 1 ||
        config->Callbacks.Size != sizeof(config->Callbacks) ||
        config->Callbacks.Version != 1 || !config->Callbacks.Acquire ||
        !config->Callbacks.Release || !config->Callbacks.Submit ||
        !config->Ports || config->PortCount == 0 || config->MaxQueueCapacity == 0 ||
        config->WorkerItemBudget == 0 ||
        (config->Protocol != CAMD_PROVIDER_PROTOCOL_MIDI1 &&
         config->Protocol != CAMD_PROVIDER_PROTOCOL_MIDI2)) return 0;
    for (i = 0; i < config->PortCount; ++i)
        if (config->Ports[i].Size != sizeof(config->Ports[i]) ||
            config->Ports[i].Version != 1 || !config->Ports[i].CapacityFanout)
            return 0;
    return 1;
}

enum CAMDProviderResult camd_ump_driver_backend_create(
    const struct CAMDUMPBackendConfigV1 *config,
    struct CAMDUMPDriverBackend **out)
{
    struct CAMDUMPDriverBackend *backend;
    if (!out)
        return CAMD_PROVIDER_INVALID;
    *out = NULL;
    if (!valid_config(config)) return CAMD_PROVIDER_INVALID;
    backend = ualloc(sizeof(*backend)); if (!backend) return CAMD_PROVIDER_CALLBACK_FAILED;
    if (!lock_init(&backend->lock)) { ufree(backend); return CAMD_PROVIDER_CALLBACK_FAILED; }
    backend->ports = ualloc(config->PortCount * sizeof(*backend->ports));
    backend->inputs = ualloc(config->PortCount * sizeof(*backend->inputs));
    if (!backend->ports || !backend->inputs) {
        ufree(backend->inputs); ufree(backend->ports); lock_done(&backend->lock);
        ufree(backend); return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    memcpy(backend->ports, config->Ports, config->PortCount * sizeof(*backend->ports));
    backend->callbacks = config->Callbacks; backend->port_count = config->PortCount;
    backend->protocol = config->Protocol; backend->capacity = config->MaxQueueCapacity;
    backend->budget = config->WorkerItemBudget;
    backend->ops.Size = sizeof(backend->ops); backend->ops.Version = 1;
    backend->ops.Open = open_session; backend->ops.Close = close_session;
    backend->ops.SendUMP = send_ump; backend->ops.StartReceive = start_receive;
    backend->ops.StopReceive = stop_receive; backend->ops.Drain = drain_session;
    backend->ops.Cancel = cancel_session; backend->ops.BeginShutdown = begin_shutdown;
    backend->ops.ShutdownReady = shutdown_ready; *out = backend;
    return CAMD_PROVIDER_OK;
}

const struct CAMDProviderOpsV1 *camd_ump_driver_backend_ops(
    struct CAMDUMPDriverBackend *backend) { return backend ? &backend->ops : NULL; }
void *camd_ump_driver_backend_context(struct CAMDUMPDriverBackend *backend)
{ return backend; }

enum CAMDProviderResult camd_ump_driver_backend_destroy(
    struct CAMDUMPDriverBackend *backend)
{
    if (!backend) return CAMD_PROVIDER_INVALID;
    lock_get(&backend->lock);
    if (backend->active) { lock_put(&backend->lock); return CAMD_PROVIDER_STATE; }
    lock_put(&backend->lock); ufree(backend->inputs); ufree(backend->ports);
    lock_done(&backend->lock); ufree(backend); return CAMD_PROVIDER_OK;
}
