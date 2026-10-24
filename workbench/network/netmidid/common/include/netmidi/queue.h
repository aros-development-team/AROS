/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_QUEUE_H
#define NETMIDI_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#define NETMIDI_EVENT_QUEUE_CAP 256
#define NETMIDI_EVENT_QUEUE_SYSEX_BYTES 65536

struct netmidi_queued_event {
    uint64_t due;
    uint8_t bytes[3];
    uint8_t length;
    uint8_t *sysex;
    size_t sysex_length;
};

struct netmidi_event_queue {
    struct netmidi_queued_event events[NETMIDI_EVENT_QUEUE_CAP];
    size_t count;
    size_t sysex_bytes;
};

void netmidi_queue_reset(struct netmidi_event_queue *queue);
int netmidi_queue_push(struct netmidi_event_queue *queue, uint64_t due,
                  const uint8_t *bytes, size_t length);
/* SysEx data is copied into bounded queue-owned storage. */
int netmidi_queue_push_sysex(struct netmidi_event_queue *queue, uint64_t due,
                        const uint8_t *bytes, size_t length);
/* A popped SysEx event owns its payload until netmidi_queue_event_release(). */
int netmidi_queue_pop_due(struct netmidi_event_queue *queue, uint64_t now,
                      struct netmidi_queued_event *event);
void netmidi_queue_event_release(struct netmidi_queued_event *event);
int netmidi_queue_next_due(const struct netmidi_event_queue *queue, uint64_t *due);
size_t netmidi_queue_cancel_note_on(struct netmidi_event_queue *queue,
                                uint8_t channel, uint8_t note);
int netmidi_queue_has_note_on(const struct netmidi_event_queue *queue,
                          uint8_t channel, uint8_t note);

#endif
