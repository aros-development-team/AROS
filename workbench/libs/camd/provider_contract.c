#include "provider_contract.h"

#include <string.h>

typedef char midi1_event_size_must_be_32[
    sizeof(struct CAMDMIDI1EventV1) == 32 ? 1 : -1];
typedef char ump_event_size_must_be_44[
    sizeof(struct CAMDUMPEventV1) == 44 ? 1 : -1];

static int id_is_zero(const struct CAMDEndpointIDV1 *id)
{
    return !(id->word[0] | id->word[1] | id->word[2] | id->word[3]);
}

uint32_t camd_provider_native_path(uint32_t data_format, uint32_t protocol)
{
    if (data_format == CAMD_PROVIDER_FORMAT_MIDI1 &&
        protocol == CAMD_PROVIDER_PROTOCOL_MIDI1)
        return CAMD_PROVIDER_PATH_MIDI1;
    if (data_format == CAMD_PROVIDER_FORMAT_UMP &&
        protocol == CAMD_PROVIDER_PROTOCOL_MIDI1)
        return CAMD_PROVIDER_PATH_UMP_MIDI1;
    if (data_format == CAMD_PROVIDER_FORMAT_UMP &&
        protocol == CAMD_PROVIDER_PROTOCOL_MIDI2)
        return CAMD_PROVIDER_PATH_UMP_MIDI2;
    return 0;
}

