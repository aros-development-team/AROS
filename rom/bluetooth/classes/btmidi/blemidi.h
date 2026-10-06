/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The BLE MIDI 1.0 packet format: decoding, encoding, and the reassembly of
   MIDI messages from one sender's packets. Portable C. */

#ifndef BTMIDI_BLEMIDI_H
#define BTMIDI_BLEMIDI_H

#include <stddef.h>
#include <stdint.h>

#include "btmidi_limits.h"

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
int blemidi_encode_message(const uint8_t *message, size_t length,
                                uint16_t timestamp, uint8_t *packet,
                                size_t capacity, size_t *written);
int blemidi_encode_sysex_chunk(const uint8_t *message, size_t length,
                                    size_t *offset, uint16_t timestamp,
                                    uint8_t *packet, size_t capacity,
                                    size_t *written);

/* Reassembles complete MIDI messages from the BLE MIDI packets of one
   sender: running status and SysEx span packets, so every connection needs
   its own stream. The callback receives each channel, System Common or
   Real-Time message, and each SysEx from F0 to F7. */
typedef void (*blemidi_message_callback)(void *context,
                                             const uint8_t *message,
                                             size_t length);

struct blemidi_stream {
    struct blemidi_decoder decoder;
    blemidi_message_callback callback;
    void *context;
    uint8_t message[3];
    uint8_t message_length;
    uint8_t message_needed;
    size_t sysex_length;
    uint8_t sysex[BTMIDI_SYSEX_MAX];
};

void blemidi_stream_init(struct blemidi_stream *stream,
                             blemidi_message_callback callback,
                             void *context);
/* Forgets a partial message, e.g. after the link was lost. */
void blemidi_stream_reset(struct blemidi_stream *stream);
/* Returns -1 for a malformed packet or an oversized SysEx; the stream is
   then reset and the next packet starts afresh. */
int blemidi_stream_feed(struct blemidi_stream *stream,
                            const uint8_t *packet, size_t length);

/* The CAMD names of a BLE MIDI device AROS connects to: the node is named
   after the device, its clusters "<name> In" (what the device plays) and
   "<name> Out" (what is sent to it). Each buffer holds size bytes; the name
   is shortened so that " Out" still fits. btmidi.class makes the ports, and other
   programs can recognise them by these names. */
void blemidi_port_names(const char *device_name, char *node, char *in,
                            char *out, size_t size);

#endif
