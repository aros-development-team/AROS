/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "rtpmidi/timing.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int rtpmidi_clock_offset(const uint64_t timestamps[3], int is_initiator,
                    int64_t *peer_to_local)
{
    uint64_t midpoint;
    uint64_t distance;
    if (!timestamps || !peer_to_local || timestamps[2] < timestamps[0])
        return -1;
    midpoint = timestamps[0] + (timestamps[2] - timestamps[0]) / 2;
    distance = midpoint >= timestamps[1] ? midpoint - timestamps[1] :
                                           timestamps[1] - midpoint;
    if (distance > INT64_MAX)
        return -1;
    if (is_initiator)
        *peer_to_local = midpoint >= timestamps[1] ?
                         (int64_t)distance : -(int64_t)distance;
    else
        *peer_to_local = midpoint >= timestamps[1] ?
                         -(int64_t)distance : (int64_t)distance;
    return 0;
}

int rtpmidi_clock_due(uint32_t timestamp, uint64_t local_now,
                 int64_t peer_to_local, uint64_t *local_due)
{
    uint64_t peer_now;
    uint32_t difference;
    int64_t delta;
    if (!local_due)
        return -1;
    if (peer_to_local >= 0) {
        if (local_now < (uint64_t)peer_to_local)
            return -1;
        peer_now = local_now - (uint64_t)peer_to_local;
    } else {
        uint64_t magnitude = (uint64_t)(-(peer_to_local + 1)) + 1;
        if (local_now > UINT64_MAX - magnitude)
            return -1;
        peer_now = local_now + magnitude;
    }
    difference = timestamp - (uint32_t)peer_now;
    delta = difference <= INT32_MAX ? (int64_t)difference :
            -(int64_t)(UINT64_C(0x100000000) - difference);
    if (delta >= 0) {
        if (local_now > UINT64_MAX - (uint64_t)delta)
            return -1;
        *local_due = local_now + (uint64_t)delta;
    } else {
        uint64_t past = (uint64_t)-delta;
        *local_due = local_now >= past ? local_now - past : 0;
    }
    return 0;
}

void rtpmidi_queue_reset(struct rtpmidi_event_queue *queue)
{
    size_t i;
    if (!queue) return;
    for (i = 0; i < queue->count; ++i)
        free(queue->events[i].sysex);
    memset(queue->events, 0, sizeof(queue->events));
    queue->count = 0;
    queue->sysex_bytes = 0;
}

static size_t rtpmidi_queue_position(struct rtpmidi_event_queue *queue, uint64_t due)
{
    size_t position = queue->count;
    while (position && queue->events[position - 1].due > due) {
        queue->events[position] = queue->events[position - 1];
        --position;
    }
    return position;
}

int rtpmidi_queue_push(struct rtpmidi_event_queue *queue, uint64_t due,
                  const uint8_t *bytes, size_t length)
{
    size_t position;
    if (!queue || !bytes || length == 0 || length > 3 ||
        queue->count == RTPMIDI_EVENT_QUEUE_CAP)
        return -1;
    position = rtpmidi_queue_position(queue, due);
    memset(&queue->events[position], 0, sizeof(queue->events[position]));
    queue->events[position].due = due;
    queue->events[position].length = (uint8_t)length;
    memcpy(queue->events[position].bytes, bytes, length);
    ++queue->count;
    return 0;
}

int rtpmidi_queue_push_sysex(struct rtpmidi_event_queue *queue, uint64_t due,
                        const uint8_t *bytes, size_t length)
{
    uint8_t *copy;
    size_t position;
    if (!queue || !bytes || length < 2 || bytes[0] != 0xf0 ||
        bytes[length - 1] != 0xf7 || queue->count == RTPMIDI_EVENT_QUEUE_CAP ||
        length > RTPMIDI_EVENT_QUEUE_SYSEX_BYTES - queue->sysex_bytes)
        return -1;
    copy = malloc(length);
    if (!copy) return -1;
    memcpy(copy, bytes, length);
    position = rtpmidi_queue_position(queue, due);
    memset(&queue->events[position], 0, sizeof(queue->events[position]));
    queue->events[position].due = due;
    queue->events[position].sysex = copy;
    queue->events[position].sysex_length = length;
    ++queue->count;
    queue->sysex_bytes += length;
    return 0;
}

int rtpmidi_queue_pop_due(struct rtpmidi_event_queue *queue, uint64_t now,
                      struct rtpmidi_queued_event *event)
{
    if (!queue || !event || !queue->count || queue->events[0].due > now)
        return 0;
    *event = queue->events[0];
    if (event->sysex)
        queue->sysex_bytes -= event->sysex_length;
    --queue->count;
    if (queue->count)
        memmove(queue->events, queue->events + 1,
                queue->count * sizeof(queue->events[0]));
    memset(&queue->events[queue->count], 0,
           sizeof(queue->events[queue->count]));
    return 1;
}

void rtpmidi_queue_event_release(struct rtpmidi_queued_event *event)
{
    if (!event) return;
    free(event->sysex);
    event->sysex = NULL;
    event->sysex_length = 0;
    event->length = 0;
}

int rtpmidi_queue_next_due(const struct rtpmidi_event_queue *queue, uint64_t *due)
{
    if (!queue || !due || !queue->count)
        return 0;
    *due = queue->events[0].due;
    return 1;
}

size_t rtpmidi_queue_cancel_note_on(struct rtpmidi_event_queue *queue,
                                uint8_t channel, uint8_t note)
{
    size_t read_index;
    size_t write_index = 0;
    if (!queue || channel >= 16 || note >= 128)
        return 0;
    for (read_index = 0; read_index < queue->count; ++read_index) {
        const struct rtpmidi_queued_event *event = &queue->events[read_index];
        if (event->length == 3 && event->bytes[0] == (uint8_t)(0x90 | channel) &&
            event->bytes[1] == note && event->bytes[2] != 0)
            continue;
        if (write_index != read_index)
            queue->events[write_index] = *event;
        ++write_index;
    }
    read_index = queue->count - write_index;
    queue->count = write_index;
    return read_index;
}

int rtpmidi_queue_has_note_on(const struct rtpmidi_event_queue *queue,
                          uint8_t channel, uint8_t note)
{
    size_t i;
    if (!queue || channel >= 16 || note >= 128)
        return 0;
    for (i = 0; i < queue->count; ++i) {
        const struct rtpmidi_queued_event *event = &queue->events[i];
        if (event->length == 3 &&
            event->bytes[0] == (uint8_t)(0x90 | channel) &&
            event->bytes[1] == note && event->bytes[2])
            return 1;
    }
    return 0;
}
