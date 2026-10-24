/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "applemidi/applemidi.h"

#include <string.h>

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t read32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static uint64_t read64(const uint8_t *p)
{
    return ((uint64_t)read32(p) << 32) | read32(p + 4);
}

static void write16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static void write32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

static void write64(uint8_t *p, uint64_t value)
{
    write32(p, (uint32_t)(value >> 32));
    write32(p + 4, (uint32_t)value);
}

static int exchange_command(uint16_t command)
{
    return command == APPLEMIDI_IN || command == APPLEMIDI_OK ||
           command == APPLEMIDI_NO || command == APPLEMIDI_BY;
}

uint32_t applemidi_feedback_field(uint16_t sequence)
{
    return (uint32_t)sequence << 16;
}

uint16_t applemidi_feedback_acked(uint32_t field, uint16_t last_sent)
{
    uint16_t first = (uint16_t)(field >> 16);
    uint16_t last = (uint16_t)field;

    return (uint16_t)(last_sent - first) <= (uint16_t)(last_sent - last) ?
           first : last;
}

int applemidi_decode(const uint8_t *data, size_t length,
                    struct applemidi_packet *packet)
{
    uint16_t command;
    size_t i;

    if (!data || !packet || length < 4 || read16(data) != 0xffff)
        return APPLEMIDI_NOT_APPLEMIDI;
    command = read16(data + 2);
    memset(packet, 0, sizeof(*packet));
    packet->command = (enum applemidi_command)command;
    if (exchange_command(command)) {
        if (length < 16)
            return APPLEMIDI_TRUNCATED;
        if (read32(data + 4) != 2)
            return APPLEMIDI_BAD_VERSION;
        packet->token = read32(data + 8);
        packet->ssrc = read32(data + 12);
        if (length == 16)
            return APPLEMIDI_DECODED;
        for (i = 16; i < length && data[i]; ++i) {}
        packet->name = data + 16;
        packet->name_length = i - 16;
        return APPLEMIDI_DECODED;
    }
    if (command == APPLEMIDI_CK) {
        if (length < 36)
            return APPLEMIDI_TRUNCATED;
        if (data[8] > 2)
            return APPLEMIDI_MALFORMED;
        packet->ssrc = read32(data + 4);
        packet->sync_count = data[8];
        for (i = 0; i < 3; ++i)
            packet->timestamps[i] = read64(data + 12 + i * 8);
        return APPLEMIDI_DECODED;
    }
    if (command == APPLEMIDI_RS) {
        if (length < 12)
            return APPLEMIDI_TRUNCATED;
        packet->ssrc = read32(data + 4);
        packet->feedback_sequence = read32(data + 8);
        return APPLEMIDI_DECODED;
    }
    return APPLEMIDI_UNKNOWN_COMMAND;
}

int applemidi_encode(const struct applemidi_packet *packet, uint8_t *data,
                    size_t capacity, size_t *length)
{
    size_t total;
    size_t i;

    if (!packet || !data || !length)
        return -1;
    if (exchange_command((uint16_t)packet->command)) {
        if (packet->name_length && !packet->name)
            return -1;
        if (packet->name_length > SIZE_MAX - 17)
            return -1;
        total = 16 + (packet->name ? packet->name_length + 1 : 0);
        if (capacity < total)
            return -1;
        for (i = 0; i < packet->name_length; ++i)
            if (!packet->name[i])
                return -1;
        write16(data, 0xffff);
        write16(data + 2, (uint16_t)packet->command);
        write32(data + 4, 2);
        write32(data + 8, packet->token);
        write32(data + 12, packet->ssrc);
        if (packet->name) {
            memcpy(data + 16, packet->name, packet->name_length);
            data[16 + packet->name_length] = 0;
        }
    } else if (packet->command == APPLEMIDI_CK) {
        if (capacity < 36 || packet->sync_count > 2)
            return -1;
        total = 36;
        write16(data, 0xffff);
        write16(data + 2, APPLEMIDI_CK);
        write32(data + 4, packet->ssrc);
        data[8] = packet->sync_count;
        memset(data + 9, 0, 3);
        for (i = 0; i < 3; ++i)
            write64(data + 12 + i * 8, packet->timestamps[i]);
    } else if (packet->command == APPLEMIDI_RS) {
        if (capacity < 12)
            return -1;
        total = 12;
        write16(data, 0xffff);
        write16(data + 2, APPLEMIDI_RS);
        write32(data + 4, packet->ssrc);
        write32(data + 8, packet->feedback_sequence);
    } else {
        return -1;
    }
    *length = total;
    return 0;
}
