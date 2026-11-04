#include "native_event_queue.h"

#include <limits.h>
#include <string.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#else
#include <pthread.h>
#include <stdlib.h>
#endif

struct queue_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct queue_slot {
    uint32_t kind;
    uint32_t batch_count;
    struct CAMDMIDI1EventV1 midi1;
    struct CAMDUMPEventV1 ump;
    struct CAMDNativeQueueSysExInfoV1 sysex;
};

struct CAMDNativeEventQueue {
    struct queue_lock lock;
    struct queue_slot *slots;
    uint8_t *sysex_arena;
    uint32_t format;
    uint32_t capacity;
    uint32_t max_sysex_bytes;
    uint32_t head;
    uint32_t tail;
    uint32_t used;
    uint32_t high_water;
    uint64_t accepted;
    uint64_t dequeued;
    uint64_t cancelled;
    uint64_t full_rejections;
    uint64_t oversize_rejections;
    uint64_t next_checkout;
    uint64_t active_checkout;
    uint32_t checked_out_records;
};

static void *queue_alloc(size_t size, int clear)
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

static void queue_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int queue_lock_init(struct queue_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore);
    return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}

static void queue_lock_destroy(struct queue_lock *lock)
{
#ifndef __AROS__
    pthread_mutex_destroy(&lock->mutex);
#else
    (void)lock;
#endif
}

static void queue_lock_acquire(struct queue_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}

static void queue_lock_release(struct queue_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
#endif
}

static uint32_t queue_index(const struct CAMDNativeEventQueue *queue,
                            uint32_t base, size_t offset)
{
    uint32_t step = (uint32_t)(offset % queue->capacity);
    uint32_t remaining = queue->capacity - base;

    return step >= remaining ? step - remaining : base + step;
}

static uint8_t *sysex_slot(struct CAMDNativeEventQueue *queue,
                           uint32_t index)
{
    return queue->sysex_arena + (size_t)index * queue->max_sysex_bytes;
}

static void counter_add(uint64_t *counter, uint64_t amount)
{
    if (UINT64_MAX - *counter < amount)
        *counter = UINT64_MAX;
    else
        *counter += amount;
}

enum CAMDNativeQueueResult camd_native_queue_create(
    const struct CAMDNativeQueueConfigV1 *config,
    struct CAMDNativeEventQueue **queue_out)
{
    struct CAMDNativeEventQueue *queue;
    size_t slot_bytes, sysex_bytes = 0;

    if (!queue_out)
        return CAMD_NATIVE_QUEUE_INVALID;
    *queue_out = NULL;
    if (!config || config->Size != sizeof(*config) || config->Version != 1 ||
        (config->DataFormat != CAMD_PROVIDER_FORMAT_MIDI1 &&
         config->DataFormat != CAMD_PROVIDER_FORMAT_UMP) ||
        config->Capacity == 0 ||
        (config->DataFormat == CAMD_PROVIDER_FORMAT_MIDI1 &&
         config->MaxSysExBytes < 2) ||
        (config->DataFormat == CAMD_PROVIDER_FORMAT_UMP &&
         config->MaxSysExBytes != 0))
        return CAMD_NATIVE_QUEUE_INVALID;
#if SIZE_MAX <= UINT32_MAX
    if (config->Capacity > SIZE_MAX / sizeof(struct queue_slot))
        return CAMD_NATIVE_QUEUE_RANGE;
#endif
    slot_bytes = (size_t)config->Capacity * sizeof(struct queue_slot);
    if (config->DataFormat == CAMD_PROVIDER_FORMAT_MIDI1) {
        if (config->MaxSysExBytes > SIZE_MAX / config->Capacity)
            return CAMD_NATIVE_QUEUE_RANGE;
        sysex_bytes = (size_t)config->Capacity * config->MaxSysExBytes;
    }
    queue = queue_alloc(sizeof(*queue), 1);
    if (!queue)
        return CAMD_NATIVE_QUEUE_NOMEM;
    if (!queue_lock_init(&queue->lock)) {
        queue_free(queue);
        return CAMD_NATIVE_QUEUE_NOMEM;
    }
    queue->slots = queue_alloc(slot_bytes, 1);
    if (!queue->slots)
        goto nomem;
    if (sysex_bytes != 0) {
        queue->sysex_arena = queue_alloc(sysex_bytes, 0);
        if (!queue->sysex_arena)
            goto nomem;
    }
    queue->format = config->DataFormat;
    queue->capacity = config->Capacity;
    queue->max_sysex_bytes = config->MaxSysExBytes;
    queue->next_checkout = 1;
    *queue_out = queue;
    return CAMD_NATIVE_QUEUE_OK;

nomem:
    queue_free(queue->sysex_arena);
    queue_free(queue->slots);
    queue_lock_destroy(&queue->lock);
    queue_free(queue);
    return CAMD_NATIVE_QUEUE_NOMEM;
}

