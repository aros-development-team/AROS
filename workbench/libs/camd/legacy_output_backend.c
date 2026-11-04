#include "legacy_output_backend.h"

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

struct backend_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct CAMDLegacyOutputBackend {
    struct backend_lock lock;
    struct CAMDLegacyOutputCallbacksV1 callbacks;
    struct CAMDLegacyOutputPortV1 *ports;
    struct output_session **inputs;     /* per port, under lock */
    size_t port_count;
    uint32_t max_queue_capacity;
    uint32_t max_sysex_bytes;
    uint32_t worker_item_budget;
    uint32_t active_sessions;
    int retiring;
    struct CAMDProviderOpsV1 ops;
};

struct output_session {
    struct CAMDLegacyOutputBackend *backend;
    struct CAMDLegacyOutputPortV1 *port;
    struct CAMDNativeEventQueue *queue;
    struct CAMDNativeEventPump *pump;
    struct CAMDNativeEventWorker *worker;
    struct CAMDNativeWorkerFanoutHandleV1 fanout_handle;
    int fanout_attached;
    int port_acquired;
    int pipeline_destroyed;
    uint32_t direction;
    /* Input sessions only; all under the backend lock. */
    struct output_session *next_input;
    struct CAMDProviderReceiveSinkV1 sink;
    int receiving;
};

static void *backend_alloc(size_t size)
{
#ifdef __AROS__
    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, MEMF_ANY | MEMF_CLEAR);
#else
    return calloc(1, size);
#endif
}

static void backend_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int lock_init(struct backend_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore);
    return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}

static void lock_destroy(struct backend_lock *lock)
{
#ifdef __AROS__
    (void)lock;
#else
    pthread_mutex_destroy(&lock->mutex);
#endif
}

static void lock_acquire(struct backend_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}

static void lock_release(struct backend_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
#endif
}

static int id_equal(const struct CAMDEndpointIDV1 *left,
                    const struct CAMDEndpointIDV1 *right)
{
    return memcmp(left, right, sizeof(*left)) == 0;
}

static int id_is_zero(const struct CAMDEndpointIDV1 *id)
{
    return id->word[0] == 0 && id->word[1] == 0 &&
           id->word[2] == 0 && id->word[3] == 0;
}

static struct CAMDLegacyOutputPortV1 *find_port(
    struct CAMDLegacyOutputBackend *backend,
    const struct CAMDEndpointIDV1 *endpoint_id)
{
    size_t i;

    for (i = 0; i < backend->port_count; ++i) {
        if (id_equal(&backend->ports[i].EndpointID, endpoint_id))
            return &backend->ports[i];
    }
    return NULL;
}

static enum CAMDProviderResult queue_result(
    enum CAMDNativeQueueResult result)
{
    if (result == CAMD_NATIVE_QUEUE_OK)
        return CAMD_PROVIDER_OK;
    if (result == CAMD_NATIVE_QUEUE_FULL)
        return CAMD_PROVIDER_QUEUE_FULL;
    if (result == CAMD_NATIVE_QUEUE_TOO_LARGE)
        return CAMD_PROVIDER_TOO_LARGE;
    if (result == CAMD_NATIVE_QUEUE_INVALID ||
        result == CAMD_NATIVE_QUEUE_WRONG_FORMAT ||
        result == CAMD_NATIVE_QUEUE_RANGE)
        return CAMD_PROVIDER_INVALID;
    return CAMD_PROVIDER_STATE;
}

/* Whether the head message may go to the port now. One that has to wait
 * tells the session's worker when to try again; the pump keeps it queued
 * because the caller then reports no room. Times compare by their signed
 * difference, so the clock may wrap. */
static int due(struct output_session *session, uint32_t flags,
               uint32_t time_low)
{
    int32_t ahead;

    if ((flags & CAMD_EVENT_TIME_VALID) == 0 ||
        !session->backend->callbacks.Now)
        return 1;
    ahead = (int32_t)(time_low -
                      session->backend->callbacks.Now(session->port->Context));
    if (ahead <= 0)
        return 1;
    camd_native_worker_wake_after(session->worker, (uint32_t)ahead);
    return 0;
}

