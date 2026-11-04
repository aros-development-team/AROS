#ifndef CAMD_NATIVE_EVENT_QUEUE_H
#define CAMD_NATIVE_EVENT_QUEUE_H

/*
 * Private, format-fixed, bounded queue for CAMD provider data paths.
 *
 * All storage is allocated at create time. Enqueue/dequeue never allocate,
 * batches are committed atomically, and MIDI 1.0 data is never represented as
 * UMP. The current lock is task-context only; interrupt ingress needs a
 * separately reviewed handoff before this can become a public guarantee.
 */

#include "provider_contract.h"

#include <stddef.h>
#include <stdint.h>

enum CAMDNativeQueueResult {
    CAMD_NATIVE_QUEUE_OK = 0,
    CAMD_NATIVE_QUEUE_INVALID,
    CAMD_NATIVE_QUEUE_NOMEM,
    CAMD_NATIVE_QUEUE_EMPTY,
    CAMD_NATIVE_QUEUE_FULL,
    CAMD_NATIVE_QUEUE_TOO_LARGE,
    CAMD_NATIVE_QUEUE_WRONG_FORMAT,
    CAMD_NATIVE_QUEUE_RANGE,
    CAMD_NATIVE_QUEUE_STATE,
    CAMD_NATIVE_QUEUE_BUSY
};

enum CAMDNativeQueueItemKindV1 {
    CAMD_NATIVE_QUEUE_ITEM_MIDI1 = 1,
    CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX,
    CAMD_NATIVE_QUEUE_ITEM_UMP
};

struct CAMDNativeQueueConfigV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t DataFormat;
    uint32_t Capacity;
    uint32_t MaxSysExBytes;
};

struct CAMDNativeQueueSysExInfoV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t ByteCount;
    uint32_t Flags;
    uint32_t TimeHigh;
    uint32_t TimeLow;
    uint32_t ClockDomain;
};

struct CAMDNativeQueueHeadV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t Kind;
    uint32_t RecordCount;
    uint32_t ByteCount;
};

struct CAMDNativeQueueStatsV1 {
    uint32_t Size;
    uint32_t Version;
    uint64_t AcceptedRecords;
    uint64_t DequeuedRecords;
    uint64_t CancelledRecords;
    uint64_t FullRejections;
    uint64_t OversizeRejections;
    uint32_t PendingRecords;
    uint32_t HighWaterRecords;
};

/* A checkout copies one complete head item without removing it.  Exactly one
 * checkout may be active per queue.  The consumer calls commit only after the
 * downstream accepted the item, or release to leave it queued for retry. */
struct CAMDNativeQueueCheckoutV1 {
    uint32_t Size;
    uint32_t Version;
    uint64_t Token;
    uint32_t Kind;
    uint32_t RecordCount;
};

struct CAMDNativeEventQueue;

enum CAMDNativeQueueResult camd_native_queue_create(
    const struct CAMDNativeQueueConfigV1 *config,
    struct CAMDNativeEventQueue **queue);
void camd_native_queue_destroy(struct CAMDNativeEventQueue *queue);

enum CAMDNativeQueueResult camd_native_queue_enqueue_midi1(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count);
/* Atomically accepts all records, but stores each short message as a separate
 * dispatch item.  This is used when a downstream can accept only one legacy
 * driver message at a time: retry never repeats an earlier accepted item. */
enum CAMDNativeQueueResult camd_native_queue_enqueue_midi1_items(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count);
enum CAMDNativeQueueResult camd_native_queue_enqueue_midi1_sysex(
    struct CAMDNativeEventQueue *queue,
    const uint8_t *bytes,
    size_t byte_count,
    uint32_t time_high,
    uint32_t time_low,
    uint32_t clock_domain,
    uint32_t flags);
enum CAMDNativeQueueResult camd_native_queue_enqueue_ump(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDUMPEventV1 *events,
    size_t event_count);

enum CAMDNativeQueueResult camd_native_queue_peek(
    struct CAMDNativeEventQueue *queue,
    struct CAMDNativeQueueHeadV1 *head);
enum CAMDNativeQueueResult camd_native_queue_dequeue_midi1(
    struct CAMDNativeEventQueue *queue,
    struct CAMDMIDI1EventV1 *events,
    size_t event_capacity,
    size_t *event_count);
enum CAMDNativeQueueResult camd_native_queue_dequeue_midi1_sysex(
    struct CAMDNativeEventQueue *queue,
    uint8_t *bytes,
    size_t byte_capacity,
    struct CAMDNativeQueueSysExInfoV1 *info);
enum CAMDNativeQueueResult camd_native_queue_dequeue_ump(
    struct CAMDNativeEventQueue *queue,
    struct CAMDUMPEventV1 *events,
    size_t event_capacity,
    size_t *event_count);

enum CAMDNativeQueueResult camd_native_queue_checkout_midi1(
    struct CAMDNativeEventQueue *queue,
    struct CAMDMIDI1EventV1 *events,
    size_t event_capacity,
    size_t *event_count,
    struct CAMDNativeQueueCheckoutV1 *checkout);
enum CAMDNativeQueueResult camd_native_queue_checkout_midi1_sysex(
    struct CAMDNativeEventQueue *queue,
    uint8_t *bytes,
    size_t byte_capacity,
    struct CAMDNativeQueueSysExInfoV1 *info,
    struct CAMDNativeQueueCheckoutV1 *checkout);
enum CAMDNativeQueueResult camd_native_queue_checkout_ump(
    struct CAMDNativeEventQueue *queue,
    struct CAMDUMPEventV1 *events,
    size_t event_capacity,
    size_t *event_count,
    struct CAMDNativeQueueCheckoutV1 *checkout);
enum CAMDNativeQueueResult camd_native_queue_commit(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativeQueueCheckoutV1 *checkout);
enum CAMDNativeQueueResult camd_native_queue_release(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativeQueueCheckoutV1 *checkout);

enum CAMDNativeQueueResult camd_native_queue_cancel(
    struct CAMDNativeEventQueue *queue,
    size_t *cancelled_records);
enum CAMDNativeQueueResult camd_native_queue_stats(
    struct CAMDNativeEventQueue *queue,
    struct CAMDNativeQueueStatsV1 *stats);

#endif
