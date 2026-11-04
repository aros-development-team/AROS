#include "app_endpoint.h"

#include "native_event_queue.h"

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

struct app_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct app_session {
    struct CAMDAppEndpoint *endpoint;
    struct app_session *next;
    uint32_t direction;
    struct CAMDNativeEventQueue *queue;         /* output sessions */
    uint32_t capacity;                          /* of queue, in records */
    struct CAMDProviderReceiveSinkV1 sink;      /* input sessions */
    int receiving;
};

struct CAMDAppEndpoint {
    struct app_lock lock;
    struct CAMDAppEndpointConfigV1 config;
    struct app_session *sessions;       /* under lock */
    struct app_session *last_taken;     /* under lock; for taking in turn */
    /* What legacy senders sent: always the last in sessions, never counted
     * and never closed. NULL without the output direction. */
    struct app_session *projected;
    struct CAMDProviderReceiveSinkV1 projection;    /* under lock */
    int projecting;
    uint32_t session_count;
    int retiring;
    struct CAMDProviderOpsV1 ops;
};

static void *app_alloc(size_t size)
{
#ifdef __AROS__
    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, MEMF_ANY | MEMF_CLEAR);
#else
    return calloc(1, size);
#endif
}

static void app_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int lock_init(struct app_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore);
    return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}

static void lock_destroy(struct app_lock *lock)
{
#ifdef __AROS__
    (void)lock;
#else
    pthread_mutex_destroy(&lock->mutex);
#endif
}

static void lock_acquire(struct app_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}

static void lock_release(struct app_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
#endif
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

static void notify(struct CAMDAppEndpoint *endpoint)
{
    void (*call)(void *context);
    void *context;

    lock_acquire(&endpoint->lock);
    call = endpoint->config.Notify;
    context = endpoint->config.NotifyContext;
    lock_release(&endpoint->lock);
    if (call)
        call(context);
}

/* A UMP queue holds no MIDI 1.0 SysEx. */
static uint32_t sysex_bytes(const struct CAMDAppEndpoint *endpoint)
{
    return endpoint->config.DataFormat == CAMD_PROVIDER_FORMAT_UMP
               ? 0
               : endpoint->config.MaxSysExBytes;
}

static enum CAMDProviderResult app_open(
    void *context, const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDProviderOpenResultV1 *result)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session;
    struct CAMDNativeQueueConfigV1 queue_config;

    if (!endpoint || !request || !result)
        return CAMD_PROVIDER_INVALID;
    if ((request->Direction != CAMD_PROVIDER_DIRECTION_INPUT &&
         request->Direction != CAMD_PROVIDER_DIRECTION_OUTPUT) ||
        (request->Direction & ~endpoint->config.Directions) != 0 ||
        request->DataFormat != endpoint->config.DataFormat ||
        request->Protocol != endpoint->config.Protocol ||
        request->QueueCapacity == 0 ||
        request->QueueCapacity > endpoint->config.MaxQueueCapacity)
        return CAMD_PROVIDER_UNSUPPORTED;
    session = app_alloc(sizeof(*session));
    if (!session)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    session->endpoint = endpoint;
    session->direction = request->Direction;
    if (request->Direction == CAMD_PROVIDER_DIRECTION_OUTPUT) {
        memset(&queue_config, 0, sizeof(queue_config));
        queue_config.Size = sizeof(queue_config);
        queue_config.Version = 1;
        queue_config.DataFormat = endpoint->config.DataFormat;
        queue_config.Capacity = request->QueueCapacity;
        queue_config.MaxSysExBytes = sysex_bytes(endpoint);
        if (camd_native_queue_create(&queue_config, &session->queue) !=
            CAMD_NATIVE_QUEUE_OK) {
            app_free(session);
            return CAMD_PROVIDER_CALLBACK_FAILED;
        }
        session->capacity = request->QueueCapacity;
    }
    lock_acquire(&endpoint->lock);
    if (endpoint->retiring ||
        endpoint->session_count >= endpoint->config.MaxSessions) {
        int retiring = endpoint->retiring;

        lock_release(&endpoint->lock);
        camd_native_queue_destroy(session->queue);
        app_free(session);
        return retiring ? CAMD_PROVIDER_RETIRED : CAMD_PROVIDER_STATE;
    }
    session->next = endpoint->sessions;
    endpoint->sessions = session;
    ++endpoint->session_count;
    lock_release(&endpoint->lock);
    result->SessionContext = session;
    result->EffectiveQueueCapacity = request->QueueCapacity;
    result->MaxSysExBytes = sysex_bytes(endpoint);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult app_close(void *context, void *session_context)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = session_context, **link;

    if (!endpoint || !session || session->endpoint != endpoint)
        return CAMD_PROVIDER_INVALID;
    /* Out of the list under the lock: no emit or take reaches it after. */
    lock_acquire(&endpoint->lock);
    for (link = &endpoint->sessions; *link && *link != session;
         link = &(*link)->next)
        ;
    if (*link) {
        *link = session->next;
        --endpoint->session_count;
    }
    if (endpoint->last_taken == session)
        endpoint->last_taken = NULL;
    lock_release(&endpoint->lock);
    camd_native_queue_destroy(session->queue);
    app_free(session);
    return CAMD_PROVIDER_OK;
}