static enum CAMDProviderResult submit_midi1(
    void *context, const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct output_session *session = context;

    if (!session || !events || event_count != 1)
        return CAMD_PROVIDER_INVALID;
    if (!due(session, events->Flags, events->TimeLow))
        return CAMD_PROVIDER_QUEUE_FULL;
    return session->backend->callbacks.SubmitMIDI1(session->port->Context,
                                                    events);
}

static enum CAMDProviderResult submit_sysex(
    void *context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags)
{
    struct output_session *session = context;

    if (!session)
        return CAMD_PROVIDER_INVALID;
    if (!due(session, flags, time_low))
        return CAMD_PROVIDER_QUEUE_FULL;
    return session->backend->callbacks.SubmitMIDI1SysEx(
        session->port->Context, bytes, byte_count, time_high, time_low,
        clock_domain, flags);
}

static void release_reservation(struct CAMDLegacyOutputBackend *backend)
{
    lock_acquire(&backend->lock);
    --backend->active_sessions;
    lock_release(&backend->lock);
}

static void destroy_pipeline(struct output_session *session)
{
    size_t cancelled;

    if (session->pipeline_destroyed)
        return;
    if (session->worker) {
        camd_native_worker_stop(session->worker);
        camd_native_worker_destroy(session->worker);
        session->worker = NULL;
    }
    if (session->queue)
        camd_native_queue_cancel(session->queue, &cancelled);
    camd_native_pump_destroy(session->pump);
    session->pump = NULL;
    camd_native_queue_destroy(session->queue);
    session->queue = NULL;
    session->pipeline_destroyed = 1;
}

static enum CAMDProviderResult open_input(
    struct CAMDLegacyOutputBackend *backend,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDProviderOpenResultV1 *result)
{
    struct CAMDLegacyOutputPortV1 *port;
    struct output_session *session;
    enum CAMDProviderResult provider_result;
    size_t index;

    if (!backend->callbacks.AcquireInput ||
        request->DataFormat != CAMD_PROVIDER_FORMAT_MIDI1 ||
        request->Protocol != CAMD_PROVIDER_PROTOCOL_MIDI1 ||
        request->QueueCapacity == 0 ||
        request->QueueCapacity > backend->max_queue_capacity)
        return CAMD_PROVIDER_UNSUPPORTED;
    port = find_port(backend, &request->EndpointID);
    if (!port)
        return CAMD_PROVIDER_INVALID;
    index = (size_t)(port - backend->ports);
    lock_acquire(&backend->lock);
    if (backend->retiring || backend->active_sessions == UINT32_MAX) {
        lock_release(&backend->lock);
        return backend->retiring ? CAMD_PROVIDER_RETIRED
                                 : CAMD_PROVIDER_STATE;
    }
    ++backend->active_sessions;
    lock_release(&backend->lock);

