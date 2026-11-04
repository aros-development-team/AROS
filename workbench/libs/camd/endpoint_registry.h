#ifndef CAMD_ENDPOINT_REGISTRY_H
#define CAMD_ENDPOINT_REGISTRY_H

/*
 * Executable model of CAMD's private endpoint registry.
 *
 * This is deliberately not installed as a public header.  It validates the
 * data and lifetime invariants that must precede the version 43 ABI review.
 */

#include <stddef.h>
#include <stdint.h>

#define CAMD_ENDPOINT_NAME_BYTES       128u
#define CAMD_ENDPOINT_PRODUCT_BYTES    128u
#define CAMD_ENDPOINT_TRANSPORT_BYTES   32u
#define CAMD_TOPOLOGY_NAME_BYTES        64u
#define CAMD_ENDPOINT_WATCH_MAX_EVENTS  32u

#define CAMD_DATA_FORMAT_MIDI1           (1u << 0)
#define CAMD_DATA_FORMAT_UMP             (1u << 1)
#define CAMD_DATA_FORMAT_ALL             (CAMD_DATA_FORMAT_MIDI1 | \
                                          CAMD_DATA_FORMAT_UMP)

struct CAMDHandleV1 {
    uint32_t slot;
    uint32_t generation;
};

struct CAMDEndpointIDV1 {
    uint32_t word[4];
};

struct CAMDGenerationV1 {
    uint32_t high;
    uint32_t low;
};

enum CAMDEndpointStateV1 {
    CAMD_ENDPOINT_REGISTERED = 1,
    CAMD_ENDPOINT_DISCOVERING,
    CAMD_ENDPOINT_AVAILABLE,
    CAMD_ENDPOINT_OFFLINE,
    CAMD_ENDPOINT_RETIRING,
    CAMD_ENDPOINT_RETIRED
};

struct CAMDEndpointInfoV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 ID;
    struct CAMDEndpointIDV1 ProviderID;
    uint32_t State;
    uint32_t Flags;
    uint32_t IdentityKind;
    uint32_t NativeDataFormats;
    uint32_t ProtocolCapabilities;
    uint32_t CurrentProtocol;
    struct CAMDGenerationV1 Generation;
    char Name[CAMD_ENDPOINT_NAME_BYTES];
    char ProductInstance[CAMD_ENDPOINT_PRODUCT_BYTES];
    char Transport[CAMD_ENDPOINT_TRANSPORT_BYTES];
};

struct CAMDGroupInfoV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    uint32_t Group;
    uint32_t Flags;
    uint32_t Protocol;
    char Name[CAMD_TOPOLOGY_NAME_BYTES];
};

struct CAMDFunctionBlockInfoV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    uint32_t Number;
    uint32_t Flags;
    uint32_t FirstGroup;
    uint32_t GroupCount;
    char Name[CAMD_TOPOLOGY_NAME_BYTES];
};

enum CAMDEndpointWatchEventTypeV1 {
    CAMD_ENDPOINT_EVENT_ADDED = 1,
    CAMD_ENDPOINT_EVENT_UPDATED,
    CAMD_ENDPOINT_EVENT_OFFLINE,
    CAMD_ENDPOINT_EVENT_RETIRED,
    CAMD_ENDPOINT_EVENT_LOST
};

struct CAMDEndpointWatchEventV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDGenerationV1 Generation;
    struct CAMDEndpointIDV1 EndpointID;
    uint32_t Type;
};

enum CAMDRegistryResult {
    CAMD_REGISTRY_OK = 0,
    CAMD_REGISTRY_INVALID,
    CAMD_REGISTRY_NOMEM,
    CAMD_REGISTRY_DUPLICATE,
    CAMD_REGISTRY_STALE,
    CAMD_REGISTRY_STATE,
    CAMD_REGISTRY_RETIRED,
    CAMD_REGISTRY_RANGE,
    CAMD_REGISTRY_EMPTY,
    CAMD_REGISTRY_BUSY,
    CAMD_REGISTRY_CALLBACK_FAILED,
    CAMD_REGISTRY_UNSUPPORTED,
    CAMD_REGISTRY_QUEUE_FULL,
    CAMD_REGISTRY_TOO_LARGE
};

struct CAMDEndpointRegistry;
struct CAMDEndpointSnapshot;
struct CAMDEndpointWatch;
struct CAMDProviderDescriptorV1;
struct CAMDProviderOpenRequestV1;
struct CAMDProviderReceiveSinkV1;
struct CAMDMIDI1EventV1;
struct CAMDUMPEventV1;