static struct app_session *output_session(struct CAMDAppEndpoint *endpoint,
                                          void *session_context)
{
    struct app_session *session = session_context;

    if (!endpoint || !session || session->endpoint != endpoint ||
        session->direction != CAMD_PROVIDER_DIRECTION_OUTPUT)
        return NULL;
    return session;
}

static enum CAMDProviderResult app_send_midi1(
    void *context, void *session_context,
    const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = output_session(endpoint, session_context);
    enum CAMDNativeQueueResult result;

    if (!session)
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_midi1_items(session->queue, events,
                                                   event_count);
    if (result != CAMD_NATIVE_QUEUE_OK)
        return queue_result(result);
    notify(endpoint);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult app_send_sysex(
    void *context, void *session_context, const uint8_t *bytes,
    size_t byte_count, uint32_t time_high, uint32_t time_low,
    uint32_t clock_domain, uint32_t flags)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = output_session(endpoint, session_context);
    enum CAMDNativeQueueResult result;

    if (!session)
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_midi1_sysex(
        session->queue, bytes, byte_count, time_high, time_low,
        clock_domain, flags);
    if (result != CAMD_NATIVE_QUEUE_OK)
        return queue_result(result);
    notify(endpoint);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult app_send_ump(
    void *context, void *session_context,
    const struct CAMDUMPEventV1 *events, size_t event_count)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = output_session(endpoint, session_context);
    enum CAMDNativeQueueResult result;
    size_t i;

    if (!session)
        return CAMD_PROVIDER_INVALID;
    /* One item per message, so that the publisher takes them one by one;
     * the room for all of them is checked first. */
    {
        struct CAMDNativeQueueStatsV1 stats;

        memset(&stats, 0, sizeof(stats));
        stats.Size = sizeof(stats);
        stats.Version = 1;
        if (camd_native_queue_stats(session->queue, &stats) !=
            CAMD_NATIVE_QUEUE_OK)
            return CAMD_PROVIDER_STATE;
        if (event_count > session->capacity - stats.PendingRecords)
            return CAMD_PROVIDER_QUEUE_FULL;
    }
    for (i = 0; i < event_count; ++i) {
        result = camd_native_queue_enqueue_ump(session->queue, &events[i], 1);
        if (result != CAMD_NATIVE_QUEUE_OK)
            return queue_result(result);
    }
    notify(endpoint);
    return CAMD_PROVIDER_OK;
}

