/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The BLE MIDI 1.0 packet format: packets to a stream of MIDI bytes and
   back. Portable C. */

#ifndef BTMIDI_BLEMIDI_H
#define BTMIDI_BLEMIDI_H

#include <stddef.h>
#include <stdint.h>

#define BLEMIDI_SERVICE_UUID \
    {0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00}
#define BLEMIDI_IO_UUID \
    {0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3}

typedef int (*blemidi_byte_callback)(void *context, uint16_t timestamp,
                                          uint8_t byte);

struct blemidi_decoder {
    uint8_t sysex;
    uint16_t timestamp;
};

void blemidi_decoder_init(struct blemidi_decoder *decoder);
int blemidi_decode(struct blemidi_decoder *decoder,
                       const uint8_t *packet, size_t length,
                       blemidi_byte_callback callback, void *context);
/* Whether blemidi_decode() would take the whole packet, without decoding
   it: 0 when it would, -1 when it would stop at a malformed byte. For a
   receiver that must not pass on the first half of a broken packet. */
int blemidi_check(const struct blemidi_decoder *decoder,
                  const uint8_t *packet, size_t length);

/* The other way: MIDI bytes, as a serial port would carry them, into BLE
   MIDI packets. A packet takes as many whole messages as fit; a SysEx
   continues over as many packets as it needs. send gets each full packet,
   and flush whatever is left once the sender has no more bytes. */
#define BLEMIDI_PACKET_MIN 20   /* what the default ATT MTU carries */
#define BLEMIDI_PACKET_MAX 512

typedef int (*blemidi_packet_callback)(void *context, const uint8_t *packet,
                                       size_t length);

struct blemidi_writer {
    blemidi_packet_callback send;
    void *context;
    size_t capacity;            /* of a packet: the link's payload */
    size_t length;              /* of the packet being filled, 0 for none */
    uint8_t sysex;              /* inside a SysEx */
    uint8_t running;
    uint8_t message[3];
    uint8_t have;
    uint8_t need;
    unsigned long messages;     /* messages written, for statistics */
    uint8_t packet[BLEMIDI_PACKET_MAX];
};

void blemidi_writer_init(struct blemidi_writer *writer,
                         blemidi_packet_callback send, void *context);
/* Forgets a partial message and packet, e.g. after the link was lost. */
void blemidi_writer_reset(struct blemidi_writer *writer);
/* Takes effect with the next packet; call it between flushes. */
void blemidi_writer_capacity(struct blemidi_writer *writer, size_t capacity);
/* Both return -1 when send refused a packet; the packet is then lost. */
int blemidi_writer_put(struct blemidi_writer *writer, uint16_t timestamp,
                       uint8_t byte);
int blemidi_writer_flush(struct blemidi_writer *writer);

#endif