void camd_native_queue_destroy(struct CAMDNativeEventQueue *queue)
{
    if (!queue)
        return;
    queue_free(queue->sysex_arena);
    queue_free(queue->slots);
    queue_lock_destroy(&queue->lock);
    queue_free(queue);
}

static enum CAMDNativeQueueResult reserve_records(
    struct CAMDNativeEventQueue *queue, size_t record_count)
{
    if (record_count > queue->capacity - queue->used) {
        counter_add(&queue->full_rejections, 1);
        return CAMD_NATIVE_QUEUE_FULL;
    }
    return CAMD_NATIVE_QUEUE_OK;
}

static void commit_records(struct CAMDNativeEventQueue *queue,
                           size_t record_count)
{
    queue->tail = queue_index(queue, queue->tail, record_count);
    queue->used += (uint32_t)record_count;
    counter_add(&queue->accepted, record_count);
    if (queue->used > queue->high_water)
        queue->high_water = queue->used;
}

static enum CAMDNativeQueueResult enqueue_midi1(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count,
    int split_items)
{
    enum CAMDNativeQueueResult result;
    size_t i;

    if (!queue)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    if (camd_provider_validate_midi1(events, event_count) != CAMD_PROVIDER_OK)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (event_count > UINT32_MAX)
        return CAMD_NATIVE_QUEUE_RANGE;
    queue_lock_acquire(&queue->lock);
    result = reserve_records(queue, event_count);
    if (result == CAMD_NATIVE_QUEUE_OK) {
        for (i = 0; i < event_count; ++i) {
            struct queue_slot *slot =
                &queue->slots[queue_index(queue, queue->tail, i)];

            slot->kind = CAMD_NATIVE_QUEUE_ITEM_MIDI1;
            slot->batch_count = split_items ? 1 :
                (i == 0 ? (uint32_t)event_count : 0);
            slot->midi1 = events[i];
        }
        commit_records(queue, event_count);
    }
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_enqueue_midi1(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count)
{
    return enqueue_midi1(queue, events, event_count, 0);
}

enum CAMDNativeQueueResult camd_native_queue_enqueue_midi1_items(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count)
{
    return enqueue_midi1(queue, events, event_count, 1);
}

enum CAMDNativeQueueResult camd_native_queue_enqueue_midi1_sysex(
    struct CAMDNativeEventQueue *queue,
    const uint8_t *bytes,
    size_t byte_count,
    uint32_t time_high,
    uint32_t time_low,
    uint32_t clock_domain,
    uint32_t flags)
{
    struct queue_slot *slot;
    enum CAMDNativeQueueResult result;

    if (!queue)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    if (camd_provider_validate_midi1_sysex(bytes, byte_count) !=
        CAMD_PROVIDER_OK)
        return CAMD_NATIVE_QUEUE_INVALID;
    queue_lock_acquire(&queue->lock);
    if (byte_count > queue->max_sysex_bytes || byte_count > UINT32_MAX) {
        counter_add(&queue->oversize_rejections, 1);
        result = CAMD_NATIVE_QUEUE_TOO_LARGE;
        goto out;
    }
    result = reserve_records(queue, 1);
    if (result != CAMD_NATIVE_QUEUE_OK)
        goto out;
    slot = &queue->slots[queue->tail];
    slot->kind = CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX;
    slot->batch_count = 1;
    slot->sysex.Size = sizeof(slot->sysex);
    slot->sysex.Version = 1;
    slot->sysex.ByteCount = (uint32_t)byte_count;
    slot->sysex.Flags = flags;
    slot->sysex.TimeHigh = time_high;
    slot->sysex.TimeLow = time_low;
    slot->sysex.ClockDomain = clock_domain;
    memcpy(sysex_slot(queue, queue->tail), bytes, byte_count);
    commit_records(queue, 1);
out:
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_enqueue_ump(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDUMPEventV1 *events,
    size_t event_count)
{
    enum CAMDNativeQueueResult result;
    size_t i;

    if (!queue)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_UMP)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    if (camd_provider_validate_ump(events, event_count) != CAMD_PROVIDER_OK)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (event_count > UINT32_MAX)
        return CAMD_NATIVE_QUEUE_RANGE;
    queue_lock_acquire(&queue->lock);
    result = reserve_records(queue, event_count);
    if (result == CAMD_NATIVE_QUEUE_OK) {
        for (i = 0; i < event_count; ++i) {
            struct queue_slot *slot =
                &queue->slots[queue_index(queue, queue->tail, i)];

            slot->kind = CAMD_NATIVE_QUEUE_ITEM_UMP;
            slot->batch_count = i == 0 ? (uint32_t)event_count : 0;
            slot->ump = events[i];
        }
        commit_records(queue, event_count);
    }
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_peek(
    struct CAMDNativeEventQueue *queue,
    struct CAMDNativeQueueHeadV1 *head)
{
    struct queue_slot *slot;
    enum CAMDNativeQueueResult result = CAMD_NATIVE_QUEUE_OK;

    if (!queue || !head || head->Size != sizeof(*head) || head->Version != 1)
        return CAMD_NATIVE_QUEUE_INVALID;
    queue_lock_acquire(&queue->lock);
    if (queue->used == 0) {
        result = CAMD_NATIVE_QUEUE_EMPTY;
        goto out;
    }
    slot = &queue->slots[queue->head];
    head->Kind = slot->kind;
    head->RecordCount = slot->batch_count;
    head->ByteCount = slot->kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX
                          ? slot->sysex.ByteCount
                          : 0;
out:
    queue_lock_release(&queue->lock);
    return result;
}

static void consume_records(struct CAMDNativeEventQueue *queue,
                            uint32_t record_count)
{
    uint32_t i;

    for (i = 0; i < record_count; ++i)
        memset(&queue->slots[queue_index(queue, queue->head, i)], 0,
               sizeof(queue->slots[0]));
    queue->head = queue_index(queue, queue->head, record_count);
    queue->used -= record_count;
    counter_add(&queue->dequeued, record_count);
}

static enum CAMDNativeQueueResult dequeue_events(
    struct CAMDNativeEventQueue *queue, uint32_t kind, void *events,
    size_t event_capacity, size_t event_size, size_t *event_count)
{
    struct queue_slot *first;
    uint32_t count;
    size_t i;

    if (!event_count || (!events && event_capacity != 0))
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->active_checkout != 0)
        return CAMD_NATIVE_QUEUE_BUSY;
    if (queue->used == 0)
        return CAMD_NATIVE_QUEUE_EMPTY;
    first = &queue->slots[queue->head];
    if (first->kind != kind)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    count = first->batch_count;
    if (count == 0 || count > queue->used)
        return CAMD_NATIVE_QUEUE_INVALID;
    *event_count = count;
    if (event_capacity < count)
        return CAMD_NATIVE_QUEUE_RANGE;
    for (i = 0; i < count; ++i) {
        const struct queue_slot *slot =
            &queue->slots[queue_index(queue, queue->head, i)];
        const void *source = kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1
                                 ? (const void *)&slot->midi1
                                 : (const void *)&slot->ump;

        memcpy((uint8_t *)events + i * event_size, source, event_size);
    }
    consume_records(queue, count);
    return CAMD_NATIVE_QUEUE_OK;
}

enum CAMDNativeQueueResult camd_native_queue_dequeue_midi1(
    struct CAMDNativeEventQueue *queue,
    struct CAMDMIDI1EventV1 *events,
    size_t event_capacity,
    size_t *event_count)
{
    enum CAMDNativeQueueResult result;

    if (!queue)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    queue_lock_acquire(&queue->lock);
    result = dequeue_events(queue, CAMD_NATIVE_QUEUE_ITEM_MIDI1, events,
                            event_capacity, sizeof(*events), event_count);
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_dequeue_midi1_sysex(
    struct CAMDNativeEventQueue *queue,
    uint8_t *bytes,
    size_t byte_capacity,
    struct CAMDNativeQueueSysExInfoV1 *info)
{
    struct queue_slot *slot;
    enum CAMDNativeQueueResult result = CAMD_NATIVE_QUEUE_OK;

    if (!queue || !info || info->Size != sizeof(*info) ||
        info->Version != 1 || (!bytes && byte_capacity != 0))
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    queue_lock_acquire(&queue->lock);
    if (queue->active_checkout != 0) {
        result = CAMD_NATIVE_QUEUE_BUSY;
        goto out;
    }
    if (queue->used == 0) {
        result = CAMD_NATIVE_QUEUE_EMPTY;
        goto out;
    }
    slot = &queue->slots[queue->head];
    if (slot->kind != CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX) {
        result = CAMD_NATIVE_QUEUE_WRONG_FORMAT;
        goto out;
    }
    *info = slot->sysex;
    if (byte_capacity < slot->sysex.ByteCount) {
        result = CAMD_NATIVE_QUEUE_RANGE;
        goto out;
    }
    memcpy(bytes, sysex_slot(queue, queue->head), slot->sysex.ByteCount);
    memset(sysex_slot(queue, queue->head), 0, slot->sysex.ByteCount);
    consume_records(queue, 1);
out:
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_dequeue_ump(
    struct CAMDNativeEventQueue *queue,
    struct CAMDUMPEventV1 *events,
    size_t event_capacity,
    size_t *event_count)
{
    enum CAMDNativeQueueResult result;

    if (!queue)
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_UMP)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    queue_lock_acquire(&queue->lock);
    result = dequeue_events(queue, CAMD_NATIVE_QUEUE_ITEM_UMP, events,
                            event_capacity, sizeof(*events), event_count);
    queue_lock_release(&queue->lock);
    return result;
}

static enum CAMDNativeQueueResult begin_checkout(
    struct CAMDNativeEventQueue *queue, uint32_t kind,
    struct CAMDNativeQueueCheckoutV1 *checkout)
{
    struct queue_slot *first;
    uint64_t token;

    if (queue->active_checkout != 0)
        return CAMD_NATIVE_QUEUE_BUSY;
    if (queue->used == 0)
        return CAMD_NATIVE_QUEUE_EMPTY;
    first = &queue->slots[queue->head];
    if (first->kind != kind)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    if (first->batch_count == 0 || first->batch_count > queue->used)
        return CAMD_NATIVE_QUEUE_INVALID;
    token = queue->next_checkout++;
    if (queue->next_checkout == 0)
        queue->next_checkout = 1;
    queue->active_checkout = token;
    queue->checked_out_records = first->batch_count;
    checkout->Token = token;
    checkout->Kind = kind;
    checkout->RecordCount = first->batch_count;
    return CAMD_NATIVE_QUEUE_OK;
}

static int valid_checkout(const struct CAMDNativeQueueCheckoutV1 *checkout)
{
    return checkout && checkout->Size == sizeof(*checkout) &&
           checkout->Version == 1;
}

enum CAMDNativeQueueResult camd_native_queue_checkout_midi1(
    struct CAMDNativeEventQueue *queue,
    struct CAMDMIDI1EventV1 *events,
    size_t event_capacity,
    size_t *event_count,
    struct CAMDNativeQueueCheckoutV1 *checkout)
{
    enum CAMDNativeQueueResult result;
    uint32_t count;
    size_t i;

    if (!queue || !valid_checkout(checkout) || !event_count ||
        (!events && event_capacity != 0))
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    queue_lock_acquire(&queue->lock);
    result = begin_checkout(queue, CAMD_NATIVE_QUEUE_ITEM_MIDI1, checkout);
    if (result != CAMD_NATIVE_QUEUE_OK)
        goto out;
    count = checkout->RecordCount;
    *event_count = count;
    if (event_capacity < count) {
        queue->active_checkout = 0;
        queue->checked_out_records = 0;
        result = CAMD_NATIVE_QUEUE_RANGE;
        goto out;
    }
    for (i = 0; i < count; ++i)
        events[i] = queue->slots[queue_index(queue, queue->head, i)].midi1;
out:
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_checkout_midi1_sysex(
    struct CAMDNativeEventQueue *queue,
    uint8_t *bytes,
    size_t byte_capacity,
    struct CAMDNativeQueueSysExInfoV1 *info,
    struct CAMDNativeQueueCheckoutV1 *checkout)
{
    enum CAMDNativeQueueResult result;
    struct queue_slot *slot;

    if (!queue || !valid_checkout(checkout) || !info ||
        info->Size != sizeof(*info) || info->Version != 1 ||
        (!bytes && byte_capacity != 0))
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_MIDI1)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    queue_lock_acquire(&queue->lock);
    result = begin_checkout(queue, CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX,
                            checkout);
    if (result != CAMD_NATIVE_QUEUE_OK)
        goto out;
    slot = &queue->slots[queue->head];
    *info = slot->sysex;
    if (byte_capacity < slot->sysex.ByteCount) {
        queue->active_checkout = 0;
        queue->checked_out_records = 0;
        result = CAMD_NATIVE_QUEUE_RANGE;
        goto out;
    }
    memcpy(bytes, sysex_slot(queue, queue->head), slot->sysex.ByteCount);
out:
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_checkout_ump(
    struct CAMDNativeEventQueue *queue,
    struct CAMDUMPEventV1 *events,
    size_t event_capacity,
    size_t *event_count,
    struct CAMDNativeQueueCheckoutV1 *checkout)
{
    enum CAMDNativeQueueResult result;
    uint32_t count;
    size_t i;

    if (!queue || !valid_checkout(checkout) || !event_count ||
        (!events && event_capacity != 0))
        return CAMD_NATIVE_QUEUE_INVALID;
    if (queue->format != CAMD_PROVIDER_FORMAT_UMP)
        return CAMD_NATIVE_QUEUE_WRONG_FORMAT;
    queue_lock_acquire(&queue->lock);
    result = begin_checkout(queue, CAMD_NATIVE_QUEUE_ITEM_UMP, checkout);
    if (result != CAMD_NATIVE_QUEUE_OK)
        goto out;
    count = checkout->RecordCount;
    *event_count = count;
    if (event_capacity < count) {
        queue->active_checkout = 0;
        queue->checked_out_records = 0;
        result = CAMD_NATIVE_QUEUE_RANGE;
        goto out;
    }
    for (i = 0; i < count; ++i)
        events[i] = queue->slots[queue_index(queue, queue->head, i)].ump;
out:
    queue_lock_release(&queue->lock);
    return result;
}

static enum CAMDNativeQueueResult finish_checkout(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativeQueueCheckoutV1 *checkout, int commit)
{
    enum CAMDNativeQueueResult result = CAMD_NATIVE_QUEUE_OK;

    if (!queue || !valid_checkout(checkout))
        return CAMD_NATIVE_QUEUE_INVALID;
    queue_lock_acquire(&queue->lock);
    if (queue->active_checkout == 0 ||
        queue->active_checkout != checkout->Token ||
        queue->checked_out_records != checkout->RecordCount ||
        queue->slots[queue->head].kind != checkout->Kind) {
        result = CAMD_NATIVE_QUEUE_STATE;
        goto out;
    }
    if (commit) {
        if (checkout->Kind == CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX)
            memset(sysex_slot(queue, queue->head), 0,
                   queue->slots[queue->head].sysex.ByteCount);
        consume_records(queue, queue->checked_out_records);
    }
    queue->active_checkout = 0;
    queue->checked_out_records = 0;
out:
    queue_lock_release(&queue->lock);
    return result;
}

enum CAMDNativeQueueResult camd_native_queue_commit(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativeQueueCheckoutV1 *checkout)
{
    return finish_checkout(queue, checkout, 1);
}

enum CAMDNativeQueueResult camd_native_queue_release(
    struct CAMDNativeEventQueue *queue,
    const struct CAMDNativeQueueCheckoutV1 *checkout)
{
    return finish_checkout(queue, checkout, 0);
}

enum CAMDNativeQueueResult camd_native_queue_cancel(
    struct CAMDNativeEventQueue *queue,
    size_t *cancelled_records)
{
    uint32_t count, i;

    if (!queue || !cancelled_records)
        return CAMD_NATIVE_QUEUE_INVALID;
    queue_lock_acquire(&queue->lock);
    if (queue->active_checkout != 0) {
        queue_lock_release(&queue->lock);
        return CAMD_NATIVE_QUEUE_BUSY;
    }
    count = queue->used;
    for (i = 0; i < count; ++i) {
        uint32_t index = queue_index(queue, queue->head, i);

        if (queue->slots[index].kind ==
            CAMD_NATIVE_QUEUE_ITEM_MIDI1_SYSEX)
            memset(sysex_slot(queue, index), 0,
                   queue->slots[index].sysex.ByteCount);
        memset(&queue->slots[index], 0, sizeof(queue->slots[index]));
    }
    queue->head = queue->tail;
    queue->used = 0;
    counter_add(&queue->cancelled, count);
    *cancelled_records = count;
    queue_lock_release(&queue->lock);
    return CAMD_NATIVE_QUEUE_OK;
}

enum CAMDNativeQueueResult camd_native_queue_stats(
    struct CAMDNativeEventQueue *queue,
    struct CAMDNativeQueueStatsV1 *stats)
{
    if (!queue || !stats || stats->Size != sizeof(*stats) ||
        stats->Version != 1)
        return CAMD_NATIVE_QUEUE_INVALID;
    queue_lock_acquire(&queue->lock);
    stats->AcceptedRecords = queue->accepted;
    stats->DequeuedRecords = queue->dequeued;
    stats->CancelledRecords = queue->cancelled;
    stats->FullRejections = queue->full_rejections;
    stats->OversizeRejections = queue->oversize_rejections;
    stats->PendingRecords = queue->used;
    stats->HighWaterRecords = queue->high_water;
    queue_lock_release(&queue->lock);
    return CAMD_NATIVE_QUEUE_OK;
}
