/*
 * Endpoints that programs publish: PublishEndpoint() and its companions.
 *
 * A published endpoint is a provider like a driver port, with the publishing
 * program in the place of the hardware. Its object stays until the last
 * client session has closed, also when the publisher has withdrawn it.
 */

#include <proto/exec.h>

#include <string.h>

#include "camd_intern.h"
#include "app_endpoint.h"
#include "identity_map.h"
#include "identity_store_aros.h"
#include "legacy_driver_adapter.h"

#define NAMESPACE_APP_PROVIDER 0x41505256u  /* 'APRV' */
#define NAMESPACE_APP_ENDPOINT 0x41454e44u  /* 'AEND' */

/* Runs in the task of the client that sent something. */
static void publisher_notify(void *context)
{
    struct EndpointPublisher *publisher = context;

    if (publisher->signal != CAMD_SIGNAL_NONE)
        Signal(publisher->task, 1UL << publisher->signal);
}

static uint32_t publisher_now(void *context)
{
    (void)context;
    return CamdClock();
}

/* The publisher took a message: a client may be waiting for that. */
static void publisher_taken(void *context)
{
    NotifyEndpointDrained(context);
}

/* EndpointClientSemaphore must be held exclusively. Frees what withdrawn
 * endpoints no client holds any more. */
static void reap_publishers(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    ULONG i;

    for (i = 0; i < ENDPOINT_PUBLISHERS; ++i) {
        publisher = &base->EndpointPublishers[i];
        if (!publisher->withdrawn)
            continue;
        if (publisher->adapter != NULL &&
            camd_legacy_driver_adapter_release(publisher->adapter) ==
                CAMD_REGISTRY_OK)
            publisher->adapter = NULL;
        if (publisher->adapter == NULL &&
            camd_app_endpoint_destroy(publisher->app) == CAMD_PROVIDER_OK) {
            publisher->app = NULL;
            publisher->withdrawn = FALSE;
        }
    }
}

/* EndpointClientSemaphore must be held. */
static struct EndpointPublisher *find_publisher(
    const struct CAMDHandleV1 *handle, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;

    if (handle->slot >= ENDPOINT_PUBLISHERS || handle->generation == 0)
        return NULL;
    publisher = &base->EndpointPublishers[handle->slot];
    if (publisher->app == NULL || publisher->withdrawn ||
        publisher->generation != handle->generation)
        return NULL;
    return publisher;
}

/* Names are compared without regard to the case of ASCII letters, as a
 * driver's file name is: "Synth" and "synth" are one endpoint, with one
 * stored ID, and cannot be published side by side. */
static UBYTE fold_ascii(UBYTE byte)
{
    return (byte >= 'A' && byte <= 'Z') ? (UBYTE)(byte - 'A' + 'a') : byte;
}

static BOOL same_name(const char *a, const char *b)
{
    while (fold_ascii((UBYTE)*a) == fold_ascii((UBYTE)*b)) {
        if (*a == '\0')
            return TRUE;
        ++a;
        ++b;
    }
    return FALSE;
}

/* The same Name always gives the same key, so the same stored ID. */
static LONG resolve_name(struct CAMDIdentityMap *map, uint32_t namespace_id,
                         const char *name, BOOL temporary,
                         struct CAMDEndpointIDV1 *id, uint32_t *kind)
{
    struct CAMDIdentityKeyV1 key;
    struct CAMDIdentityResolutionV1 resolution;
    size_t length = strlen(name), n;