/* QUEUE_FULL while the publisher has not taken everything yet. */
static enum CAMDProviderResult app_drain(void *context, void *session_context)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = output_session(endpoint, session_context);
    struct CAMDNativeQueueStatsV1 stats;

    if (!session)
        return CAMD_PROVIDER_INVALID;
    memset(&stats, 0, sizeof(stats));
    stats.Size = sizeof(stats);
    stats.Version = 1;
    if (camd_native_queue_stats(session->queue, &stats) !=
        CAMD_NATIVE_QUEUE_OK)
        return CAMD_PROVIDER_STATE;
    if (stats.PendingRecords == 0)
        return CAMD_PROVIDER_OK;
    notify(endpoint);
    return CAMD_PROVIDER_QUEUE_FULL;
}

static enum CAMDProviderResult app_cancel(void *context, void *session_context)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = output_session(endpoint, session_context);
    size_t cancelled;

    if (!session)
        return CAMD_PROVIDER_INVALID;
    return queue_result(camd_native_queue_cancel(session->queue, &cancelled));
}

static enum CAMDProviderResult app_start_receive(
    void *context, void *session_context,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = session_context;

    if (!endpoint || !session || session->endpoint != endpoint ||
        session->direction != CAMD_PROVIDER_DIRECTION_INPUT || !sink ||
        sink->Size != sizeof(*sink) || sink->Version != 1 ||
        (endpoint->config.DataFormat == CAMD_PROVIDER_FORMAT_UMP
             ? !sink->SubmitUMP
             : !sink->SubmitMIDI1 || !sink->SubmitMIDI1SysEx))
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&endpoint->lock);
    if (session->receiving) {
        lock_release(&endpoint->lock);
        return CAMD_PROVIDER_STATE;
    }
    session->sink = *sink;
    session->receiving = 1;
    lock_release(&endpoint->lock);
    return CAMD_PROVIDER_OK;
}

/* Emits hold the lock, so none is in a sink once this returns. */
static enum CAMDProviderResult app_stop_receive(void *context,
                                                void *session_context)
{
    struct CAMDAppEndpoint *endpoint = context;
    struct app_session *session = session_context;

    if (!endpoint || !session || session->endpoint != endpoint ||
        session->direction != CAMD_PROVIDER_DIRECTION_INPUT)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&endpoint->lock);
    session->receiving = 0;
    lock_release(&endpoint->lock);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult app_begin_shutdown(void *context)
{
    struct CAMDAppEndpoint *endpoint = context;

    if (!endpoint)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&endpoint->lock);
    endpoint->retiring = 1;
    lock_release(&endpoint->lock);
    return CAMD_PROVIDER_OK;
}

static int app_shutdown_ready(void *context)
{
    struct CAMDAppEndpoint *endpoint = context;
    int ready;

    if (!endpoint)
        return 0;
    lock_acquire(&endpoint->lock);
    ready = endpoint->retiring && endpoint->session_count == 0;
    lock_release(&endpoint->lock);
    return ready;
}

enum CAMDProviderResult camd_app_endpoint_create(
    const struct CAMDAppEndpointConfigV1 *config,
    struct CAMDAppEndpoint **endpoint_out)
{
    struct CAMDAppEndpoint *endpoint;

