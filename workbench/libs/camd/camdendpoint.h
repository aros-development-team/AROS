#ifndef MIDI_CAMDENDPOINT_H
#define MIDI_CAMDENDPOINT_H

/*
 * CAMD endpoint client records.
 *
 * PROVISIONAL: this is the camd.library 43 client surface under review.
 * Names, values and layouts can still change. Every record starts with Size
 * and Version, holds no pointers and has the same layout on every target.
 */

#ifdef __AROS__
#   include <exec/types.h>
#else
#   include <stdint.h>
typedef uint32_t ULONG;
typedef uint8_t UBYTE;
#endif

#define CAMD_ENDPOINT_NAME_BYTES       128u
#define CAMD_ENDPOINT_PRODUCT_BYTES    128u
#define CAMD_ENDPOINT_TRANSPORT_BYTES   32u

/* The container a session carries, fixed when it is opened. */
#define CAMD_DATA_FORMAT_MIDI1           (1u << 0)
#define CAMD_DATA_FORMAT_UMP             (1u << 1)
#define CAMD_DATA_FORMAT_ALL             (CAMD_DATA_FORMAT_MIDI1 | \
                                          CAMD_DATA_FORMAT_UMP)

/* The MIDI protocol carried in that container. */
#define CAMD_PROTOCOL_MIDI1              1u
#define CAMD_PROTOCOL_MIDI2              2u

/* A session has exactly one direction. */
#define CAMD_DIRECTION_INPUT             (1u << 0)
#define CAMD_DIRECTION_OUTPUT            (1u << 1)

/* CAMDSessionRequestV1.Conversion. Only CAMD_CONVERSION_NONE exists yet. */
#define CAMD_CONVERSION_NONE             0u

/* A runtime capability, passed by address and never stored on disk. All
 * zero is invalid. */
struct CAMDHandleV1 {
    ULONG slot;
    ULONG generation;
};

/* An opaque stable system ID: compare or store all 16 bytes. */
struct CAMDEndpointIDV1 {
    ULONG word[4];
};

struct CAMDGenerationV1 {
    ULONG high;
    ULONG low;
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
    ULONG Size;
    ULONG Version;
    struct CAMDEndpointIDV1 ID;
    struct CAMDEndpointIDV1 ProviderID;
    ULONG State;
    ULONG Flags;
    ULONG IdentityKind;
    ULONG NativeDataFormats;
    ULONG ProtocolCapabilities;
    ULONG CurrentProtocol;
    struct CAMDGenerationV1 Generation;
    char Name[CAMD_ENDPOINT_NAME_BYTES];
    char ProductInstance[CAMD_ENDPOINT_PRODUCT_BYTES];
    char Transport[CAMD_ENDPOINT_TRANSPORT_BYTES];
};

/* A change to the endpoints, read from a watch. After CAMD_ENDPOINT_EVENT_LOST
 * events are missing: take a new snapshot. */
enum CAMDEndpointWatchEventTypeV1 {
    CAMD_ENDPOINT_EVENT_ADDED = 1,
    CAMD_ENDPOINT_EVENT_UPDATED,
    CAMD_ENDPOINT_EVENT_OFFLINE,
    CAMD_ENDPOINT_EVENT_RETIRED,
    CAMD_ENDPOINT_EVENT_LOST
};

struct CAMDEndpointWatchEventV1 {
    ULONG Size;
    ULONG Version;
    struct CAMDGenerationV1 Generation;
    struct CAMDEndpointIDV1 EndpointID;
    ULONG Type;
};

/* What the endpoint functions return. */
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

/* A complete non-SysEx MIDI 1.0 message. Bytes are in wire order and Length
 * is 1..3. Running status never crosses an event boundary. */
struct CAMDMIDI1EventV1 {
    ULONG Size;
    ULONG Version;
    ULONG Length;
    UBYTE Bytes[4];
    ULONG Flags;
    ULONG TimeHigh;
    ULONG TimeLow;
    ULONG ClockDomain;
};

/* CAMDSessionRequestV1.Signal when the opener wants no signal. */
#define CAMD_SIGNAL_NONE                 0xffffffffu

/* QueueCapacity is the least number of native records to reserve. Signal is
 * for an input session: the number of the signal the opening task gets when
 * something arrives, or CAMD_SIGNAL_NONE. */
struct CAMDSessionRequestV1 {
    ULONG Size;
    ULONG Version;
    struct CAMDEndpointIDV1 EndpointID;
    ULONG Direction;
    ULONG DataFormat;
    ULONG Protocol;
    ULONG Conversion;
    ULONG QueueCapacity;
    ULONG Signal;
};

/* Records since a session opened; each count stops at 0xffffffff. Sent and
 * Rejected are an output session's: queued, and refused because the queue was
 * full or the message too large. Received and Dropped are an input
 * session's: queued for reading, and lost because its queue was full. */
struct CAMDSessionStatsV1 {
    ULONG Size;
    ULONG Version;
    ULONG Sent;
    ULONG Rejected;
    ULONG Received;
    ULONG Dropped;
};

/* A program's own endpoint. Directions are as its clients see them:
 * CAMD_DIRECTION_OUTPUT when they can send to it, CAMD_DIRECTION_INPUT when
 * they can receive from it. Name identifies it: the same Name gets the same
 * stable ID again, and only one endpoint of a Name is published at a time.
 * Signal is the number of the signal the publishing task gets when a client
 * sent something, or CAMD_SIGNAL_NONE. */
struct CAMDPublishRequestV1 {
    ULONG Size;
    ULONG Version;
    ULONG Directions;
    ULONG Signal;
    char Name[CAMD_ENDPOINT_NAME_BYTES];
    char ProductInstance[CAMD_ENDPOINT_PRODUCT_BYTES];
};

/* What an open session got. MaxSysExBytes is zero for UMP. */
struct CAMDSessionInfoV1 {
    ULONG Size;
    ULONG Version;
    ULONG Direction;
    ULONG DataFormat;
    ULONG Protocol;
    ULONG QueueCapacity;
    ULONG MaxSysExBytes;
};

#endif