enum CAMDProviderResult camd_provider_validate_midi1(
    const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    size_t i, j;

    if (!events || event_count == 0)
        return CAMD_PROVIDER_INVALID;
    for (i = 0; i < event_count; ++i) {
        uint8_t status = events[i].Bytes[0];
        uint32_t expected_length;

        if (status < 0x80 || status == 0xf0 || status == 0xf4 ||
            status == 0xf5 || status == 0xf7 || status == 0xf9 ||
            status == 0xfd)
            return CAMD_PROVIDER_INVALID;
        if (status < 0xc0)
            expected_length = 3;
        else if (status < 0xe0)
            expected_length = 2;
        else if (status < 0xf0)
            expected_length = 3;
        else if (status == 0xf1 || status == 0xf3)
            expected_length = 2;
        else if (status == 0xf2)
            expected_length = 3;
        else
            expected_length = 1;
        if (events[i].Size != sizeof(events[i]) || events[i].Version != 1 ||
            events[i].Length != expected_length)
            return CAMD_PROVIDER_INVALID;
        for (j = 1; j < events[i].Length; ++j)
            if (events[i].Bytes[j] >= 0x80)
                return CAMD_PROVIDER_INVALID;
    }
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_validate_midi1_sysex(
    const uint8_t *bytes, size_t byte_count)
{
    if (!bytes || byte_count < 2 || bytes[0] != 0xf0 ||
        bytes[byte_count - 1] != 0xf7)
        return CAMD_PROVIDER_INVALID;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_validate_ump(
    const struct CAMDUMPEventV1 *events, size_t event_count)
{
    size_t i;

    if (!events || event_count == 0)
        return CAMD_PROVIDER_INVALID;
    for (i = 0; i < event_count; ++i) {
        /* Words per message type, UMP 1.1: 32-bit types 0-2 and 6-7, 64-bit
         * 3-4 and 8-A, 96-bit B-C, 128-bit 5 and D-F. */
        static const uint8_t words[16] = {
            1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4
        };

        if (events[i].Size != sizeof(events[i]) || events[i].Version != 1 ||
            events[i].WordCount != words[events[i].Words[0] >> 28])
            return CAMD_PROVIDER_INVALID;
    }
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_validate_open_result(
    const struct CAMDProviderOpenRequestV1 *request,
    const struct CAMDProviderOpenResultV1 *result)
{
    if (!request || !result || result->Size != sizeof(*result) ||
        result->Version != 1 ||
        result->EffectiveQueueCapacity < request->QueueCapacity)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    if ((request->DataFormat == CAMD_PROVIDER_FORMAT_MIDI1 &&
         result->MaxSysExBytes < 2) ||
        (request->DataFormat == CAMD_PROVIDER_FORMAT_UMP &&
         result->MaxSysExBytes != 0))
        return CAMD_PROVIDER_CALLBACK_FAILED;
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult session_state(
    const struct CAMDPrivateProviderSession *session)
{
    if (!session || !session->open || !session->provider ||
        !session->provider->initialized)
        return CAMD_PROVIDER_STATE;
    if (session->provider->retiring)
        return CAMD_PROVIDER_RETIRED;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_init(
    struct CAMDPrivateProvider *provider,
    const struct CAMDProviderDescriptorV1 *descriptor)
{
    const struct CAMDProviderOpsV1 *ops;

    if (!provider || !descriptor ||
        descriptor->Size != sizeof(*descriptor) || descriptor->Version != 1 ||
        id_is_zero(&descriptor->ProviderID) || !descriptor->Ops ||
        descriptor->NativePaths == 0 ||
        (descriptor->NativePaths & ~CAMD_PROVIDER_PATH_ALL) != 0 ||
        descriptor->Directions == 0 ||
        (descriptor->Directions & ~CAMD_PROVIDER_DIRECTION_ALL) != 0)
        return CAMD_PROVIDER_INVALID;
    ops = descriptor->Ops;
    if (ops->Size != sizeof(*ops) || ops->Version != 1 || !ops->Open ||
        !ops->Close || !ops->BeginShutdown || !ops->ShutdownReady)
        return CAMD_PROVIDER_INVALID;
    if ((descriptor->Directions & CAMD_PROVIDER_DIRECTION_OUTPUT) != 0 &&
        (!ops->Drain || !ops->Cancel))
        return CAMD_PROVIDER_INVALID;
    if ((descriptor->Directions & CAMD_PROVIDER_DIRECTION_OUTPUT) != 0 &&
        (descriptor->NativePaths & CAMD_PROVIDER_PATH_MIDI1) != 0 &&
        (!ops->SendMIDI1 || !ops->SendMIDI1SysEx))
        return CAMD_PROVIDER_INVALID;
    if ((descriptor->Directions & CAMD_PROVIDER_DIRECTION_OUTPUT) != 0 &&
        (descriptor->NativePaths & (CAMD_PROVIDER_PATH_UMP_MIDI1 |
                                    CAMD_PROVIDER_PATH_UMP_MIDI2)) != 0 &&
        !ops->SendUMP)
        return CAMD_PROVIDER_INVALID;
    if ((descriptor->Directions & CAMD_PROVIDER_DIRECTION_INPUT) != 0 &&
        (!ops->StartReceive || !ops->StopReceive))
        return CAMD_PROVIDER_INVALID;

    memset(provider, 0, sizeof(*provider));
    provider->descriptor = *descriptor;
    provider->operations = *ops;
    provider->descriptor.Ops = &provider->operations;
    provider->initialized = 1;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_open(
    struct CAMDPrivateProvider *provider,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDPrivateProviderSession *session)
{
    enum CAMDProviderResult result;
    struct CAMDProviderOpenResultV1 open_result;
    uint32_t path;

    if (!provider || !provider->initialized || !request || !session ||
        request->Size != sizeof(*request) || request->Version != 1 ||
        id_is_zero(&request->EndpointID) || request->QueueCapacity == 0 ||
        (request->Direction != CAMD_PROVIDER_DIRECTION_INPUT &&
         request->Direction != CAMD_PROVIDER_DIRECTION_OUTPUT) ||
        (request->Direction & ~provider->descriptor.Directions) != 0)
        return CAMD_PROVIDER_INVALID;
    if (provider->retiring)
        return CAMD_PROVIDER_RETIRED;
    if (provider->active_sessions == UINT32_MAX)
        return CAMD_PROVIDER_STATE;
    path = camd_provider_native_path(request->DataFormat, request->Protocol);
    if (path == 0 || (provider->descriptor.NativePaths & path) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;

    memset(&open_result, 0, sizeof(open_result));
    open_result.Size = sizeof(open_result);
    open_result.Version = 1;
    result = provider->operations.Open(provider->descriptor.Context, request,
                                       &open_result);
    if (result != CAMD_PROVIDER_OK)
        return result;
    result = camd_provider_validate_open_result(request, &open_result);
    if (result != CAMD_PROVIDER_OK) {
        provider->operations.Close(provider->descriptor.Context,
                                   open_result.SessionContext);
        return result;
    }
    memset(session, 0, sizeof(*session));
    session->provider = provider;
    session->provider_context = open_result.SessionContext;
    session->direction = request->Direction;
    session->data_format = request->DataFormat;
    session->protocol = request->Protocol;
    session->requested_queue_capacity = request->QueueCapacity;
    session->effective_queue_capacity = open_result.EffectiveQueueCapacity;
    session->max_sysex_bytes = open_result.MaxSysExBytes;
    session->open = 1;
    ++provider->active_sessions;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_send_midi1(
    struct CAMDPrivateProviderSession *session,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count)
{
    enum CAMDProviderResult result = session_state(session);

    if (result != CAMD_PROVIDER_OK)
        return result;
    if (session->data_format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_PROVIDER_UNSUPPORTED;
    if ((session->direction & CAMD_PROVIDER_DIRECTION_OUTPUT) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    result = camd_provider_validate_midi1(events, event_count);
    if (result != CAMD_PROVIDER_OK)
        return result;
    return session->provider->operations.SendMIDI1(
        session->provider->descriptor.Context, session->provider_context,
        events, event_count);
}

enum CAMDProviderResult camd_provider_send_midi1_sysex(
    struct CAMDPrivateProviderSession *session,
    const uint8_t *bytes,
    size_t byte_count,
    uint32_t time_high,
    uint32_t time_low,
    uint32_t clock_domain,
    uint32_t flags)
{
    enum CAMDProviderResult result = session_state(session);

    if (result != CAMD_PROVIDER_OK)
        return result;
    if (session->data_format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_PROVIDER_UNSUPPORTED;
    if ((session->direction & CAMD_PROVIDER_DIRECTION_OUTPUT) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    result = camd_provider_validate_midi1_sysex(bytes, byte_count);
    if (result != CAMD_PROVIDER_OK)
        return result;
    if (byte_count > session->max_sysex_bytes)
        return CAMD_PROVIDER_TOO_LARGE;
    return session->provider->operations.SendMIDI1SysEx(
        session->provider->descriptor.Context, session->provider_context,
        bytes, byte_count, time_high, time_low, clock_domain, flags);
}

enum CAMDProviderResult camd_provider_send_ump(
    struct CAMDPrivateProviderSession *session,
    const struct CAMDUMPEventV1 *events,
    size_t event_count)
{
    enum CAMDProviderResult result = session_state(session);

    if (result != CAMD_PROVIDER_OK)
        return result;
    if (session->data_format != CAMD_PROVIDER_FORMAT_UMP)
        return CAMD_PROVIDER_UNSUPPORTED;
    if ((session->direction & CAMD_PROVIDER_DIRECTION_OUTPUT) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    result = camd_provider_validate_ump(events, event_count);
    if (result != CAMD_PROVIDER_OK)
        return result;
    return session->provider->operations.SendUMP(
        session->provider->descriptor.Context, session->provider_context,
        events, event_count);
}

enum CAMDProviderResult camd_provider_start_receive(
    struct CAMDPrivateProviderSession *session,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    enum CAMDProviderResult result = session_state(session);

    if (result != CAMD_PROVIDER_OK)
        return result;
    if ((session->direction & CAMD_PROVIDER_DIRECTION_INPUT) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    if (session->receive_started || !sink || sink->Size != sizeof(*sink) ||
        sink->Version != 1)
        return CAMD_PROVIDER_INVALID;
    if (session->data_format == CAMD_PROVIDER_FORMAT_MIDI1) {
        if (!sink->SubmitMIDI1 || !sink->SubmitMIDI1SysEx)
            return CAMD_PROVIDER_INVALID;
    } else if (session->data_format == CAMD_PROVIDER_FORMAT_UMP) {
        if (!sink->SubmitUMP)
            return CAMD_PROVIDER_INVALID;
    } else {
        return CAMD_PROVIDER_UNSUPPORTED;
    }

    session->receive_sink = *sink;
    if (session->data_format == CAMD_PROVIDER_FORMAT_MIDI1)
        session->receive_sink.SubmitUMP = NULL;
    else {
        session->receive_sink.SubmitMIDI1 = NULL;
        session->receive_sink.SubmitMIDI1SysEx = NULL;
    }
    result = session->provider->operations.StartReceive(
        session->provider->descriptor.Context, session->provider_context,
        &session->receive_sink);
    if (result != CAMD_PROVIDER_OK) {
        memset(&session->receive_sink, 0, sizeof(session->receive_sink));
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    session->receive_started = 1;
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_stop_receive(
    struct CAMDPrivateProviderSession *session)
{
    enum CAMDProviderResult result;

    if (!session || !session->open || !session->provider ||
        !session->receive_started)
        return CAMD_PROVIDER_STATE;
    result = session->provider->operations.StopReceive(
        session->provider->descriptor.Context, session->provider_context);
    if (result != CAMD_PROVIDER_OK)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    session->receive_started = 0;
    memset(&session->receive_sink, 0, sizeof(session->receive_sink));
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_drain(
    struct CAMDPrivateProviderSession *session)
{
    if (!session || !session->open || !session->provider)
        return CAMD_PROVIDER_STATE;
    if ((session->direction & CAMD_PROVIDER_DIRECTION_OUTPUT) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    return session->provider->operations.Drain(
        session->provider->descriptor.Context, session->provider_context);
}

enum CAMDProviderResult camd_provider_cancel(
    struct CAMDPrivateProviderSession *session)
{
    if (!session || !session->open || !session->provider)
        return CAMD_PROVIDER_STATE;
    if ((session->direction & CAMD_PROVIDER_DIRECTION_OUTPUT) == 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    return session->provider->operations.Cancel(
        session->provider->descriptor.Context, session->provider_context);
}

enum CAMDProviderResult camd_provider_close(
    struct CAMDPrivateProviderSession *session)
{
    struct CAMDPrivateProvider *provider;
    enum CAMDProviderResult result;

    if (!session || !session->open || !session->provider)
        return CAMD_PROVIDER_STATE;
    if (session->receive_started)
        return CAMD_PROVIDER_STATE;
    provider = session->provider;
    result = provider->operations.Close(provider->descriptor.Context,
                                        session->provider_context);
    if (result != CAMD_PROVIDER_OK)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    --provider->active_sessions;
    memset(session, 0, sizeof(*session));
    return CAMD_PROVIDER_OK;
}

enum CAMDProviderResult camd_provider_begin_retire(
    struct CAMDPrivateProvider *provider)
{
    enum CAMDProviderResult result;

    if (!provider || !provider->initialized)
        return CAMD_PROVIDER_STATE;
    if (provider->retiring)
        return CAMD_PROVIDER_RETIRED;
    /* Publish retirement before the callback so callback reentry cannot open
     * a new session after shutdown has begun. Retirement is irreversible. */
    provider->retiring = 1;
    result = provider->operations.BeginShutdown(provider->descriptor.Context);
    if (result != CAMD_PROVIDER_OK)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    return CAMD_PROVIDER_OK;
}

int camd_provider_ready_to_release(
    const struct CAMDPrivateProvider *provider)
{
    return provider && provider->initialized && provider->retiring &&
           provider->active_sessions == 0 &&
           provider->operations.ShutdownReady(provider->descriptor.Context);
}
