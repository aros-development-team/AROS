#include "native_event_pump.h"

#include <limits.h>
#include <string.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <proto/exec.h>
#else
#include <stdlib.h>
#endif

struct CAMDNativeEventPump {
    struct CAMDNativeEventQueue *queue;
    struct CAMDProviderReceiveSinkV1 downstream;
    struct CAMDMIDI1EventV1 *midi1;
    struct CAMDUMPEventV1 *ump;
    uint8_t *sysex;
    uint32_t format;
    uint32_t max_batch_records;
    uint32_t max_sysex_bytes;
};

static void *pump_alloc(size_t size, int clear)
{
#ifdef __AROS__
    ULONG flags = MEMF_ANY | (clear ? MEMF_CLEAR : 0);

    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, flags);
#else
    return clear ? calloc(1, size) : malloc(size);
#endif
}

static void pump_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int valid_config(const struct CAMDNativePumpConfigV1 *config,
                        const struct CAMDProviderReceiveSinkV1 *downstream)
{
    if (!config || config->Size != sizeof(*config) || config->Version != 1 ||
        config->MaxBatchRecords == 0 || !downstream ||
        downstream->Size != sizeof(*downstream) || downstream->Version != 1)
        return 0;
    if (config->DataFormat == CAMD_PROVIDER_FORMAT_MIDI1)
        return config->MaxSysExBytes >= 2 && downstream->SubmitMIDI1 &&
               downstream->SubmitMIDI1SysEx && !downstream->SubmitUMP;
    if (config->DataFormat == CAMD_PROVIDER_FORMAT_UMP)
        return config->MaxSysExBytes == 0 && !downstream->SubmitMIDI1 &&
               !downstream->SubmitMIDI1SysEx && downstream->SubmitUMP;
    return 0;
}

enum CAMDNativePumpResult camd_native_pump_create(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativePumpConfigV1 *config,
    const struct CAMDProviderReceiveSinkV1 *downstream,
    struct CAMDNativeEventPump **pump_out)
{
    struct CAMDNativeEventPump *pump;
    size_t bytes;

    if (!pump_out)
        return CAMD_NATIVE_PUMP_INVALID;
    *pump_out = NULL;
    if (!queue || !valid_config(config, downstream))
        return CAMD_NATIVE_PUMP_INVALID;
    pump = pump_alloc(sizeof(*pump), 1);
    if (!pump)
        return CAMD_NATIVE_PUMP_NOMEM;
    pump->queue = queue;
    pump->downstream = *downstream;
    pump->format = config->DataFormat;
    pump->max_batch_records = config->MaxBatchRecords;
    pump->max_sysex_bytes = config->MaxSysExBytes;
#if SIZE_MAX <= UINT32_MAX
    if (config->MaxBatchRecords > SIZE_MAX / sizeof(*pump->midi1))
        goto nomem;
#endif
    bytes = (size_t)config->MaxBatchRecords * sizeof(*pump->midi1);
    if (pump->format == CAMD_PROVIDER_FORMAT_MIDI1) {
        pump->midi1 = pump_alloc(bytes, 0);
        pump->sysex = pump_alloc(config->MaxSysExBytes, 0);
        if (!pump->midi1 || !pump->sysex)
            goto nomem;
    } else {
#if SIZE_MAX <= UINT32_MAX
        if (config->MaxBatchRecords > SIZE_MAX / sizeof(*pump->ump))
            goto nomem;
#endif
        bytes = (size_t)config->MaxBatchRecords * sizeof(*pump->ump);
        pump->ump = pump_alloc(bytes, 0);
        if (!pump->ump)
            goto nomem;
    }
    *pump_out = pump;
    return CAMD_NATIVE_PUMP_OK;

nomem:
    pump_free(pump->sysex);
    pump_free(pump->ump);
    pump_free(pump->midi1);
    pump_free(pump);
    return CAMD_NATIVE_PUMP_NOMEM;
}

void camd_native_pump_destroy(struct CAMDNativeEventPump *pump)
{
    if (!pump)
        return;
    pump_free(pump->sysex);
    pump_free(pump->ump);
    pump_free(pump->midi1);
    pump_free(pump);
}

static enum CAMDNativePumpResult checkout_head(
    struct CAMDNativeEventPump *pump,
    const struct CAMDNativeQueueHeadV1 *head,
    struct CAMDNativeQueueCheckoutV1 *checkout,
    enum CAMDProviderResult *downstream_result)
{
    enum CAMDNativeQueueResult queue_result;
    struct CAMDNativeQueueSysExInfoV1 sysex_info;
    size_t count = 0;