    session = backend_alloc(sizeof(*session));
    if (!session) {
        release_reservation(backend);
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    session->backend = backend;
    session->port = port;
    session->direction = CAMD_PROVIDER_DIRECTION_INPUT;
    session->pipeline_destroyed = 1;
    provider_result = backend->callbacks.AcquireInput(port->Context);
    if (provider_result != CAMD_PROVIDER_OK) {
        backend_free(session);
        release_reservation(backend);
        return provider_result;
    }
    session->port_acquired = 1;
    lock_acquire(&backend->lock);
    session->next_input = backend->inputs[index];
    backend->inputs[index] = session;
    lock_release(&backend->lock);
    result->SessionContext = session;
    result->EffectiveQueueCapacity = request->QueueCapacity;
    result->MaxSysExBytes = backend->max_sysex_bytes;
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult close_input(
    struct CAMDLegacyOutputBackend *backend, struct output_session *session)
{
    struct output_session **link;
    enum CAMDProviderResult result;

    /* Out of the list first: no delivery can then reach the session. */
    lock_acquire(&backend->lock);
    link = &backend->inputs[session->port - backend->ports];
    while (*link && *link != session)
        link = &(*link)->next_input;
    if (*link)
        *link = session->next_input;
    session->receiving = 0;
    lock_release(&backend->lock);
    if (session->port_acquired) {
        result = backend->callbacks.ReleaseInput(session->port->Context);
        if (result != CAMD_PROVIDER_OK)
            return result;
        session->port_acquired = 0;
    }
    backend_free(session);
    release_reservation(backend);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult backend_start_receive(
    void *context, void *session_context,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;

    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_INPUT || !sink ||
        sink->Size != sizeof(*sink) || sink->Version != 1 ||
        !sink->SubmitMIDI1 || !sink->SubmitMIDI1SysEx)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    if (session->receiving) {
        lock_release(&backend->lock);
        return CAMD_PROVIDER_STATE;
    }
    session->sink = *sink;
    session->receiving = 1;
    lock_release(&backend->lock);
    return CAMD_PROVIDER_OK;
}

/* Deliveries hold the lock, so none is in a sink once this returns. */
static enum CAMDProviderResult backend_stop_receive(void *context,
                                                    void *session_context)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;

    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_INPUT)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    session->receiving = 0;
    lock_release(&backend->lock);
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_legacy_output_backend_receive_midi1(
    struct CAMDLegacyOutputBackend *backend, size_t port_index,
    const struct CAMDMIDI1EventV1 *event, uint32_t *dropped)
{
    struct output_session *session;
    uint32_t lost = 0;

    if (dropped)
        *dropped = 0;
    if (!backend || port_index >= backend->port_count || !event)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    for (session = backend->inputs[port_index]; session;
         session = session->next_input) {
        if (session->receiving &&
            session->sink.SubmitMIDI1(session->sink.Context, event, 1) !=
                CAMD_PROVIDER_OK)
            ++lost;
    }
    lock_release(&backend->lock);
    if (dropped)
        *dropped = lost;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_legacy_output_backend_receive_sysex(
    struct CAMDLegacyOutputBackend *backend, size_t port_index,
    const uint8_t *bytes, size_t byte_count, uint32_t *dropped)
{
    struct output_session *session;
    uint32_t lost = 0;

    if (dropped)
        *dropped = 0;
    if (!backend || port_index >= backend->port_count || !bytes ||
        byte_count < 2)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    for (session = backend->inputs[port_index]; session;
         session = session->next_input) {
        if (session->receiving &&
            session->sink.SubmitMIDI1SysEx(session->sink.Context, bytes,
                                           byte_count, 0, 0, 0, 0) !=
                CAMD_PROVIDER_OK)
            ++lost;
    }
    lock_release(&backend->lock);
    if (dropped)
        *dropped = lost;
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult backend_open(
    void *context, const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDProviderOpenResultV1 *result)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct CAMDLegacyOutputPortV1 *port;
    struct output_session *session;
    struct CAMDNativeQueueConfigV1 queue_config;
    struct CAMDNativePumpConfigV1 pump_config;
    struct CAMDNativeWorkerConfigV1 worker_config;
    struct CAMDProviderReceiveSinkV1 sink;
    enum CAMDProviderResult provider_result = CAMD_PROVIDER_CALLBACK_FAILED;

    if (backend && request && result &&
        request->Direction == CAMD_PROVIDER_DIRECTION_INPUT)
        return open_input(backend, request, result);
    if (!backend || !request || !result ||
        request->Direction != CAMD_PROVIDER_DIRECTION_OUTPUT ||
        request->DataFormat != CAMD_PROVIDER_FORMAT_MIDI1 ||
        request->Protocol != CAMD_PROVIDER_PROTOCOL_MIDI1 ||
        request->QueueCapacity == 0 ||
        request->QueueCapacity > backend->max_queue_capacity)
        return CAMD_PROVIDER_UNSUPPORTED;
    port = find_port(backend, &request->EndpointID);
    if (!port)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    if (backend->retiring || backend->active_sessions == UINT32_MAX) {
        lock_release(&backend->lock);
        return backend->retiring ? CAMD_PROVIDER_RETIRED
                                 : CAMD_PROVIDER_STATE;
    }
    ++backend->active_sessions;
    lock_release(&backend->lock);

    session = backend_alloc(sizeof(*session));
    if (!session) {
        release_reservation(backend);
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    session->backend = backend;
    session->port = port;
    session->direction = CAMD_PROVIDER_DIRECTION_OUTPUT;
    memset(&queue_config, 0, sizeof(queue_config));
    queue_config.Size = sizeof(queue_config);
    queue_config.Version = 1;
    queue_config.DataFormat = CAMD_PROVIDER_FORMAT_MIDI1;
    queue_config.Capacity = request->QueueCapacity;
    queue_config.MaxSysExBytes = backend->max_sysex_bytes;
    if (camd_native_queue_create(&queue_config, &session->queue) !=
        CAMD_NATIVE_QUEUE_OK)
        goto fail;

    memset(&sink, 0, sizeof(sink));
    sink.Size = sizeof(sink);
    sink.Version = 1;
    sink.Context = session;
    sink.SubmitMIDI1 = submit_midi1;
    sink.SubmitMIDI1SysEx = submit_sysex;
    memset(&pump_config, 0, sizeof(pump_config));
    pump_config.Size = sizeof(pump_config);
    pump_config.Version = 1;
    pump_config.DataFormat = CAMD_PROVIDER_FORMAT_MIDI1;
    pump_config.MaxBatchRecords = 1;
    pump_config.MaxSysExBytes = backend->max_sysex_bytes;
    if (camd_native_pump_create(session->queue, &pump_config, &sink,
                                &session->pump) != CAMD_NATIVE_PUMP_OK)
        goto fail;

    memset(&worker_config, 0, sizeof(worker_config));
    worker_config.Size = sizeof(worker_config);
    worker_config.Version = 1;
    worker_config.ItemBudget = backend->worker_item_budget;
    if (camd_native_worker_create(session->pump, &worker_config,
                                  &session->worker) != CAMD_NATIVE_WORKER_OK)
        goto fail;
    provider_result = backend->callbacks.Acquire(port->Context);
    if (provider_result != CAMD_PROVIDER_OK)
        goto fail;
    session->port_acquired = 1;
    if (camd_native_worker_fanout_attach(port->CapacityFanout,
                                         session->worker,
                                         &session->fanout_handle) !=
        CAMD_NATIVE_WORKER_FANOUT_OK) {
        provider_result = CAMD_PROVIDER_CALLBACK_FAILED;
        goto fail;
    }
    session->fanout_attached = 1;
    result->SessionContext = session;
    result->EffectiveQueueCapacity = request->QueueCapacity;
    result->MaxSysExBytes = backend->max_sysex_bytes;
    return CAMD_PROVIDER_OK;

fail:
    if (session->fanout_attached)
        camd_native_worker_fanout_detach(port->CapacityFanout,
                                         session->fanout_handle);
    destroy_pipeline(session);
    if (session->port_acquired)
        backend->callbacks.Release(port->Context);
    backend_free(session);
    release_reservation(backend);
    return provider_result;
}

static enum CAMDProviderResult backend_close(void *context,
                                              void *session_context)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;
    enum CAMDProviderResult result;

    if (!backend || !session || session->backend != backend)
        return CAMD_PROVIDER_INVALID;
    if (session->direction == CAMD_PROVIDER_DIRECTION_INPUT)
        return close_input(backend, session);
    if (session->fanout_attached) {
        if (camd_native_worker_fanout_detach(
                session->port->CapacityFanout, session->fanout_handle) !=
            CAMD_NATIVE_WORKER_FANOUT_OK)
            return CAMD_PROVIDER_STATE;
        session->fanout_attached = 0;
    }
    destroy_pipeline(session);
    if (session->port_acquired) {
        result = backend->callbacks.Release(session->port->Context);
        if (result != CAMD_PROVIDER_OK)
            return result;
        session->port_acquired = 0;
    }
    backend_free(session);
    release_reservation(backend);
    return CAMD_PROVIDER_OK;
}

/* A time in a clock this backend does not know cannot be kept. */
static int known_clock(const struct CAMDMIDI1EventV1 *events,
                       size_t event_count)
{
    size_t i;

    for (i = 0; i < event_count; ++i) {
        if ((events[i].Flags & CAMD_EVENT_TIME_VALID) != 0 &&
            (events[i].ClockDomain != CAMD_CLOCK_CAMD ||
             events[i].TimeHigh != 0))
            return 0;
    }
    return 1;
}

static enum CAMDProviderResult backend_send_midi1(
    void *context, void *session_context,
    const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;
    enum CAMDNativeQueueResult result;

    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT)
        return CAMD_PROVIDER_INVALID;
    if (!known_clock(events, event_count))
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_midi1_items(session->queue, events,
                                                   event_count);
    if (result != CAMD_NATIVE_QUEUE_OK)
        return queue_result(result);
    /* Accepted records must never be replayed merely because a wake failed. */
    camd_native_worker_wake(session->worker);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult backend_send_sysex(
    void *context, void *session_context, const uint8_t *bytes,
    size_t byte_count, uint32_t time_high, uint32_t time_low,
    uint32_t clock_domain, uint32_t flags)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;
    enum CAMDNativeQueueResult result;

    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT)
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_midi1_sysex(
        session->queue, bytes, byte_count, time_high, time_low,
        clock_domain, flags);
    if (result != CAMD_NATIVE_QUEUE_OK)
        return queue_result(result);
    camd_native_worker_wake(session->worker);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult backend_drain(void *context,
                                             void *session_context)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;
    struct CAMDNativeQueueStatsV1 stats;

    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT)
        return CAMD_PROVIDER_INVALID;
    memset(&stats, 0, sizeof(stats));
    stats.Size = sizeof(stats);
    stats.Version = 1;
    if (camd_native_queue_stats(session->queue, &stats) !=
        CAMD_NATIVE_QUEUE_OK)
        return CAMD_PROVIDER_STATE;
    if (stats.PendingRecords == 0)
        return CAMD_PROVIDER_OK;
    camd_native_worker_wake(session->worker);
    return CAMD_PROVIDER_QUEUE_FULL;
}