    if (!endpoint_out)
        return CAMD_PROVIDER_INVALID;
    *endpoint_out = NULL;
    if (!config || config->Size != sizeof(*config) || config->Version != 1 ||
        config->Directions == 0 ||
        (config->Directions & ~CAMD_PROVIDER_DIRECTION_ALL) != 0 ||
        config->MaxQueueCapacity == 0 || config->MaxSysExBytes < 2 ||
        config->MaxSessions == 0 ||
        ((config->DataFormat || config->Protocol) &&
         camd_provider_native_path(config->DataFormat, config->Protocol) ==
             0))
        return CAMD_PROVIDER_INVALID;
    endpoint = app_alloc(sizeof(*endpoint));
    if (!endpoint)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    if (!lock_init(&endpoint->lock)) {
        app_free(endpoint);
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    endpoint->config = *config;
    if (endpoint->config.DataFormat == 0) {
        endpoint->config.DataFormat = CAMD_PROVIDER_FORMAT_MIDI1;
        endpoint->config.Protocol = CAMD_PROVIDER_PROTOCOL_MIDI1;
    }
    if (endpoint->config.DataFormat == CAMD_PROVIDER_FORMAT_MIDI1 &&
        (config->Directions & CAMD_PROVIDER_DIRECTION_OUTPUT) != 0) {
        struct CAMDNativeQueueConfigV1 queue_config;

        memset(&queue_config, 0, sizeof(queue_config));
        queue_config.Size = sizeof(queue_config);
        queue_config.Version = 1;
        queue_config.DataFormat = CAMD_PROVIDER_FORMAT_MIDI1;
        queue_config.Capacity = config->MaxQueueCapacity;
        queue_config.MaxSysExBytes = config->MaxSysExBytes;
        endpoint->projected = app_alloc(sizeof(*endpoint->projected));
        if (!endpoint->projected ||
            camd_native_queue_create(&queue_config,
                                     &endpoint->projected->queue) !=
                CAMD_NATIVE_QUEUE_OK) {
            app_free(endpoint->projected);
            lock_destroy(&endpoint->lock);
            app_free(endpoint);
            return CAMD_PROVIDER_CALLBACK_FAILED;
        }
        endpoint->projected->endpoint = endpoint;
        endpoint->projected->direction = CAMD_PROVIDER_DIRECTION_OUTPUT;
        endpoint->sessions = endpoint->projected;
    }
    endpoint->ops.Size = sizeof(endpoint->ops);
    endpoint->ops.Version = 1;
    endpoint->ops.Open = app_open;
    endpoint->ops.Close = app_close;
    endpoint->ops.SendMIDI1 = app_send_midi1;
    endpoint->ops.SendMIDI1SysEx = app_send_sysex;
    endpoint->ops.SendUMP = app_send_ump;
    endpoint->ops.StartReceive = app_start_receive;
    endpoint->ops.StopReceive = app_stop_receive;
    endpoint->ops.Drain = app_drain;
    endpoint->ops.Cancel = app_cancel;
    endpoint->ops.BeginShutdown = app_begin_shutdown;
    endpoint->ops.ShutdownReady = app_shutdown_ready;
    *endpoint_out = endpoint;
    return CAMD_PROVIDER_OK;
}

const struct CAMDProviderOpsV1 *camd_app_endpoint_ops(
    struct CAMDAppEndpoint *endpoint)
{
    return endpoint ? &endpoint->ops : NULL;
}

void *camd_app_endpoint_context(struct CAMDAppEndpoint *endpoint)
{
    return endpoint;
}

enum CAMDProviderResult camd_app_endpoint_emit_midi1(
    struct CAMDAppEndpoint *endpoint, const struct CAMDMIDI1EventV1 *events,
    size_t event_count, uint32_t *dropped)
{
    struct app_session *session;
    enum CAMDProviderResult valid;
    uint32_t lost = 0;

    if (dropped)
        *dropped = 0;
    if (!endpoint ||
        endpoint->config.DataFormat != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_PROVIDER_INVALID;
    valid = camd_provider_validate_midi1(events, event_count);
    if (valid != CAMD_PROVIDER_OK)
        return valid;
    lock_acquire(&endpoint->lock);
    for (session = endpoint->sessions; session; session = session->next) {
        if (session->receiving &&
            session->sink.SubmitMIDI1(session->sink.Context, events,
                                      event_count) != CAMD_PROVIDER_OK)
            ++lost;
    }
    if (endpoint->projecting)
        endpoint->projection.SubmitMIDI1(endpoint->projection.Context,
                                         events, event_count);
    lock_release(&endpoint->lock);
    if (dropped)
        *dropped = lost;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_app_endpoint_emit_sysex(
    struct CAMDAppEndpoint *endpoint, const uint8_t *bytes, size_t byte_count,
    uint32_t *dropped)
{
    struct app_session *session;
    enum CAMDProviderResult valid;
    uint32_t lost = 0;

    if (dropped)
        *dropped = 0;
    if (!endpoint ||
        endpoint->config.DataFormat != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_PROVIDER_INVALID;
    valid = camd_provider_validate_midi1_sysex(bytes, byte_count);
    if (valid != CAMD_PROVIDER_OK)
        return valid;
    if (byte_count > endpoint->config.MaxSysExBytes)
        return CAMD_PROVIDER_TOO_LARGE;
    lock_acquire(&endpoint->lock);
    for (session = endpoint->sessions; session; session = session->next) {
        if (session->receiving &&
            session->sink.SubmitMIDI1SysEx(session->sink.Context, bytes,
                                           byte_count, 0, 0, 0, 0) !=
                CAMD_PROVIDER_OK)
            ++lost;
    }
    if (endpoint->projecting)
        endpoint->projection.SubmitMIDI1SysEx(endpoint->projection.Context,
                                              bytes, byte_count, 0, 0, 0, 0);
    lock_release(&endpoint->lock);
    if (dropped)
        *dropped = lost;
    return CAMD_PROVIDER_OK;
}

/* Lock held. The first output session after start that has a message. */
static struct app_session *next_with_message(
    struct CAMDAppEndpoint *endpoint, struct CAMDNativeQueueHeadV1 *head)
{
    struct app_session *first, *session;

    first = endpoint->last_taken && endpoint->last_taken->next
                ? endpoint->last_taken->next
                : endpoint->sessions;
    session = first;
    while (session) {
        if (session->queue) {
            memset(head, 0, sizeof(*head));
            head->Size = sizeof(*head);
            head->Version = 1;
            if (camd_native_queue_peek(session->queue, head) ==
                CAMD_NATIVE_QUEUE_OK)
                return session;
        }
        session = session->next ? session->next : endpoint->sessions;
        if (session == first)
            break;
    }
    return NULL;
}

enum CAMDProviderResult camd_app_endpoint_take(
    struct CAMDAppEndpoint *endpoint, struct CAMDMIDI1EventV1 *event,
    uint8_t *sysex, size_t sysex_capacity, size_t *sysex_count)
{
    struct app_session *session;
    struct CAMDNativeQueueHeadV1 head;
    struct CAMDNativeQueueSysExInfoV1 info;
    enum CAMDProviderResult result;
    size_t count = 0;

    if (!endpoint || !event || !sysex_count || (!sysex && sysex_capacity) ||
        endpoint->config.DataFormat != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_PROVIDER_INVALID;
    *sysex_count = 0;
    lock_acquire(&endpoint->lock);
    session = next_with_message(endpoint, &head);
    if (!session) {
        result = CAMD_PROVIDER_QUEUE_FULL;
    } else if (head.Kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX) {
        *sysex_count = head.ByteCount;
        if (sysex_capacity < head.ByteCount) {
            result = CAMD_PROVIDER_TOO_LARGE;
        } else {
            memset(&info, 0, sizeof(info));
            info.Size = sizeof(info);
            info.Version = 1;
            result = queue_result(camd_native_queue_dequeue_midi1_sysex(
                session->queue, sysex, sysex_capacity, &info));
            endpoint->last_taken = session;
        }
    } else {
        result = queue_result(camd_native_queue_dequeue_midi1(
            session->queue, event, 1, &count));
        endpoint->last_taken = session;
    }
    lock_release(&endpoint->lock);
    return result;
}

enum CAMDProviderResult camd_app_endpoint_emit_ump(
    struct CAMDAppEndpoint *endpoint, const struct CAMDUMPEventV1 *events,
    size_t event_count, uint32_t *dropped)
{
    struct app_session *session;
    enum CAMDProviderResult valid;
    uint32_t lost = 0;

    if (dropped)
        *dropped = 0;
    if (!endpoint || endpoint->config.DataFormat != CAMD_PROVIDER_FORMAT_UMP)
        return CAMD_PROVIDER_INVALID;
    valid = camd_provider_validate_ump(events, event_count);
    if (valid != CAMD_PROVIDER_OK)
        return valid;
    lock_acquire(&endpoint->lock);
    for (session = endpoint->sessions; session; session = session->next) {
        if (session->receiving &&
            session->sink.SubmitUMP(session->sink.Context, events,
                                    event_count) != CAMD_PROVIDER_OK)
            ++lost;
    }
    lock_release(&endpoint->lock);
    if (dropped)
        *dropped = lost;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_app_endpoint_take_ump(
    struct CAMDAppEndpoint *endpoint, struct CAMDUMPEventV1 *event)
{
    struct app_session *session;
    struct CAMDNativeQueueHeadV1 head;
    enum CAMDProviderResult result;
    size_t count = 0;

    if (!endpoint || !event ||
        endpoint->config.DataFormat != CAMD_PROVIDER_FORMAT_UMP)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&endpoint->lock);
    session = next_with_message(endpoint, &head);
    if (!session) {
        result = CAMD_PROVIDER_QUEUE_FULL;
    } else {
        result = queue_result(camd_native_queue_dequeue_ump(
            session->queue, event, 1, &count));
        endpoint->last_taken = session;
    }
    lock_release(&endpoint->lock);
    return result;
}

enum CAMDProviderResult camd_app_endpoint_inject_midi1(
    struct CAMDAppEndpoint *endpoint, const struct CAMDMIDI1EventV1 *event)
{
    enum CAMDNativeQueueResult result;

    if (!endpoint || !endpoint->projected)
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_midi1_items(
        endpoint->projected->queue, event, 1);
    if (result != CAMD_NATIVE_QUEUE_OK)
        return queue_result(result);
    notify(endpoint);
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_app_endpoint_inject_sysex(
    struct CAMDAppEndpoint *endpoint, const uint8_t *bytes,
    size_t byte_count)
{
    enum CAMDNativeQueueResult result;

    if (!endpoint || !endpoint->projected)
        return CAMD_PROVIDER_INVALID;
    result = camd_native_queue_enqueue_midi1_sysex(
        endpoint->projected->queue, bytes, byte_count, 0, 0, 0, 0);
    if (result != CAMD_NATIVE_QUEUE_OK)
        return queue_result(result);
    notify(endpoint);
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_app_endpoint_set_projection(
    struct CAMDAppEndpoint *endpoint,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    if (!endpoint ||
        (sink && (sink->Size != sizeof(*sink) || sink->Version != 1 ||
                  !sink->SubmitMIDI1 || !sink->SubmitMIDI1SysEx)))
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&endpoint->lock);
    if (sink)
        endpoint->projection = *sink;
    endpoint->projecting = sink != NULL;
    lock_release(&endpoint->lock);
    return CAMD_PROVIDER_OK;
}

void camd_app_endpoint_detach(struct CAMDAppEndpoint *endpoint)
{
    if (!endpoint)
        return;
    lock_acquire(&endpoint->lock);
    endpoint->config.Notify = NULL;
    endpoint->config.NotifyContext = NULL;
    lock_release(&endpoint->lock);
}

enum CAMDProviderResult camd_app_endpoint_destroy(
    struct CAMDAppEndpoint *endpoint)
{
    if (!endpoint)
        return CAMD_PROVIDER_INVALID;
    lock_acquire(&endpoint->lock);
    if (endpoint->session_count != 0) {
        lock_release(&endpoint->lock);
        return CAMD_PROVIDER_STATE;
    }
    lock_release(&endpoint->lock);
    if (endpoint->projected) {
        camd_native_queue_destroy(endpoint->projected->queue);
        app_free(endpoint->projected);
    }
    lock_destroy(&endpoint->lock);
    app_free(endpoint);
    return CAMD_PROVIDER_OK;
}
