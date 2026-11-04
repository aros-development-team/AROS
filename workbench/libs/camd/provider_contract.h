#ifndef CAMD_PROVIDER_CONTRACT_H
#define CAMD_PROVIDER_CONTRACT_H

/*
 * Executable model of CAMD's private provider data-path contract.
 *
 * This header is internal evidence for the version 43 review.  It is not an
 * installed header or a public ABI.  Sessions select one exact native path;
 * this layer never inserts protocol or container conversion.
 */

#include "endpoint_registry.h"

#include <stddef.h>
#include <stdint.h>

#define CAMD_PROVIDER_PATH_MIDI1       (1u << 0)
#define CAMD_PROVIDER_PATH_UMP_MIDI1   (1u << 1)
#define CAMD_PROVIDER_PATH_UMP_MIDI2   (1u << 2)
#define CAMD_PROVIDER_PATH_ALL         (CAMD_PROVIDER_PATH_MIDI1 | \
                                        CAMD_PROVIDER_PATH_UMP_MIDI1 | \
                                        CAMD_PROVIDER_PATH_UMP_MIDI2)

#define CAMD_PROVIDER_DIRECTION_INPUT  CAMD_DIRECTION_INPUT
#define CAMD_PROVIDER_DIRECTION_OUTPUT CAMD_DIRECTION_OUTPUT
#define CAMD_PROVIDER_DIRECTION_ALL    (CAMD_PROVIDER_DIRECTION_INPUT | \
                                        CAMD_PROVIDER_DIRECTION_OUTPUT)

enum CAMDProviderDataFormatV1 {
    CAMD_PROVIDER_FORMAT_MIDI1 = CAMD_DATA_FORMAT_MIDI1,
    CAMD_PROVIDER_FORMAT_UMP = CAMD_DATA_FORMAT_UMP
};

enum CAMDProviderProtocolV1 {
    CAMD_PROVIDER_PROTOCOL_MIDI1 = CAMD_PROTOCOL_MIDI1,
    CAMD_PROVIDER_PROTOCOL_MIDI2 = CAMD_PROTOCOL_MIDI2
};

enum CAMDProviderResult {
    CAMD_PROVIDER_OK = 0,
    CAMD_PROVIDER_INVALID,
    CAMD_PROVIDER_UNSUPPORTED,
    CAMD_PROVIDER_STATE,
    CAMD_PROVIDER_RETIRED,
    CAMD_PROVIDER_CALLBACK_FAILED,
    CAMD_PROVIDER_QUEUE_FULL,
    CAMD_PROVIDER_TOO_LARGE
};

/* A complete host-endian Universal MIDI Packet plus CAMD timing envelope. */
struct CAMDUMPEventV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t WordCount;
    uint32_t Flags;
    uint32_t Words[4];
    uint32_t TimeHigh;
    uint32_t TimeLow;
    uint32_t ClockDomain;
};

struct CAMDProviderOpenRequestV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    uint32_t Direction;
    uint32_t DataFormat;
    uint32_t Protocol;
    uint32_t QueueCapacity;
};

/* Sessions are unidirectional. QueueCapacity is a minimum reservation; a
 * successful provider reports the actual reserved native-record capacity.
 * MaxSysExBytes is zero for UMP and at least two for native MIDI 1.0. */
struct CAMDProviderOpenResultV1 {
    uint32_t Size;
    uint32_t Version;
    void *SessionContext;
    uint32_t EffectiveQueueCapacity;
    uint32_t MaxSysExBytes;
};

typedef enum CAMDProviderResult (*CAMDProviderOpenFnV1)(
    void *provider_context,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDProviderOpenResultV1 *result);
typedef enum CAMDProviderResult (*CAMDProviderCloseFnV1)(
    void *provider_context, void *session_context);
typedef enum CAMDProviderResult (*CAMDProviderSendMIDI1FnV1)(
    void *provider_context, void *session_context,
    const struct CAMDMIDI1EventV1 *events, size_t event_count);
typedef enum CAMDProviderResult (*CAMDProviderSendMIDI1SysExFnV1)(
    void *provider_context, void *session_context,
    const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags);
typedef enum CAMDProviderResult (*CAMDProviderSendUMPFnV1)(
    void *provider_context, void *session_context,
    const struct CAMDUMPEventV1 *events, size_t event_count);
typedef enum CAMDProviderResult (*CAMDProviderSessionFnV1)(
    void *provider_context, void *session_context);
typedef enum CAMDProviderResult (*CAMDProviderShutdownFnV1)(
    void *provider_context);
typedef int (*CAMDProviderShutdownReadyFnV1)(void *provider_context);

typedef enum CAMDProviderResult (*CAMDReceiveMIDI1FnV1)(
    void *camd_context, const struct CAMDMIDI1EventV1 *events,
    size_t event_count);
typedef enum CAMDProviderResult (*CAMDReceiveMIDI1SysExFnV1)(
    void *camd_context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags);