struct CAMDRegistrySessionInfoV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t Direction;
    uint32_t DataFormat;
    uint32_t Protocol;
    uint32_t RequestedQueueCapacity;
    uint32_t EffectiveQueueCapacity;
    uint32_t MaxSysExBytes;
};

/* Registry operations serialize themselves.  Destroy still requires that no
 * operation is in flight.  Snapshot access is independent after
 * camd_registry_snapshot() returns. */

struct CAMDEndpointRegistry *camd_registry_create(void);
void camd_registry_destroy(struct CAMDEndpointRegistry *registry);

enum CAMDRegistryResult camd_registry_provider_register(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDProviderDescriptorV1 *descriptor,
    struct CAMDHandleV1 *provider);

enum CAMDRegistryResult camd_registry_provider_begin_retire(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider);

enum CAMDRegistryResult camd_registry_provider_release(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider);

enum CAMDRegistryResult camd_registry_session_open(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDHandleV1 *session);

enum CAMDRegistryResult camd_registry_session_info(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session,
    struct CAMDRegistrySessionInfoV1 *info);

enum CAMDRegistryResult camd_registry_session_send_midi1(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count);

enum CAMDRegistryResult camd_registry_session_send_midi1_sysex(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session,
    const uint8_t *bytes,
    size_t byte_count,
    uint32_t time_high,
    uint32_t time_low,
    uint32_t clock_domain,
    uint32_t flags);

enum CAMDRegistryResult camd_registry_session_send_ump(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session,
    const struct CAMDUMPEventV1 *events,
    size_t event_count);

enum CAMDRegistryResult camd_registry_session_start_receive(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session,
    const struct CAMDProviderReceiveSinkV1 *sink);

enum CAMDRegistryResult camd_registry_session_stop_receive(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session);

enum CAMDRegistryResult camd_registry_session_drain(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session);

enum CAMDRegistryResult camd_registry_session_cancel(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session);

enum CAMDRegistryResult camd_registry_session_close(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session);

enum CAMDRegistryResult camd_registry_publish(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider,
    const struct CAMDEndpointInfoV1 *endpoint,
    const struct CAMDGroupInfoV1 *groups,
    size_t group_count,
    const struct CAMDFunctionBlockInfoV1 *blocks,
    size_t block_count,
    struct CAMDHandleV1 *provider_lease);

enum CAMDRegistryResult camd_registry_replace(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_lease,
    const struct CAMDEndpointInfoV1 *endpoint,
    const struct CAMDGroupInfoV1 *groups,
    size_t group_count,
    const struct CAMDFunctionBlockInfoV1 *blocks,
    size_t block_count);

enum CAMDRegistryResult camd_registry_set_state(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_lease,
    uint32_t state);

enum CAMDRegistryResult camd_registry_acquire(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDEndpointIDV1 *id,
    struct CAMDHandleV1 *lease);

enum CAMDRegistryResult camd_registry_retire(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_lease);

enum CAMDRegistryResult camd_registry_release(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 lease);

enum CAMDRegistryResult camd_registry_snapshot(
    struct CAMDEndpointRegistry *registry,
    struct CAMDEndpointSnapshot **snapshot);

enum CAMDRegistryResult camd_registry_watch_start(
    struct CAMDEndpointRegistry *registry,
    size_t capacity,
    struct CAMDEndpointWatch **watch,
    struct CAMDGenerationV1 *generation);

enum CAMDRegistryResult camd_endpoint_watch_read(
    struct CAMDEndpointWatch *watch,
    struct CAMDEndpointWatchEventV1 *event);

void camd_endpoint_watch_end(struct CAMDEndpointWatch *watch);

void camd_snapshot_destroy(struct CAMDEndpointSnapshot *snapshot);
struct CAMDGenerationV1 camd_snapshot_generation(
    const struct CAMDEndpointSnapshot *snapshot);
size_t camd_snapshot_endpoint_count(const struct CAMDEndpointSnapshot *snapshot);

enum CAMDRegistryResult camd_snapshot_endpoint(
    const struct CAMDEndpointSnapshot *snapshot,
    size_t index,
    const struct CAMDEndpointInfoV1 **endpoint,
    const struct CAMDGroupInfoV1 **groups,
    size_t *group_count,
    const struct CAMDFunctionBlockInfoV1 **blocks,
    size_t *block_count);

#endif
