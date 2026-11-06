#ifndef CAMD_NATIVE_EVENT_PUMP_H
#define CAMD_NATIVE_EVENT_PUMP_H

/*
 * Private single-consumer pump for a CAMD native-event queue.
 *
 * A downstream callback runs without the queue lock held.  The head item is
 * committed only after CAMD_PROVIDER_OK; every other result leaves it queued
 * for an explicit retry or cancellation.  This is the bridge needed between
 * reserved session queues and bounded legacy/device buffers.
 */

#include "native_event_queue.h"

enum CAMDNativePumpResult {
    CAMD_NATIVE_PUMP_OK = 0,
    CAMD_NATIVE_PUMP_INVALID,
    CAMD_NATIVE_PUMP_NOMEM,
    CAMD_NATIVE_PUMP_EMPTY,
    CAMD_NATIVE_PUMP_BLOCKED,
    CAMD_NATIVE_PUMP_DOWNSTREAM,
    CAMD_NATIVE_PUMP_BUSY,
    CAMD_NATIVE_PUMP_STATE
};

struct CAMDNativePumpConfigV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t DataFormat;
    uint32_t MaxBatchRecords;
    uint32_t MaxSysExBytes;
};

struct CAMDNativeEventPump;

enum CAMDNativePumpResult camd_native_pump_create(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativePumpConfigV1 *config,
    const struct CAMDProviderReceiveSinkV1 *downstream,
    struct CAMDNativeEventPump **pump);
void camd_native_pump_destroy(struct CAMDNativeEventPump *pump);

/* Process at most item_budget complete queue items. processed_records counts
 * native records, so a batch can contribute more than one.  A blocked or
 * failing downstream result is returned separately and the item stays at the
 * head. */
enum CAMDNativePumpResult camd_native_pump_run(
    struct CAMDNativeEventPump *pump,
    size_t item_budget,
    size_t *processed_items,
    size_t *processed_records,
    enum CAMDProviderResult *downstream_result);

#endif
