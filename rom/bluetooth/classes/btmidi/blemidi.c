/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "blemidi.h"

#include <string.h>

static unsigned message_size(uint8_t status)
{
    if (status < 0x80)
        return 0;
    if (status < 0xf0)
        return (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 2 : 3;
    if (status == 0xf1 || status == 0xf3)
        return 2;
    if (status == 0xf2)
        return 3;
    if (status == 0xf6 || status >= 0xf8)
        return 1;
    return 0;
}

void blemidi_decoder_init(struct blemidi_decoder *decoder)
{
    memset(decoder, 0, sizeof(*decoder));
}

static int emit(void *context, blemidi_byte_callback callback,
                uint16_t timestamp, uint8_t byte)
{
    return callback(context, timestamp, byte) == 0 ? 0 : -1;
}

int blemidi_decode(struct blemidi_decoder *decoder,
                       const uint8_t *packet, size_t length,
                       blemidi_byte_callback callback, void *context)
{
    size_t i;
    uint8_t running = 0, pending = 0, need = 0;
    uint8_t sysex;
    uint16_t timestamp, high;
    uint8_t previous_low = 0;
    int after_timestamp = 0, have_timestamp = 0;

    if (!decoder || !packet || !callback || length < 2 ||
        (packet[0] & 0xc0) != 0x80)
        return -1;
    high = (uint16_t)(packet[0] & 0x3f) << 7;
    timestamp = decoder->timestamp;
    sysex = decoder->sysex;
    i = 1;
    if (!sysex || (packet[i] & 0x80)) {
        if (!(packet[i] & 0x80))
            return -1;
        previous_low = packet[i] & 0x7f;
        timestamp = high | previous_low;
        i++;
        after_timestamp = 1;
        have_timestamp = 1;
        if (i == length)
            return -1;
    }
    while (i < length) {
        uint8_t byte = packet[i];
        if (sysex) {
            if (byte & 0x80) {
                if (!after_timestamp) {
                    if (have_timestamp && (byte & 0x7f) < previous_low)
                        high = (high + 0x80) & 0x1fff;
                    previous_low = byte & 0x7f;
                    timestamp = high | previous_low;
                    have_timestamp = 1;
                    after_timestamp = 1;
                    i++;
                    if (i == length)
                        return -1;
                    continue;
                }
                if (byte == 0xf7) {
                    if (emit(context, callback, timestamp, byte))
                        return -1;
                    sysex = 0;
                } else if (byte >= 0xf8) {
                    if (emit(context, callback, timestamp, byte))
                        return -1;
                } else {
                    return -1;
                }
                after_timestamp = 0;
            } else {
                if (after_timestamp)
                    return -1;
                if (emit(context, callback, timestamp, byte))
                    return -1;
            }
            i++;
            continue;
        }
        if (need) {
            if (byte & 0x80)
                return -1;
            if (emit(context, callback, timestamp, byte))
                return -1;
            need--;
            i++;
            continue;
        }
        if (byte & 0x80) {
            if (!after_timestamp) {
                if (have_timestamp && (byte & 0x7f) < previous_low)
                    high = (high + 0x80) & 0x1fff;
                previous_low = byte & 0x7f;
                timestamp = high | previous_low;
                have_timestamp = 1;
                after_timestamp = 1;
                i++;
                if (i == length)
                    return -1;
                continue;
            }
            pending = byte;
            if (pending == 0xf0) {
                if (emit(context, callback, timestamp, byte))
                    return -1;
                sysex = 1;
                running = 0;
            } else {
                unsigned size = message_size(pending);
                if (!size)
                    return -1;
                if (emit(context, callback, timestamp, byte))
                    return -1;
                need = (uint8_t)(size - 1);
                if (byte < 0xf0)
                    running = byte;
            }
            after_timestamp = 0;
        } else {
            unsigned size;
            if (!running)
                return -1;
            size = message_size(running);
            if (after_timestamp || i > 1) {
                if (emit(context, callback, timestamp, running))
                    return -1;
                need = (uint8_t)(size - 1);
                after_timestamp = 0;
            }
            if (emit(context, callback, timestamp, byte))
                return -1;
            need--;
        }
        i++;
    }
    if (need || after_timestamp)
        return -1;
    decoder->timestamp = timestamp;
    decoder->sysex = sysex;
    return 0;
}

int blemidi_encode_sysex_chunk(const uint8_t *message, size_t length,
                                    size_t *offset, uint16_t timestamp,
                                    uint8_t *packet, size_t capacity,
                                    size_t *written)
{
    size_t pos, available, remaining, copy, i;
    if (!message || !offset || !packet || !written || length < 2 ||
        message[0] != 0xf0 || message[length - 1] != 0xf7 ||
        *offset >= length || capacity < 5)
        return -1;
    for (i = 1; i + 1 < length; i++)
        if (message[i] & 0x80)
            return -1;
    packet[0] = (uint8_t)(0x80 | ((timestamp >> 7) & 0x3f));
    pos = 1;
    if (*offset == 0)
        packet[pos++] = (uint8_t)(0x80 | (timestamp & 0x7f));
    available = capacity - pos;
    remaining = length - 1 - *offset;
    copy = remaining <= available - 2 ? remaining :
           (remaining > available ? available : remaining - 1);
    if (copy) {
        memcpy(packet + pos, message + *offset, copy);
        *offset += copy;
        pos += copy;
    }
    if (*offset == length - 1) {
        packet[pos++] = (uint8_t)(0x80 | (timestamp & 0x7f));
        packet[pos++] = 0xf7;
        *offset = length;
    }
    *written = pos;
    return 0;
}

int blemidi_encode_message(const uint8_t *message, size_t length,
                                uint16_t timestamp, uint8_t *packet,
                                size_t capacity, size_t *written)
{
    size_t i;
    if (!message || !packet || !written || !length || capacity < length + 2 ||
        message_size(message[0]) != length)
        return -1;
    for (i = 1; i < length; i++)
        if (message[i] & 0x80)
            return -1;
    packet[0] = (uint8_t)(0x80 | ((timestamp >> 7) & 0x3f));
    packet[1] = (uint8_t)(0x80 | (timestamp & 0x7f));
    memcpy(packet + 2, message, length);
    *written = length + 2;
    return 0;
}

void blemidi_stream_init(struct blemidi_stream *stream,
                             blemidi_message_callback callback,
                             void *context)
{
    stream->callback = callback;
    stream->context = context;
    blemidi_stream_reset(stream);
}

void blemidi_stream_reset(struct blemidi_stream *stream)
{
    blemidi_decoder_init(&stream->decoder);
    stream->message_length = stream->message_needed = 0;
    stream->sysex_length = 0;
}

static int stream_byte(void *context, uint16_t timestamp, uint8_t byte)
{
    struct blemidi_stream *stream = context;

    (void)timestamp;
    if (byte >= 0xf8) {
        /* Real-Time may appear anywhere, even inside SysEx */
        stream->callback(stream->context, &byte, 1);
        return 0;
    }
    if (stream->sysex_length) {
        if (stream->sysex_length == BTMIDI_SYSEX_MAX) {
            stream->sysex_length = 0;
            return -1;
        }
        stream->sysex[stream->sysex_length++] = byte;
        if (byte == 0xf7) {
            stream->callback(stream->context, stream->sysex,
                             stream->sysex_length);
            stream->sysex_length = 0;
        }
        return 0;
    }
    if (byte == 0xf0) {
        stream->sysex[0] = byte;
        stream->sysex_length = 1;
        stream->message_length = stream->message_needed = 0;
        return 0;
    }
    if (byte & 0x80) {
        unsigned size = message_size(byte);

        stream->message_length = stream->message_needed = 0;
        if (size == 1) {        /* Tune Request */
            stream->callback(stream->context, &byte, 1);
            return 0;
        }
        if (!size)              /* a stray F7 or an undefined status */
            return 0;
        stream->message[0] = byte;
        stream->message_length = 1;
        stream->message_needed = (uint8_t)(size - 1);
        return 0;
    }
    if (!stream->message_needed)
        return -1;
    stream->message[stream->message_length++] = byte;
    if (!--stream->message_needed) {
        stream->callback(stream->context, stream->message,
                         stream->message_length);
        stream->message_length = 0;
    }
    return 0;
}

int blemidi_stream_feed(struct blemidi_stream *stream,
                            const uint8_t *packet, size_t length)
{
    if (blemidi_decode(&stream->decoder, packet, length, stream_byte,
                           stream)) {
        blemidi_stream_reset(stream);
        return -1;
    }
    return 0;
}

void blemidi_port_names(const char *device_name, char *node, char *in,
                            char *out, size_t size)
{
    size_t n = 0, limit = size > 5 ? size - 5 : 0;

    if (device_name)
        while (n < limit && device_name[n]) {
            node[n] = device_name[n];
            n++;
        }
    while (n && node[n - 1] == ' ')
        n--;
    node[n] = 0;
    if (!n && limit >= 15) {
        strcpy(node, "BLE MIDI device");
        n = 15;
    }
    memcpy(in, node, n);
    memcpy(in + n, " In", 4);
    memcpy(out, node, n);
    memcpy(out + n, " Out", 5);
}
