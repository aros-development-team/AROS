/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "netmidi/queue.h"

#include <stdlib.h>
#include <string.h>

void netmidi_queue_reset(struct netmidi_event_queue *queue)
{
    size_t i;
    if (!queue) return;
    for (i = 0; i < queue->count; ++i)
        free(queue->events[i].sysex);
    memset(queue->events, 0, sizeof(queue->events));
    queue->count = 0;
    queue->sysex_bytes = 0;
}

static size_t netmidi_queue_position(struct netmidi_event_queue *queue, uint64_t due)
{
    size_t position = queue->count;
    while (position && queue->events[position - 1].due > due) {
        queue->events[position] = queue->events[position - 1];
        --position;
    }
    return position;
}

int netmidi_queue_push(struct netmidi_event_queue *queue, uint64_t due,
                  const uint8_t *bytes, size_t length)
{
    size_t position;
    if (!queue || !bytes || length == 0 || length > 3 ||
        queue->count == NETMIDI_EVENT_QUEUE_CAP)
        return -1;
    position = netmidi_queue_position(queue, due);
    memset(&queue->events[position], 0, sizeof(queue->events[position]));
    queue->events[position].due = due;
    queue->events[position].length = (uint8_t)length;
    memcpy(queue->events[position].bytes, bytes, length);
    ++queue->count;
    return 0;
}

int netmidi_queue_push_sysex(struct netmidi_event_queue *queue, uint64_t due,
                        const uint8_t *bytes, size_t length)
{
    uint8_t *copy;
    size_t position;
    if (!queue || !bytes || length < 2 || bytes[0] != 0xf0 ||
        bytes[length - 1] != 0xf7 || queue->count == NETMIDI_EVENT_QUEUE_CAP ||
        length > NETMIDI_EVENT_QUEUE_SYSEX_BYTES - queue->sysex_bytes)
        return -1;
    copy = malloc(length);
    if (!copy) return -1;
    memcpy(copy, bytes, length);
    position = netmidi_queue_position(queue, due);
    memset(&queue->events[position], 0, sizeof(queue->events[position]));
    queue->events[position].due = due;
    queue->events[position].sysex = copy;
    queue->events[position].sysex_length = length;
    ++queue->count;
    queue->sysex_bytes += length;
    return 0;
}

int netmidi_queue_pop_due(struct netmidi_event_queue *queue, uint64_t now,
                      struct netmidi_queued_event *event)
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

void netmidi_queue_event_release(struct netmidi_queued_event *event)
{
    if (!event) return;
    free(event->sysex);
    event->sysex = NULL;
    event->sysex_length = 0;
    event->length = 0;
}

int netmidi_queue_next_due(const struct netmidi_event_queue *queue, uint64_t *due)
{
    if (!queue || !due || !queue->count)
        return 0;
    *due = queue->events[0].due;
    return 1;
}

size_t netmidi_queue_cancel_note_on(struct netmidi_event_queue *queue,
                                uint8_t channel, uint8_t note)
{
    size_t read_index;
    size_t write_index = 0;
    if (!queue || channel >= 16 || note >= 128)
        return 0;
    for (read_index = 0; read_index < queue->count; ++read_index) {
        const struct netmidi_queued_event *event = &queue->events[read_index];
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

int netmidi_queue_has_note_on(const struct netmidi_event_queue *queue,
                          uint8_t channel, uint8_t note)
{
    size_t i;
    if (!queue || channel >= 16 || note >= 128)
        return 0;
    for (i = 0; i < queue->count; ++i) {
        const struct netmidi_queued_event *event = &queue->events[i];
        if (event->length == 3 &&
            event->bytes[0] == (uint8_t)(0x90 | channel) &&
            event->bytes[1] == note && event->bytes[2])
            return 1;
    }
    return 0;
}