typedef enum CAMDProviderResult (*CAMDReceiveUMPFnV1)(
    void *camd_context, const struct CAMDUMPEventV1 *events,
    size_t event_count);

struct CAMDProviderReceiveSinkV1 {
    uint32_t Size;
    uint32_t Version;
    void *Context;
    CAMDReceiveMIDI1FnV1 SubmitMIDI1;
    CAMDReceiveMIDI1SysExFnV1 SubmitMIDI1SysEx;
    CAMDReceiveUMPFnV1 SubmitUMP;
};

typedef enum CAMDProviderResult (*CAMDProviderStartReceiveFnV1)(
    void *provider_context, void *session_context,
    const struct CAMDProviderReceiveSinkV1 *sink);

struct CAMDProviderOpsV1 {
    uint32_t Size;
    uint32_t Version;
    CAMDProviderOpenFnV1 Open;
    CAMDProviderCloseFnV1 Close;
    CAMDProviderSendMIDI1FnV1 SendMIDI1;
    CAMDProviderSendMIDI1SysExFnV1 SendMIDI1SysEx;
    CAMDProviderSendUMPFnV1 SendUMP;
    CAMDProviderStartReceiveFnV1 StartReceive;
    CAMDProviderSessionFnV1 StopReceive;
    CAMDProviderSessionFnV1 Drain;
    CAMDProviderSessionFnV1 Cancel;
    CAMDProviderShutdownFnV1 BeginShutdown;
    CAMDProviderShutdownReadyFnV1 ShutdownReady;
};

struct CAMDProviderDescriptorV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 ProviderID;
    uint32_t NativePaths;
    uint32_t Directions;
    uint32_t Flags;
    void *Context;
    const struct CAMDProviderOpsV1 *Ops;
};

/* Private concrete objects. The registry will eventually own and serialize
 * them; provider callbacks must be invoked without the registry lock held. */
struct CAMDPrivateProvider {
    struct CAMDProviderDescriptorV1 descriptor;
    struct CAMDProviderOpsV1 operations;
    uint32_t active_sessions;
    int initialized;
    int retiring;
};

struct CAMDPrivateProviderSession {
    struct CAMDPrivateProvider *provider;
    void *provider_context;
    struct CAMDProviderReceiveSinkV1 receive_sink;
    uint32_t direction;
    uint32_t data_format;
    uint32_t protocol;
    uint32_t requested_queue_capacity;
    uint32_t effective_queue_capacity;
    uint32_t max_sysex_bytes;
    int receive_started;
    int open;
};

uint32_t camd_provider_native_path(uint32_t data_format, uint32_t protocol);
enum CAMDProviderResult camd_provider_validate_midi1(
    const struct CAMDMIDI1EventV1 *events, size_t event_count);
enum CAMDProviderResult camd_provider_validate_midi1_sysex(
    const uint8_t *bytes, size_t byte_count);
enum CAMDProviderResult camd_provider_validate_ump(
    const struct CAMDUMPEventV1 *events, size_t event_count);
enum CAMDProviderResult camd_provider_validate_open_result(
    const struct CAMDProviderOpenRequestV1 *request,
    const struct CAMDProviderOpenResultV1 *result);

enum CAMDProviderResult camd_provider_init(
    struct CAMDPrivateProvider *provider,
    const struct CAMDProviderDescriptorV1 *descriptor);

enum CAMDProviderResult camd_provider_open(
    struct CAMDPrivateProvider *provider,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDPrivateProviderSession *session);

enum CAMDProviderResult camd_provider_send_midi1(
    struct CAMDPrivateProviderSession *session,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count);

enum CAMDProviderResult camd_provider_send_midi1_sysex(
    struct CAMDPrivateProviderSession *session,
    const uint8_t *bytes,
    size_t byte_count,
    uint32_t time_high,
    uint32_t time_low,
    uint32_t clock_domain,
    uint32_t flags);

enum CAMDProviderResult camd_provider_send_ump(
    struct CAMDPrivateProviderSession *session,
    const struct CAMDUMPEventV1 *events,
    size_t event_count);

enum CAMDProviderResult camd_provider_start_receive(
    struct CAMDPrivateProviderSession *session,
    const struct CAMDProviderReceiveSinkV1 *sink);
enum CAMDProviderResult camd_provider_stop_receive(
    struct CAMDPrivateProviderSession *session);

enum CAMDProviderResult camd_provider_drain(
    struct CAMDPrivateProviderSession *session);
enum CAMDProviderResult camd_provider_cancel(
    struct CAMDPrivateProviderSession *session);
enum CAMDProviderResult camd_provider_close(
    struct CAMDPrivateProviderSession *session);
enum CAMDProviderResult camd_provider_begin_retire(
    struct CAMDPrivateProvider *provider);
int camd_provider_ready_to_release(
    const struct CAMDPrivateProvider *provider);

#endif
