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

void blemidi_writer_init(struct blemidi_writer *writer,
                         blemidi_packet_callback send, void *context)
{
    memset(writer, 0, sizeof(*writer));
    writer->send = send;
    writer->context = context;
    writer->capacity = BLEMIDI_PACKET_MIN;
}

void blemidi_writer_reset(struct blemidi_writer *writer)
{
    writer->length = 0;
    writer->sysex = writer->running = 0;
    writer->have = writer->need = 0;
}

void blemidi_writer_capacity(struct blemidi_writer *writer, size_t capacity)
{
    if (capacity < BLEMIDI_PACKET_MIN)
        capacity = BLEMIDI_PACKET_MIN;
    if (capacity > BLEMIDI_PACKET_MAX)
        capacity = BLEMIDI_PACKET_MAX;
    if (!writer->length)
        writer->capacity = capacity;
}

int blemidi_writer_flush(struct blemidi_writer *writer)
{
    int result = 0;

    if (writer->length > 1 &&
        writer->send(writer->context, writer->packet, writer->length))
        result = -1;
    writer->length = 0;
    return result;
}

/* Makes room for count more bytes, in a new packet if they do not fit.
   A packet that continues a SysEx starts with data after its header. */
static int writer_room(struct blemidi_writer *writer, uint16_t timestamp,
                       size_t count)
{
    int result = 0;

    if (writer->length && writer->length + count > writer->capacity)
        result = blemidi_writer_flush(writer);
    if (!writer->length) {
        writer->packet[0] = (uint8_t)(0x80 | ((timestamp >> 7) & 0x3f));
        writer->length = 1;
    }
    return result;
}

int blemidi_writer_put(struct blemidi_writer *writer, uint16_t timestamp,
                       uint8_t byte)
{
    uint8_t low = (uint8_t)(0x80 | (timestamp & 0x7f));
    int result = 0;

    if (byte >= 0xf8) {
        /* Real-Time goes out at once, also inside another message */
        result = writer_room(writer, timestamp, 2);
        writer->packet[writer->length++] = low;
        writer->packet[writer->length++] = byte;
        writer->messages++;
        return result;
    }
    if (writer->sysex) {
        if (!(byte & 0x80)) {
            result = writer_room(writer, timestamp, 1);
            writer->packet[writer->length++] = byte;
            return result;
        }
        /* F7, or a status that cuts the SysEx short: it ends either way */
        result = writer_room(writer, timestamp, 2);
        writer->packet[writer->length++] = low;
        writer->packet[writer->length++] = 0xf7;
        writer->sysex = 0;
        if (byte == 0xf7)
            return result;
    }
    if (byte == 0xf0) {
        writer->have = writer->need = writer->running = 0;
        if (writer_room(writer, timestamp, 2))
            result = -1;
        writer->packet[writer->length++] = low;
        writer->packet[writer->length++] = byte;
        writer->sysex = 1;
        writer->messages++;
        return result;
    }
    if (byte & 0x80) {
        unsigned size = message_size(byte);

        writer->have = writer->need = 0;
        writer->running = byte < 0xf0 ? byte : 0;
        if (!size)              /* a stray F7 or an undefined status */
            return result;
        writer->message[0] = byte;
        writer->have = 1;
        writer->need = (uint8_t)(size - 1);
    } else {
        if (!writer->need) {
            if (!writer->running)
                return result;  /* data that belongs to nothing */
            writer->message[0] = writer->running;
            writer->have = 1;
            writer->need = (uint8_t)(message_size(writer->running) - 1);
        }
        writer->message[writer->have++] = byte;
        writer->need--;
    }
    if (!writer->need) {
        if (writer_room(writer, timestamp, (size_t)writer->have + 1))
            result = -1;
        writer->packet[writer->length++] = low;
        memcpy(writer->packet + writer->length, writer->message, writer->have);
        writer->length += writer->have;
        writer->have = 0;
        writer->messages++;
    }
    return result;
}