static enum CAMDProviderResult backend_cancel(void *context,
                                              void *session_context)
{
    struct CAMDLegacyOutputBackend *backend = context;
    struct output_session *session = session_context;
    size_t cancelled;

    if (!backend || !session || session->backend != backend ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT)
        return CAMD_PROVIDER_INVALID;
    return queue_result(camd_native_queue_cancel(session->queue, &cancelled));
}

static enum CAMDProviderResult backend_begin_shutdown(void *context)
{
    struct CAMDLegacyOutputBackend *backend = context;

    if (!backend)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    backend->retiring = 1;
    lock_release(&backend->lock);
    return CAMD_PROVIDER_OK;
}

static int backend_shutdown_ready(void *context)
{
    struct CAMDLegacyOutputBackend *backend = context;
    int ready;

    if (!backend)
        return 0;
    lock_acquire(&backend->lock);
    ready = backend->retiring && backend->active_sessions == 0;
    lock_release(&backend->lock);
    return ready;
}

static int valid_config(const struct CAMDLegacyOutputBackendConfigV1 *config)
{
    size_t i, j;

    if (!config || config->Size != sizeof(*config) ||
        config->Version != 1 ||
        config->Callbacks.Size != sizeof(config->Callbacks) ||
        config->Callbacks.Version != 1 || !config->Callbacks.Acquire ||
        !config->Callbacks.Release || !config->Callbacks.SubmitMIDI1 ||
        !config->Callbacks.SubmitMIDI1SysEx ||
        !config->Callbacks.AcquireInput != !config->Callbacks.ReleaseInput ||
        !config->Ports ||
        config->PortCount == 0 ||
        config->PortCount > SIZE_MAX / sizeof(*config->Ports) ||
        config->MaxQueueCapacity == 0 || config->MaxSysExBytes < 2 ||
        config->WorkerItemBudget == 0)
        return 0;
    for (i = 0; i < config->PortCount; ++i) {
        if (config->Ports[i].Size != sizeof(config->Ports[i]) ||
            config->Ports[i].Version != 1 ||
            id_is_zero(&config->Ports[i].EndpointID) ||
            !config->Ports[i].CapacityFanout)
            return 0;
        for (j = 0; j < i; ++j) {
            if (id_equal(&config->Ports[i].EndpointID,
                         &config->Ports[j].EndpointID))
                return 0;
        }
    }
    return 1;
}