    memset(&key, 0, sizeof(key));
    key.Size = sizeof(key);
    key.Version = 1;
    key.Namespace = namespace_id;
    key.Confidence = CAMD_IDENTITY_CONFIGURED;
    key.Bytes[0] = 1;
    key.Bytes[1] = (uint8_t)length;
    for (n = 0; n < length; ++n)
        key.Bytes[2 + n] = fold_ascii((UBYTE)name[n]);
    key.ByteCount = (uint32_t)(2 + length);
    memset(&resolution, 0, sizeof(resolution));
    resolution.Size = sizeof(resolution);
    resolution.Version = 1;
    if (!temporary &&
        camd_identity_map_resolve(map, &key, &resolution) ==
            CAMD_IDENTITY_MAP_OK) {
        *id = resolution.ID;
        *kind = resolution.EffectiveConfidence;
        return CAMD_REGISTRY_OK;
    }
    /* Asked for, or the store is full: an ID for this time only. */
    if (camd_identity_map_temporary(map, id) != CAMD_IDENTITY_MAP_OK)
        return CAMD_REGISTRY_NOMEM;
    *kind = CAMD_IDENTITY_EPHEMERAL;
    return CAMD_REGISTRY_OK;
}

static BOOL driver_named(const char *name, struct CamdBase *CamdBase)
{
    struct Drivers *driver;
    BOOL found = FALSE;

    ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
    for (driver = CB(CamdBase)->drivers; driver; driver = driver->next) {
        if (same_name(driver->mididevicedata->Name, name)) {
            found = TRUE;
            break;
        }
    }
    ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
    return found;
}

/* Whether a published endpoint has that name, for LoadDriver(). A driver is
 * in CAMD's list before this is asked, and PublishAppEndpoint() looks at
 * that list with the semaphore held, so one of the two always sees the
 * other. */