    if (head->RecordCount > pump->max_batch_records)
        return CAMD_NATIVE_PUMP_STATE;
    if ((pump->format == CAMD_PROVIDER_FORMAT_MIDI1 &&
         head->Kind == CAMD_NATIVE_QUEUE_ITEM_UMP) ||
        (pump->format == CAMD_PROVIDER_FORMAT_UMP &&
         head->Kind != CAMD_NATIVE_QUEUE_ITEM_UMP))
        return CAMD_NATIVE_PUMP_STATE;
    if (head->Kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1) {
        queue_result = camd_native_queue_checkout_midi1(
            pump->queue, pump->midi1, pump->max_batch_records, &count,
            checkout);
        if (queue_result == CAMD_NATIVE_QUEUE_OK)
            *downstream_result = pump->downstream.SubmitMIDI1(
                pump->downstream.Context, pump->midi1, count);
    } else if (head->Kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX) {
        memset(&sysex_info, 0, sizeof(sysex_info));
        sysex_info.Size = sizeof(sysex_info);
        sysex_info.Version = 1;
        queue_result = camd_native_queue_checkout_midi1_sysex(
            pump->queue, pump->sysex, pump->max_sysex_bytes, &sysex_info,
            checkout);
        if (queue_result == CAMD_NATIVE_QUEUE_OK)
            *downstream_result = pump->downstream.SubmitMIDI1SysEx(
                pump->downstream.Context, pump->sysex,
                sysex_info.ByteCount, sysex_info.TimeHigh,
                sysex_info.TimeLow, sysex_info.ClockDomain,
                sysex_info.Flags);
    } else if (head->Kind == CAMD_NATIVE_QUEUE_ITEM_UMP) {
        queue_result = camd_native_queue_checkout_ump(
            pump->queue, pump->ump, pump->max_batch_records, &count,
            checkout);
        if (queue_result == CAMD_NATIVE_QUEUE_OK)
            *downstream_result = pump->downstream.SubmitUMP(
                pump->downstream.Context, pump->ump, count);
    } else {
        return CAMD_NATIVE_PUMP_STATE;
    }
    if (queue_result == CAMD_NATIVE_QUEUE_BUSY)
        return CAMD_NATIVE_PUMP_BUSY;
    if (queue_result != CAMD_NATIVE_QUEUE_OK)
        return CAMD_NATIVE_PUMP_STATE;
    return CAMD_NATIVE_PUMP_OK;
}

enum CAMDNativePumpResult camd_native_pump_run(
    struct CAMDNativeEventPump *pump,
    size_t item_budget,
    size_t *processed_items,
    size_t *processed_records,
    enum CAMDProviderResult *downstream_result)
{
    enum CAMDNativePumpResult result = CAMD_NATIVE_PUMP_OK;
    size_t items = 0, records = 0;

    if (!pump || item_budget == 0 || !processed_items ||
        !processed_records || !downstream_result)
        return CAMD_NATIVE_PUMP_INVALID;
    *processed_items = 0;
    *processed_records = 0;
    *downstream_result = CAMD_PROVIDER_OK;
    while (items < item_budget) {
        struct CAMDNativeQueueHeadV1 head;
        struct CAMDNativeQueueCheckoutV1 checkout;
        enum CAMDNativeQueueResult queue_result;

        memset(&head, 0, sizeof(head));
        head.Size = sizeof(head);
        head.Version = 1;
        queue_result = camd_native_queue_peek(pump->queue, &head);
        if (queue_result == CAMD_NATIVE_QUEUE_EMPTY) {
            result = items == 0 ? CAMD_NATIVE_PUMP_EMPTY
                                : CAMD_NATIVE_PUMP_OK;
            break;
        }
        if (queue_result != CAMD_NATIVE_QUEUE_OK) {
            result = CAMD_NATIVE_PUMP_STATE;
            break;
        }
        memset(&checkout, 0, sizeof(checkout));
        checkout.Size = sizeof(checkout);
        checkout.Version = 1;
        result = checkout_head(pump, &head, &checkout, downstream_result);
        if (result != CAMD_NATIVE_PUMP_OK)
            break;
        if (*downstream_result != CAMD_PROVIDER_OK) {
            if (camd_native_queue_release(pump->queue, &checkout) !=
                CAMD_NATIVE_QUEUE_OK) {
                result = CAMD_NATIVE_PUMP_STATE;
                break;
            }
            result = *downstream_result == CAMD_PROVIDER_QUEUE_FULL
                         ? CAMD_NATIVE_PUMP_BLOCKED
                         : CAMD_NATIVE_PUMP_DOWNSTREAM;
            break;
        }
        if (camd_native_queue_commit(pump->queue, &checkout) !=
            CAMD_NATIVE_QUEUE_OK) {
            result = CAMD_NATIVE_PUMP_STATE;
            break;
        }
        ++items;
        records += head.RecordCount;
    }
    *processed_items = items;
    *processed_records = records;
    return result;
}