enum CAMDProviderResult camd_legacy_output_backend_create(
    const struct CAMDLegacyOutputBackendConfigV1 *config,
    struct CAMDLegacyOutputBackend **backend_out)
{
    struct CAMDLegacyOutputBackend *backend;

    if (!backend_out)
        return CAMD_PROVIDER_INVALID;
    *backend_out = NULL;
    if (!valid_config(config))
        return CAMD_PROVIDER_INVALID;
    backend = backend_alloc(sizeof(*backend));
    if (!backend)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    if (!lock_init(&backend->lock)) {
        backend_free(backend);
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    backend->ports = backend_alloc(config->PortCount * sizeof(*backend->ports));
    backend->inputs = backend_alloc(config->PortCount *
                                    sizeof(*backend->inputs));
    if (!backend->ports || !backend->inputs) {
        backend_free(backend->inputs);
        backend_free(backend->ports);
        lock_destroy(&backend->lock);
        backend_free(backend);
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    memcpy(backend->ports, config->Ports,
           config->PortCount * sizeof(*backend->ports));
    backend->callbacks = config->Callbacks;
    backend->port_count = config->PortCount;
    backend->max_queue_capacity = config->MaxQueueCapacity;
    backend->max_sysex_bytes = config->MaxSysExBytes;
    backend->worker_item_budget = config->WorkerItemBudget;
    backend->ops.Size = sizeof(backend->ops);
    backend->ops.Version = 1;
    backend->ops.Open = backend_open;
    backend->ops.Close = backend_close;
    backend->ops.SendMIDI1 = backend_send_midi1;
    backend->ops.SendMIDI1SysEx = backend_send_sysex;
    if (config->Callbacks.AcquireInput) {
        backend->ops.StartReceive = backend_start_receive;
        backend->ops.StopReceive = backend_stop_receive;
    }
    backend->ops.Drain = backend_drain;
    backend->ops.Cancel = backend_cancel;
    backend->ops.BeginShutdown = backend_begin_shutdown;
    backend->ops.ShutdownReady = backend_shutdown_ready;
    *backend_out = backend;
    return CAMD_PROVIDER_OK;
}

const struct CAMDProviderOpsV1 *camd_legacy_output_backend_ops(
    struct CAMDLegacyOutputBackend *backend)
{
    return backend ? &backend->ops : NULL;
}

void *camd_legacy_output_backend_context(
    struct CAMDLegacyOutputBackend *backend)
{
    return backend;
}

enum CAMDProviderResult camd_legacy_output_backend_destroy(
    struct CAMDLegacyOutputBackend *backend)
{
    if (!backend)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&backend->lock);
    if (backend->active_sessions != 0) {
        lock_release(&backend->lock);
        return CAMD_PROVIDER_STATE;
    }
    lock_release(&backend->lock);
    backend_free(backend->inputs);
    backend_free(backend->ports);
    lock_destroy(&backend->lock);
    backend_free(backend);
    return CAMD_PROVIDER_OK;
}