BOOL AppEndpointNamed(const char *name, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    BOOL found = FALSE;
    ULONG i;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    for (i = 0; i < ENDPOINT_PUBLISHERS; ++i) {
        if (base->EndpointPublishers[i].projection != NULL &&
            same_name(EndpointProjectionName(
                          base->EndpointPublishers[i].projection), name)) {
            found = TRUE;
            break;
        }
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return found;
}

static BOOL terminated(const char *text, size_t size)
{
    return memchr(text, '\0', size) != NULL;
}

/* A name CAMD can show and, for a MIDI 1.0 endpoint, build cluster names
 * from: printable, and short enough that "<name>.out.0" fits where cluster
 * names are reported whole. */
static BOOL usable_name(const char *name, ULONG format)
{
    size_t length = 0;

    if (!terminated(name, CAMD_ENDPOINT_NAME_BYTES))
        return FALSE;
    for (; name[length] != '\0'; ++length) {
        UBYTE byte = (UBYTE)name[length];

        if (byte < 0x20 || byte == 0x7f)
            return FALSE;
    }
    if (length == 0 || name[0] == ' ' || name[length - 1] == ' ')
        return FALSE;
    return format != CAMD_DATA_FORMAT_MIDI1 ||
           length <= CWE_NAMESIZE - sizeof(".out.0");
}

/* The calling task is the one that gets the signal. */
LONG PublishAppEndpoint(const struct CAMDPublishRequestV1 *request,
                        struct CAMDHandleV1 *handle,
                        struct CAMDEndpointIDV1 *id,
                        struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher = NULL;
    struct CAMDIdentityMap *map;
    struct CAMDAppEndpointConfigV1 config;
    struct CAMDLegacyPortDescriptorV1 port;
    struct CAMDLegacyDriverDescriptorV1 descriptor;
    struct CAMDPublishRequestV1 taken;
    uint32_t provider_kind, endpoint_kind;
    BOOL temporary;
    LONG result;
    ULONG i;

    /* The record first ended with ProductInstance, and an endpoint was a
     * MIDI 1.0 one: that is what a record without DataFormat asks for. One
     * that has the field has to say what it wants. */
    if (request->Version != 1 ||
        !TakeEndpointRecord((const APTR)request, &taken, sizeof(taken),
                            offsetof(struct CAMDPublishRequestV1, DataFormat)))
        return CAMD_REGISTRY_INVALID;
    if (request->Size <= offsetof(struct CAMDPublishRequestV1, DataFormat)) {
        taken.DataFormat = CAMD_DATA_FORMAT_MIDI1;
        taken.Protocol = CAMD_PROTOCOL_MIDI1;
    }
    request = &taken;
    if (request->Directions == 0 ||
        (request->Directions & ~CAMD_PROVIDER_DIRECTION_ALL) != 0 ||
        (request->Flags &
         ~(CAMD_PUBLISH_TEMPORARY | CAMD_PUBLISH_HOLD_UNTIL_DUE)) != 0 ||
        ((request->Flags & CAMD_PUBLISH_HOLD_UNTIL_DUE) != 0 &&
         request->DataFormat != CAMD_DATA_FORMAT_MIDI1) ||
        !usable_name(request->Name, request->DataFormat) ||
        (request->Signal != CAMD_SIGNAL_NONE && request->Signal > 31) ||
        !terminated(request->ProductInstance,
                    sizeof(request->ProductInstance)) ||
        camd_provider_native_path(request->DataFormat, request->Protocol) ==
            0)
        return CAMD_REGISTRY_INVALID;
    /* Without the identity store there is no stable ID to give. */
    map = camd_identity_store_aros_map(base->EndpointIdentity);
    if (map == NULL)
        return CAMD_REGISTRY_STATE;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    reap_publishers(CamdBase);
    /* A loaded driver of that name already owns the clusters. */
    if (driver_named(request->Name, CamdBase)) {
        result = CAMD_REGISTRY_DUPLICATE;
        goto out;
    }
    /* So does an endpoint published under that name, whatever its ID. */
    for (i = 0; i < ENDPOINT_PUBLISHERS; ++i) {
        if (base->EndpointPublishers[i].app != NULL &&
            !base->EndpointPublishers[i].withdrawn &&
            base->EndpointPublishers[i].projection != NULL &&
            same_name(EndpointProjectionName(
                          base->EndpointPublishers[i].projection),
                      request->Name)) {
            result = CAMD_REGISTRY_DUPLICATE;
            goto out;
        }
    }
    for (i = 0; i < ENDPOINT_PUBLISHERS; ++i) {
        if (base->EndpointPublishers[i].app == NULL) {
            publisher = &base->EndpointPublishers[i];
            break;
        }
    }
    if (publisher == NULL) {
        result = CAMD_REGISTRY_NOMEM;
        goto out;
    }

    memset(&port, 0, sizeof(port));
    port.Size = sizeof(port);
    port.Version = 1;
    port.Directions = request->Directions;
    strcpy(port.Name, request->Name);
    strcpy(port.ProductInstance, request->ProductInstance);
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.Size = sizeof(descriptor);
    descriptor.Version = 1;
    temporary = (request->Flags & CAMD_PUBLISH_TEMPORARY) != 0;
    result = resolve_name(map, NAMESPACE_APP_PROVIDER, request->Name,
                          temporary, &descriptor.ProviderID, &provider_kind);
    /* Both stored or both temporary: an endpoint whose provider's ID did
     * not fit in the store does not take a place there either. */
    if (result == CAMD_REGISTRY_OK)
        result = resolve_name(map, NAMESPACE_APP_ENDPOINT, request->Name,
                              temporary ||
                                  provider_kind == CAMD_IDENTITY_EPHEMERAL,
                              &port.EndpointID, &endpoint_kind);
    if (result != CAMD_REGISTRY_OK)
        goto out;

    memset(&config, 0, sizeof(config));
    config.Size = sizeof(config);
    config.Version = 1;
    config.Directions = request->Directions;
    config.MaxQueueCapacity = ENDPOINT_OUTPUT_QUEUE;
    config.MaxSysExBytes = ENDPOINT_OUTPUT_SYSEX;
    config.LimitSysExBytes = ENDPOINT_SYSEX_LIMIT;
    config.MaxSessions = ENDPOINT_OUTPUT_SESSIONS;
    config.DataFormat = request->DataFormat;
    config.Protocol = request->Protocol;
    config.Notify = publisher_notify;
    config.NotifyContext = publisher;
    config.Taken = publisher_taken;
    config.TakenContext = CamdBase;
    if ((request->Flags & CAMD_PUBLISH_HOLD_UNTIL_DUE) != 0)
        config.Now = publisher_now;
    if (camd_app_endpoint_create(&config, &publisher->app) !=
        CAMD_PROVIDER_OK) {
        publisher->app = NULL;
        result = CAMD_REGISTRY_NOMEM;
        goto out;
    }
    publisher->task = FindTask(NULL);
    publisher->signal = request->Signal;
    publisher->withdrawn = FALSE;
    if (++publisher->generation == 0)
        ++publisher->generation;

    descriptor.IdentityKind = endpoint_kind;
    descriptor.ProtocolCapabilities = request->Protocol;
    descriptor.DataFormat = request->DataFormat;
    descriptor.Protocol = request->Protocol;
    descriptor.BackendContext = camd_app_endpoint_context(publisher->app);
    descriptor.BackendOps = camd_app_endpoint_ops(publisher->app);
    descriptor.Ports = &port;
    descriptor.PortCount = 1;
    descriptor.Transport = "camd-app";
    result = camd_legacy_driver_adapter_create(base->EndpointRegistry,
                                               &descriptor,
                                               &publisher->adapter);
    if (result != CAMD_REGISTRY_OK) {
        /* A half-published adapter can still be pinned: withdraw it and
         * let the slot be reaped like any other. */
        camd_app_endpoint_detach(publisher->app);
        if (publisher->adapter != NULL)
            camd_legacy_driver_adapter_begin_retire(publisher->adapter);
        publisher->withdrawn = TRUE;
        reap_publishers(CamdBase);
        goto out;
    }
    /* A MIDI 1.0 endpoint's legacy clusters. Without them old programs
     * could not reach it, so it is not left published half-way. UMP has no
     * cluster form. */
    if (request->DataFormat == CAMD_DATA_FORMAT_MIDI1)
        publisher->projection = CreateEndpointProjection(
            publisher->app, request->Name, request->Directions, CamdBase);
    if (request->DataFormat == CAMD_DATA_FORMAT_MIDI1 &&
        publisher->projection == NULL) {
        camd_app_endpoint_detach(publisher->app);
        camd_legacy_driver_adapter_begin_retire(publisher->adapter);
        publisher->withdrawn = TRUE;
        reap_publishers(CamdBase);
        result = CAMD_REGISTRY_NOMEM;
        goto out;
    }
    handle->slot = i;
    handle->generation = publisher->generation;
    if (id != NULL)
        *id = port.EndpointID;

out:
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

void WithdrawAppEndpoint(const struct CAMDHandleV1 *handle,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL) {
        DeleteEndpointProjection(publisher->projection);
        publisher->projection = NULL;
        camd_app_endpoint_detach(publisher->app);
        camd_legacy_driver_adapter_begin_retire(publisher->adapter);
        publisher->withdrawn = TRUE;
        /* Whoever waits for its sessions to drain will wait in vain. */
        NotifyEndpointDrained(CamdBase);
    }
    reap_publishers(CamdBase);
    ReleaseSemaphore(&base->EndpointClientSemaphore);
}

LONG SetAppEndpointState(const struct CAMDHandleV1 *handle, ULONG state,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    LONG result = CAMD_REGISTRY_STALE;

    /* Exclusive: the endpoint's clusters change with its state. */
    ObtainSemaphore(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL) {
        result = camd_legacy_driver_adapter_set_state(publisher->adapter, 0,
                                                      state);
        if (result == CAMD_REGISTRY_OK &&
            !SetEndpointProjectionOnline(publisher->projection,
                                         state == CAMD_ENDPOINT_AVAILABLE)) {
            camd_legacy_driver_adapter_set_state(publisher->adapter, 0,
                                                 CAMD_ENDPOINT_OFFLINE);
            result = CAMD_REGISTRY_NOMEM;
        }
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

static LONG emit_result(enum CAMDProviderResult result)
{
    if (result == CAMD_PROVIDER_OK)
        return CAMD_REGISTRY_OK;
    if (result == CAMD_PROVIDER_TOO_LARGE)
        return CAMD_REGISTRY_TOO_LARGE;
    if (result == CAMD_PROVIDER_UNSUPPORTED)
        return CAMD_REGISTRY_UNSUPPORTED;
    return CAMD_REGISTRY_INVALID;
}

LONG PutAppEndpointMidi(const struct CAMDHandleV1 *handle,
                        const struct CAMDMIDI1EventV1 *events, ULONG count,
                        struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL)
        result = emit_result(camd_app_endpoint_emit_midi1(
            publisher->app, events, count, NULL));
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

LONG PutAppEndpointSysEx(const struct CAMDHandleV1 *handle,
                         const UBYTE *bytes, ULONG length,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL)
        result = emit_result(camd_app_endpoint_emit_sysex(
            publisher->app, bytes, length, NULL));
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

LONG GetAppEndpointMidi(const struct CAMDHandleV1 *handle,
                        struct CAMDMIDI1EventV1 *event, UBYTE *sysex,
                        ULONG size, ULONG *length, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    enum CAMDProviderResult taken;
    size_t count = 0;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL) {
        taken = camd_app_endpoint_take(publisher->app, event, sysex, size,
                                       &count);
        *length = (ULONG)count;
        if (taken == CAMD_PROVIDER_OK)
            result = CAMD_REGISTRY_OK;
        else if (taken == CAMD_PROVIDER_TOO_LARGE)
            result = CAMD_REGISTRY_TOO_LARGE;
        else if (taken == CAMD_PROVIDER_QUEUE_FULL)
            result = CAMD_REGISTRY_EMPTY;
        else
            result = CAMD_REGISTRY_UNSUPPORTED;
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

LONG PutAppEndpointUMP(const struct CAMDHandleV1 *handle,
                       const struct CAMDUMPEventV1 *events, ULONG count,
                       struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL)
        result = emit_result(camd_app_endpoint_emit_ump(
            publisher->app, events, count, NULL));
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

LONG GetAppEndpointUMP(const struct CAMDHandleV1 *handle,
                       struct CAMDUMPEventV1 *event,
                       struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    enum CAMDProviderResult taken;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL) {
        taken = camd_app_endpoint_take_ump(publisher->app, event);
        if (taken == CAMD_PROVIDER_OK)
            result = CAMD_REGISTRY_OK;
        else if (taken == CAMD_PROVIDER_QUEUE_FULL)
            result = CAMD_REGISTRY_EMPTY;
        else
            result = CAMD_REGISTRY_UNSUPPORTED;
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

LONG SetAppEndpointTopology(const struct CAMDHandleV1 *handle,
                            struct CAMDGroupInfoV1 *groups, ULONG group_count,
                            struct CAMDFunctionBlockInfoV1 *blocks,
                            ULONG block_count, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL)
        result = camd_legacy_driver_adapter_set_topology(
            publisher->adapter, 0, groups, group_count, blocks, block_count);
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

LONG GetAppEndpointStats(const struct CAMDHandleV1 *handle,
                         struct CAMDPublishedStatsV1 *stats,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointPublisher *publisher;
    LONG result = CAMD_REGISTRY_STALE;

    ObtainSemaphoreShared(&base->EndpointClientSemaphore);
    publisher = find_publisher(handle, CamdBase);
    if (publisher != NULL)
        result = camd_app_endpoint_stats(publisher->app, stats) ==
                         CAMD_PROVIDER_OK
                     ? CAMD_REGISTRY_OK
                     : CAMD_REGISTRY_INVALID;
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

/* Called from expunge with the library closed. A published endpoint, or a
 * withdrawn one a client still has a session with, keeps CAMD loaded. */
BOOL AppEndpointsRemain(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG i;

    reap_publishers(CamdBase);
    for (i = 0; i < ENDPOINT_PUBLISHERS; ++i) {
        if (base->EndpointPublishers[i].app != NULL)
            return TRUE;
    }
    return FALSE;
}
