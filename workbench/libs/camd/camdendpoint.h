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
#define CAMD_TOPOLOGY_NAME_BYTES        64u

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

/* CAMDMIDI1EventV1.Flags: the time fields hold a time. */
#define CAMD_EVENT_TIME_VALID            (1u << 0)

/* ClockDomain: TimeLow is CamdTime(), milliseconds that wrap after 2^32, and
 * TimeHigh is 0. The only clock so far. */
#define CAMD_CLOCK_CAMD                  1u

/* A complete non-SysEx MIDI 1.0 message. Bytes are in wire order and Length
 * is 1..3. Running status never crosses an event boundary.
 *
 * Sent to a driver's port with CAMD_EVENT_TIME_VALID, the message is handed
 * to the port when CamdTime() reaches TimeLow, or at once when that time has
 * passed; without the flag it is handed over at once. A session's messages
 * keep their order, so one that waits holds back those queued after it.
 * A published endpoint gets the time as the client gave it.
 *
 * A received message has the flag set and the time it reached the session,
 * unless its sender already gave it a time. */
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

/* One complete Universal MIDI Packet message of WordCount words, each in
 * the machine's byte order. WordCount has to be what the message type in the
 * top four bits of Words[0] calls for. Flags and the time fields are as in
 * CAMDMIDI1EventV1. CAMD carries the words and does not interpret them. */
struct CAMDUMPEventV1 {
    ULONG Size;
    ULONG Version;
    ULONG WordCount;
    ULONG Flags;
    ULONG Words[4];
    ULONG TimeHigh;
    ULONG TimeLow;
    ULONG ClockDomain;
};

/* The Groups a UMP endpoint uses, and its Function Blocks: FirstGroup and
 * GroupCount name the Groups a block spans. A publisher sets them with
 * SetPublishedEndpointTopology(); CAMD checks their ranges and nothing else. */
struct CAMDGroupInfoV1 {
    ULONG Size;
    ULONG Version;
    struct CAMDEndpointIDV1 EndpointID;
    ULONG Group;
    ULONG Flags;
    ULONG Protocol;
    char Name[CAMD_TOPOLOGY_NAME_BYTES];
};

struct CAMDFunctionBlockInfoV1 {
    ULONG Size;
    ULONG Version;
    struct CAMDEndpointIDV1 EndpointID;
    ULONG Number;
    ULONG Flags;
    ULONG FirstGroup;
    ULONG GroupCount;
    char Name[CAMD_TOPOLOGY_NAME_BYTES];
};

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
 * they can receive from it. DataFormat and Protocol are what every session
 * with it carries; only a MIDI 1.0 endpoint gets legacy clusters. Name
 * identifies it: the same Name gets the same
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
    ULONG DataFormat;   /* CAMD_DATA_FORMAT_MIDI1 or CAMD_DATA_FORMAT_UMP */
    ULONG Protocol;     /* CAMD_PROTOCOL_MIDI1, or _MIDI2 with UMP */
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

/* The records hold 32-bit fields and bytes only, so they have these sizes on
 * every target. A compiler that disagrees stops here. */
#define CAMD_RECORD_SIZE(record, bytes) \
    typedef char camd_size_of_##record[sizeof(struct record) == (bytes) ? 1 : -1]
CAMD_RECORD_SIZE(CAMDHandleV1, 8);
CAMD_RECORD_SIZE(CAMDEndpointIDV1, 16);
CAMD_RECORD_SIZE(CAMDGenerationV1, 8);
CAMD_RECORD_SIZE(CAMDEndpointInfoV1, 360);
CAMD_RECORD_SIZE(CAMDEndpointWatchEventV1, 36);
CAMD_RECORD_SIZE(CAMDMIDI1EventV1, 32);
CAMD_RECORD_SIZE(CAMDUMPEventV1, 44);
CAMD_RECORD_SIZE(CAMDGroupInfoV1, 100);
CAMD_RECORD_SIZE(CAMDFunctionBlockInfoV1, 104);
CAMD_RECORD_SIZE(CAMDSessionRequestV1, 48);
CAMD_RECORD_SIZE(CAMDSessionInfoV1, 28);
CAMD_RECORD_SIZE(CAMDSessionStatsV1, 24);
CAMD_RECORD_SIZE(CAMDPublishRequestV1, 280);
#undef CAMD_RECORD_SIZE

#endif
