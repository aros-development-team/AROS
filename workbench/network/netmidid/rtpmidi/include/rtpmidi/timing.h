/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef RTPMIDI_TIMING_H
#define RTPMIDI_TIMING_H

#include <stddef.h>
#include <stdint.h>

#define RTPMIDI_EVENT_QUEUE_CAP 256
#define RTPMIDI_EVENT_QUEUE_SYSEX_BYTES 65536

struct rtpmidi_queued_event {
    uint64_t due;
    uint8_t bytes[3];
    uint8_t length;
    uint8_t *sysex;
    size_t sysex_length;
};

struct rtpmidi_event_queue {
    struct rtpmidi_queued_event events[RTPMIDI_EVENT_QUEUE_CAP];
    size_t count;
    size_t sysex_bytes;
};

/* The result converts timestamps in the peer clock into local ticks. */
int rtpmidi_clock_offset(const uint64_t timestamps[3], int is_initiator,
                    int64_t *peer_to_local);

/* Resolve a 32-bit RTP timestamp near the current peer time. */
/* An event due further ahead than this is not held: ten seconds, in
 * ten-thousandths. No latency setting comes near it; a time that far off
 * means the clocks are not known, not that the sender meant it. The
 * AppleMIDI path drops such an event and says so; the Network MIDI 2.0
 * path plays it at once. */
#define RTPMIDI_QUEUE_HORIZON 100000u

int rtpmidi_clock_due(uint32_t timestamp, uint64_t local_now,
                 int64_t peer_to_local, uint64_t *local_due);

void rtpmidi_queue_reset(struct rtpmidi_event_queue *queue);
int rtpmidi_queue_push(struct rtpmidi_event_queue *queue, uint64_t due,
                  const uint8_t *bytes, size_t length);
/* SysEx data is copied into bounded queue-owned storage. */
int rtpmidi_queue_push_sysex(struct rtpmidi_event_queue *queue, uint64_t due,
                        const uint8_t *bytes, size_t length);
/* A popped SysEx event owns its payload until rtpmidi_queue_event_release(). */
int rtpmidi_queue_pop_due(struct rtpmidi_event_queue *queue, uint64_t now,
                      struct rtpmidi_queued_event *event);
void rtpmidi_queue_event_release(struct rtpmidi_queued_event *event);
int rtpmidi_queue_next_due(const struct rtpmidi_event_queue *queue, uint64_t *due);
size_t rtpmidi_queue_cancel_note_on(struct rtpmidi_event_queue *queue,
                                uint8_t channel, uint8_t note);
int rtpmidi_queue_has_note_on(const struct rtpmidi_event_queue *queue,
                          uint8_t channel, uint8_t note);

#endif
