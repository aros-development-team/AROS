/*
 * Client-side state of the endpoint functions: snapshot handles and the
 * queues of input sessions.
 *
 * A client holds a slot and a generation, never an address, so a stale or
 * foreign handle is refused instead of followed.
 */

#include <proto/exec.h>

#include <string.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "native_event_queue.h"

LONG AddEndpointSnapshot(struct CAMDEndpointSnapshot *snapshot,
                         struct CAMDHandleV1 *handle,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    LONG result = CAMD_REGISTRY_NOMEM;
    ULONG i;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    for (i = 0; i < ENDPOINT_SNAPSHOTS; ++i) {
        if (base->EndpointSnapshots[i] == NULL) {
            if (++base->EndpointSnapshotGenerations[i] == 0)
                ++base->EndpointSnapshotGenerations[i];
            base->EndpointSnapshots[i] = snapshot;
            handle->slot = i;
            handle->generation = base->EndpointSnapshotGenerations[i];
            result = CAMD_REGISTRY_OK;
            break;
        }
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

/* EndpointClientSemaphore must be held; the snapshot is good while it is. */
struct CAMDEndpointSnapshot *FindEndpointSnapshot(
    const struct CAMDHandleV1 *handle, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);

    if (handle->slot >= ENDPOINT_SNAPSHOTS || handle->generation == 0 ||
        base->EndpointSnapshotGenerations[handle->slot] != handle->generation)
        return NULL;
    return base->EndpointSnapshots[handle->slot];
}

void RemoveEndpointSnapshot(const struct CAMDHandleV1 *handle,
                            struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct CAMDEndpointSnapshot *snapshot;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    snapshot = FindEndpointSnapshot(handle, CamdBase);
    if (snapshot != NULL)
        base->EndpointSnapshots[handle->slot] = NULL;
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    if (snapshot != NULL)
        camd_snapshot_destroy(snapshot);
}

/* Called from expunge with the library closed, so no client is inside CAMD
 * and the snapshot table needs no lock. */
BOOL EndpointClientsRemain(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG i;

    for (i = 0; i < ENDPOINT_SNAPSHOTS; ++i) {
        if (base->EndpointSnapshots[i] != NULL)
            return TRUE;
    }
    for (i = 0; i < ENDPOINT_WATCHES; ++i) {
        if (base->EndpointWatchers[i].watch != NULL)
            return TRUE;
    }
    return camd_registry_session_count(base->EndpointRegistry) != 0;
}

/* Snapshots a client did not release. CAMD must be expunging. */
void FreeEndpointSnapshots(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG i;

    for (i = 0; i < ENDPOINT_SNAPSHOTS; ++i) {
        if (base->EndpointSnapshots[i] != NULL) {
            camd_snapshot_destroy(base->EndpointSnapshots[i]);
            base->EndpointSnapshots[i] = NULL;
        }
    }
}

static enum CAMDProviderResult queue_to_provider(
    enum CAMDNativeQueueResult result)
{
    if (result == CAMD_NATIVE_QUEUE_OK)
        return CAMD_PROVIDER_OK;
    if (result == CAMD_NATIVE_QUEUE_TOO_LARGE)
        return CAMD_PROVIDER_TOO_LARGE;
    return CAMD_PROVIDER_QUEUE_FULL;
}

static void receiver_arrived(struct EndpointReceiver *receiver,
                             enum CAMDNativeQueueResult result)
{
    if (result != CAMD_NATIVE_QUEUE_OK)
        receiver->lost = TRUE;
    if (receiver->signal != CAMD_SIGNAL_NONE)
        Signal(receiver->task, 1UL << receiver->signal);
}

/* The two sinks run in the port's receiver process. */
static enum CAMDProviderResult receiver_midi1(
    void *context, const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct EndpointReceiver *receiver = context;
    enum CAMDNativeQueueResult result;

    result = camd_native_queue_enqueue_midi1_items(receiver->queue, events,
                                                   event_count);
    receiver_arrived(receiver, result);
    return queue_to_provider(result);
}

static enum CAMDProviderResult receiver_sysex(
    void *context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags)
{
    struct EndpointReceiver *receiver = context;
    enum CAMDNativeQueueResult result;

    result = camd_native_queue_enqueue_midi1_sysex(
        receiver->queue, bytes, byte_count, time_high, time_low,
        clock_domain, flags);
    receiver_arrived(receiver, result);
    return queue_to_provider(result);
}

/* EndpointClientSemaphore must be held. */
static struct EndpointReceiver *find_receiver(
    const struct CAMDHandleV1 *session, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG i;

    if (session->generation == 0)
        return NULL;
    for (i = 0; i < ENDPOINT_RECEIVERS; ++i) {
        if (base->EndpointReceivers[i].session.generation ==
                session->generation &&
            base->EndpointReceivers[i].session.slot == session->slot)
            return &base->EndpointReceivers[i];
    }
    return NULL;
}

/* Gives an open input session its queue and starts it receiving. The
 * calling task is the one that gets the signal. */
LONG StartEndpointReceiver(const struct CAMDHandleV1 *session, ULONG capacity,
                           ULONG maxsysex, ULONG signal,
                           struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointReceiver *receiver = NULL;
    struct CAMDNativeQueueConfigV1 config;
    struct CAMDProviderReceiveSinkV1 sink;
    LONG result;
    ULONG i;

    if (signal != CAMD_SIGNAL_NONE && signal > 31)
        return CAMD_REGISTRY_INVALID;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    for (i = 0; i < ENDPOINT_RECEIVERS; ++i) {
        if (base->EndpointReceivers[i].session.generation == 0) {
            receiver = &base->EndpointReceivers[i];
            break;
        }
    }
    if (receiver == NULL) {
        ReleaseSemaphore(&base->EndpointClientSemaphore);
        return CAMD_REGISTRY_NOMEM;
    }
    memset(&config, 0, sizeof(config));
    config.Size = sizeof(config);
    config.Version = 1;
    config.DataFormat = CAMD_PROVIDER_FORMAT_MIDI1;
    config.Capacity = capacity;
    config.MaxSysExBytes = maxsysex;
    if (camd_native_queue_create(&config, &receiver->queue) !=
        CAMD_NATIVE_QUEUE_OK) {
        ReleaseSemaphore(&base->EndpointClientSemaphore);
        return CAMD_REGISTRY_NOMEM;
    }
    receiver->task = FindTask(NULL);
    receiver->signal = signal;
    receiver->lost = FALSE;
    receiver->session = *session;

    memset(&sink, 0, sizeof(sink));
    sink.Size = sizeof(sink);
    sink.Version = 1;
    sink.Context = receiver;
    sink.SubmitMIDI1 = receiver_midi1;
    sink.SubmitMIDI1SysEx = receiver_sysex;
    result = camd_registry_session_start_receive(base->EndpointRegistry,
                                                 *session, &sink);
    if (result != CAMD_REGISTRY_OK) {
        receiver->session.generation = 0;
        camd_native_queue_destroy(receiver->queue);
        receiver->queue = NULL;
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

/* Stops a session receiving and frees its queue; nothing for an output
 * session. The semaphore is held throughout, so the slot is not reused
 * while a delivery could still be in its sink. */
void StopEndpointReceiver(const struct CAMDHandleV1 *session,
                          struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointReceiver *receiver;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    receiver = find_receiver(session, CamdBase);
    if (receiver != NULL) {
        camd_registry_session_stop_receive(base->EndpointRegistry, *session);
        camd_native_queue_destroy(receiver->queue);
        receiver->queue = NULL;
        receiver->session.generation = 0;
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
}

LONG ReadEndpointReceiver(const struct CAMDHandleV1 *session,
                          struct CAMDMIDI1EventV1 *event, UBYTE *sysex,
                          ULONG size, ULONG *length,
                          struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointReceiver *receiver;
    struct CAMDNativeQueueHeadV1 head;
    struct CAMDNativeQueueSysExInfoV1 info;
    enum CAMDNativeQueueResult got;
    size_t count = 0;
    LONG result;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    receiver = find_receiver(session, CamdBase);
    if (receiver == NULL) {
        result = CAMD_REGISTRY_STALE;
    } else if (receiver->lost) {
        receiver->lost = FALSE;
        result = CAMD_REGISTRY_QUEUE_FULL;
    } else {
        memset(&head, 0, sizeof(head));
        head.Size = sizeof(head);
        head.Version = 1;
        got = camd_native_queue_peek(receiver->queue, &head);
        if (got != CAMD_NATIVE_QUEUE_OK) {
            result = CAMD_REGISTRY_EMPTY;
        } else if (head.Kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX) {
            *length = head.ByteCount;
            if (sysex == NULL || size < head.ByteCount) {
                result = CAMD_REGISTRY_TOO_LARGE;
            } else {
                memset(&info, 0, sizeof(info));
                info.Size = sizeof(info);
                info.Version = 1;
                got = camd_native_queue_dequeue_midi1_sysex(
                    receiver->queue, sysex, size, &info);
                result = got == CAMD_NATIVE_QUEUE_OK ? CAMD_REGISTRY_OK
                                                     : CAMD_REGISTRY_EMPTY;
            }
        } else {
            got = camd_native_queue_dequeue_midi1(receiver->queue, event, 1,
                                                  &count);
            result = got == CAMD_NATIVE_QUEUE_OK && count == 1
                         ? CAMD_REGISTRY_OK
                         : CAMD_REGISTRY_EMPTY;
        }
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

/* Runs with the registry locked, in whatever task changed an endpoint. */
static void watcher_notify(void *context)
{
    struct EndpointWatcher *watcher = context;

    if (watcher->signal != CAMD_SIGNAL_NONE)
        Signal(watcher->task, 1UL << watcher->signal);
}

/* The calling task is the one that gets the signal. */
LONG StartEndpointWatcher(struct CAMDHandleV1 *handle, ULONG signal,
                          struct CAMDGenerationV1 *generation,
                          struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointWatcher *watcher = NULL;
    struct CAMDGenerationV1 started;
    LONG result;
    ULONG i;

    if (signal != CAMD_SIGNAL_NONE && signal > 31)
        return CAMD_REGISTRY_INVALID;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    for (i = 0; i < ENDPOINT_WATCHES; ++i) {
        if (base->EndpointWatchers[i].watch == NULL) {
            watcher = &base->EndpointWatchers[i];
            break;
        }
    }
    if (watcher == NULL) {
        ReleaseSemaphore(&base->EndpointClientSemaphore);
        return CAMD_REGISTRY_NOMEM;
    }
    result = camd_registry_watch_start(base->EndpointRegistry,
                                       ENDPOINT_WATCH_EVENTS,
                                       &watcher->watch, &started);
    if (result == CAMD_REGISTRY_OK) {
        if (++watcher->generation == 0)
            ++watcher->generation;
        watcher->task = FindTask(NULL);
        watcher->signal = signal;
        camd_endpoint_watch_set_notify(watcher->watch, watcher_notify,
                                       watcher);
        handle->slot = i;
        handle->generation = watcher->generation;
        if (generation != NULL)
            *generation = started;
    } else {
        watcher->watch = NULL;
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

/* EndpointClientSemaphore must be held. */
static struct EndpointWatcher *find_watcher(
    const struct CAMDHandleV1 *handle, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);

    if (handle->slot >= ENDPOINT_WATCHES || handle->generation == 0 ||
        base->EndpointWatchers[handle->slot].watch == NULL ||
        base->EndpointWatchers[handle->slot].generation != handle->generation)
        return NULL;
    return &base->EndpointWatchers[handle->slot];
}

LONG ReadEndpointWatcher(const struct CAMDHandleV1 *handle,
                         struct CAMDEndpointWatchEventV1 *event,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointWatcher *watcher;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    watcher = find_watcher(handle, CamdBase);
    if (watcher != NULL)
        result = camd_endpoint_watch_read(watcher->watch, event);
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

void EndEndpointWatcher(const struct CAMDHandleV1 *handle,
                        struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointWatcher *watcher;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    watcher = find_watcher(handle, CamdBase);
    if (watcher != NULL) {
        /* Ending it takes the registry lock, so no notify is running or
         * comes afterwards. */
        camd_endpoint_watch_end(watcher->watch);
        watcher->watch = NULL;
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
}
