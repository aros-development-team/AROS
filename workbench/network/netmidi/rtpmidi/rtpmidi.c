/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "rtpmidi/rtpmidi.h"

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

/* The length of a message by its status: tables II, V and VI of the
   Complete MIDI 1.0 Detailed Specification 96.1 (4.2.1). 0 for SysEx and
   for F4, F5, F9 and FD, which the tables leave undefined. */
static size_t short_length(uint8_t status)
{
    if (status >= 0x80 && status <= 0xef)
        return (status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0 ? 2 : 3;
    switch (status) {
    case 0xf1: case 0xf3: return 2;
    case 0xf2: return 3;
    case 0xf6: case 0xf8: case 0xfa: case 0xfb:
    case 0xfc: case 0xfe: case 0xff: return 1;
    default: return 0;
    }
}

int rtpmidi_short_valid(const uint8_t *midi, size_t midi_length)
{
    size_t i;

    if (!midi || !midi_length || midi_length != short_length(midi[0]))
        return 0;
    for (i = 1; i < midi_length; ++i)
        if (midi[i] & 0x80)
            return 0;
    return 1;
}

int rtpmidi_encode_short(uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                        const uint8_t *midi, size_t midi_length,
                        uint8_t *data, size_t capacity, size_t *length)
{
    if (!data || !length || !rtpmidi_short_valid(midi, midi_length))
        return -1;
    return rtpmidi_encode_list(sequence, timestamp, ssrc, midi, midi_length,
                              data, capacity, length);
}

int rtpmidi_encode_list(uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                       const uint8_t *midi, size_t midi_length,
                       uint8_t *data, size_t capacity, size_t *length)
{
    size_t header_length;
    if ((!midi && midi_length) || !data || !length || midi_length > 4095)
        return -1;
    header_length = midi_length < 16 ? 13 : 14;
    if (capacity < header_length + midi_length)
        return -1;
    data[0] = 0x80;
    /* The marker bit is left clear, with commands or without. RFC 6295
     * section 2.1 would have it set when there are commands, but AppleMIDI
     * implementations do not follow that: node-rtpmidi and Benoit Bouchez's
     * RTP-MIDI class both send it clear, and the second takes no packet
     * that has it set. */
    data[1] = RTPMIDI_PAYLOAD_TYPE;
    write16(data + 2, sequence);
    write32(data + 4, timestamp);
    write32(data + 8, ssrc);
    data[12] = (uint8_t)((header_length == 14 ? 0x80 : 0) |
                         ((midi_length >> (header_length == 14 ? 8 : 0)) & 0x0f));
    if (header_length == 14)
        data[13] = (uint8_t)midi_length;
    if (midi_length)
        memcpy(data + header_length, midi, midi_length);
    *length = header_length + midi_length;
    return 0;
}

/* RFC 3550 section 5.1 for the fixed header, RFC 6295 section 2.1 and 2.2
 * for what follows it. */
int rtpmidi_decode(const uint8_t *data, size_t length,
                  struct rtpmidi_packet *packet)
{
    size_t offset;
    size_t header_length;
    size_t midi_length;
    size_t payload_length;
    uint8_t flags;

    if (!data || !packet || length < 12 || (data[0] >> 6) != 2 ||
        (data[1] & 0x7f) != RTPMIDI_PAYLOAD_TYPE)
        return -1;
    /* contributing sources, four bytes each */
    offset = 12 + 4 * (size_t)(data[0] & 0x0f);
    if (length < offset)
        return -1;
    if (data[0] & 0x10) {
        /* a header extension: two bytes of profile, its length in words */
        if (length - offset < 4)
            return -1;
        offset += 4 + 4 * (size_t)read16(data + offset + 2);
        if (length < offset)
            return -1;
    }
    if (data[0] & 0x20) {
        /* padding: the last byte counts it, itself included */
        size_t padding = data[length - 1];
        if (!padding || padding > length - offset)
            return -1;
        length -= padding;
    }
    if (length == offset)
        return -1;
    flags = data[offset];
    header_length = (flags & 0x80) ? 2 : 1;
    payload_length = length - offset;
    if (payload_length < header_length)
        return -1;
    midi_length = flags & 0x0f;
    if (flags & 0x80)
        midi_length = (midi_length << 8) | data[offset + 1];
    if (midi_length > payload_length - header_length)
        return -1;
    if (!(flags & 0x40) && payload_length != header_length + midi_length)
        return -1;
    packet->sequence = read16(data + 2);
    packet->timestamp = read32(data + 4);
    packet->ssrc = read32(data + 8);
    packet->midi = data + offset + header_length;
    packet->midi_length = midi_length;
    packet->journal = (flags & 0x40) ?
                      data + offset + header_length + midi_length : 0;
    packet->journal_length = payload_length - header_length - midi_length;
    packet->z = !!(flags & 0x20);
    /* P says the first channel command's status was not in the sender's
       own source stream, which had it by running status (RFC 6295
       section 3.2). It is about that stream, not about this list, which
       has the status either way: nothing is done with it. We send it
       clear, since what we send was handed to us as whole messages. */
    packet->p = !!(flags & 0x10);
    if ((flags & 0x40) && !packet->journal_length)
        return -1;
    return 0;
}

int rtpmidi_journal_decode(const uint8_t *data, size_t length,
                      struct rtpmidi_journal *journal)
{
    size_t offset = 3;
    size_t section_length;
    size_t count;
    size_t i;
    int previous_channel = -1;

    if (!data || !journal || length < 3)
        return -1;
    memset(journal, 0, sizeof(*journal));
    journal->checkpoint = read16(data + 1);
    journal->single_packet_safe = !!(data[0] & 0x80);
    journal->enhanced_controllers = !!(data[0] & 0x10);
    if (data[0] & 0x40) {
        if (length - offset < 2)
            return -1;
        section_length = ((size_t)(data[offset] & 3) << 8) |
                         data[offset + 1];
        if (section_length < 2 || section_length > length - offset)
            return -1;
        journal->system = data + offset;
        journal->system_length = section_length;
        offset += section_length;
    }
    count = (data[0] & 0x20) ? (size_t)(data[0] & 0x0f) + 1 : 0;
    for (i = 0; i < count; ++i) {
        struct rtpmidi_journal_channel *channel = &journal->channels[i];
        if (length - offset < 3)
            return -1;
        section_length = ((size_t)(data[offset] & 3) << 8) |
                         data[offset + 1];
        if (section_length < 3 || section_length > length - offset)
            return -1;
        channel->number = (data[offset] >> 3) & 0x0f;
        if ((data[offset] & 0x04) && !journal->enhanced_controllers)
            return -1;
        if (channel->number <= previous_channel)
            return -1;
        previous_channel = channel->number;
        channel->single_packet_safe = !!(data[offset] & 0x80);
        channel->chapters = data[offset + 2];
        channel->data = data + offset;
        channel->length = section_length;
        offset += section_length;
    }
    if (offset != length)
        return -1;
    journal->channel_count = count;
    return 0;
}

int rtpmidi_journal_covers_gap(uint16_t previous, uint16_t current,
                          uint16_t checkpoint)
{
    uint16_t advance = (uint16_t)(current - previous);
    uint16_t first_missing = (uint16_t)(previous + 1);
    return advance > 1 && advance < 0x8000 &&
           (uint16_t)(first_missing - checkpoint) < 0x8000;
}

static int skip_undefined_common(const uint8_t *data, size_t length,
                                 size_t *offset)
{
    size_t size;
    if (length - *offset < 2) return -1;
    size = ((size_t)(data[*offset] & 3) << 8) | data[*offset + 1];
    if (size < 2 || size > length - *offset) return -1;
    *offset += size;
    return 0;
}

static int skip_undefined_realtime(const uint8_t *data, size_t length,
                                   size_t *offset)
{
    size_t size;
    if (length - *offset < 1) return -1;
    size = data[*offset] & 0x1f;
    if (size < 1 || size > length - *offset) return -1;
    *offset += size;
    return 0;
}

static int valid_mtc_nibbles(const uint8_t nibbles[8])
{
    unsigned int rate = (nibbles[7] >> 1) & 3;
    unsigned int fps = rate == 0 ? 24 : rate == 1 ? 25 : 30;
    unsigned int frame = nibbles[0] | ((nibbles[1] & 1) << 4);
    unsigned int second = nibbles[2] | ((nibbles[3] & 3) << 4);
    unsigned int minute = nibbles[4] | ((nibbles[5] & 3) << 4);
    unsigned int hour = nibbles[6] | ((nibbles[7] & 1) << 4);
    return !(nibbles[7] & 8) && frame < fps && second < 60 &&
           minute < 60 && hour < 24;
}

int rtpmidi_journal_decode_system(const struct rtpmidi_journal *journal,
                             struct rtpmidi_journal_system_state *state)
{
    const uint8_t *data;
    size_t length;
    size_t offset = 2;
    uint8_t toc;
    uint8_t flags;
    int decoded = 0;
    if (!journal || !state) return -1;
    memset(state, 0, sizeof(*state));
    if (!journal->system) return 0;
    data = journal->system;
    length = journal->system_length;
    if (length < 2 || (((size_t)(data[0] & 3) << 8) | data[1]) != length)
        return -1;
    toc = data[0];
    if (toc & 0x40) {
        if (offset >= length) return -1;
        flags = data[offset++];
        if (flags & 0x40) {
            if (offset >= length) return -1;
            state->has_reset = 1;
            state->reset_single_packet_safe = !!(data[offset] & 0x80);
            state->reset_count = data[offset++] & 0x7f;
            decoded = 1;
        }
        if (flags & 0x20) {
            if (offset >= length) return -1;
            state->has_tune_request = 1;
            state->tune_single_packet_safe = !!(data[offset] & 0x80);
            state->tune_count = data[offset++] & 0x7f;
            decoded = 1;
        }
        if (flags & 0x10) {
            if (offset >= length) return -1;
            state->has_song_select = 1;
            state->song_single_packet_safe = !!(data[offset] & 0x80);
            state->song = data[offset++] & 0x7f;
            decoded = 1;
        }
        if ((flags & 0x08) && skip_undefined_common(data, length, &offset))
            return -1;
        if ((flags & 0x04) && skip_undefined_common(data, length, &offset))
            return -1;
        if ((flags & 0x02) && skip_undefined_realtime(data, length, &offset))
            return -1;
        if ((flags & 0x01) && skip_undefined_realtime(data, length, &offset))
            return -1;
    }
    if (toc & 0x20) {
        if (offset >= length) return -1;
        state->has_active_sense = 1;
        state->active_sense_single_packet_safe = !!(data[offset] & 0x80);
        state->active_sense_count = data[offset++] & 0x7f;
        decoded = 1;
    }
    if (toc & 0x10) {
        if (offset >= length) return -1;
        flags = data[offset++];
        state->has_sequencer = 1;
        state->sequencer_single_packet_safe = !!(flags & 0x80);
        state->sequencer_running = !!(flags & 0x40);
        state->downbeat_played = !!(flags & 0x20);
        state->has_clock = !!(flags & 0x10);
        state->has_time_tools = !!(flags & 0x08);
        state->clock = (uint32_t)(flags & 7) << 16;
        if (state->has_clock) {
            if (length - offset < 2) return -1;
            state->clock |= (uint32_t)data[offset] << 8 | data[offset + 1];
            offset += 2;
        }
        if (state->has_time_tools) {
            if (length - offset < 3) return -1;
            state->time_tools = (uint32_t)data[offset] << 16 |
                                (uint32_t)data[offset + 1] << 8 |
                                data[offset + 2];
            offset += 3;
        }
        decoded = 1;
    }
    if (toc & 0x08) {
        size_t i;
        if (offset >= length) return -1;
        flags = data[offset++];
        state->has_mtc = 1;
        state->mtc_single_packet_safe = !!(flags & 0x80);
        state->mtc_has_complete = !!(flags & 0x40);
        state->mtc_has_partial = !!(flags & 0x20);
        state->mtc_complete_quarter_frame = !!(flags & 0x10);
        state->mtc_reverse = !!(flags & 0x08);
        state->mtc_point = flags & 7;
        if (!state->mtc_has_complete && state->mtc_complete_quarter_frame)
            return -1;
        if (!state->mtc_has_partial &&
            state->mtc_point != (state->mtc_reverse ? 0 : 7))
            return -1;
        if (state->mtc_has_partial &&
            ((!state->mtc_reverse && state->mtc_point > 6) ||
             (state->mtc_reverse && state->mtc_point < 1)))
            return -1;
        if (state->mtc_has_complete) {
            if (length - offset < 4) return -1;
            if (state->mtc_complete_quarter_frame) {
                for (i = 0; i < 8; ++i)
                    state->mtc_complete[i] =
                        (uint8_t)((data[offset + i / 2] >>
                                   (i % 2 ? 0 : 4)) & 0x0f);
                if (!valid_mtc_nibbles(state->mtc_complete)) return -1;
            } else {
                for (i = 0; i < 4; ++i)
                    if (data[offset + i] & 0x80) return -1;
                memcpy(state->mtc_complete, data + offset, 4);
            }
            offset += 4;
        }
        if (state->mtc_has_partial) {
            if (length - offset < 4) return -1;
            for (i = 0; i < 8; ++i)
                state->mtc_partial[i] =
                    (uint8_t)((data[offset + i / 2] >>
                               (i % 2 ? 0 : 4)) & 0x0f);
            offset += 4;
        }
        decoded = 1;
    }
    if (toc & 0x04) {
        if (offset >= length) return -1;
        state->sysex = data + offset;
        state->sysex_length = length - offset;
        offset = length;
        decoded = 1;
    }
    if (offset != length) return -1;
    return decoded;
}

int rtpmidi_journal_decode_sysex(const struct rtpmidi_journal_system_state *state,
                            struct rtpmidi_journal_sysex *sysex)
{
    size_t offset = 0;
    if (!state || !sysex) return -1;
    memset(sysex, 0, sizeof(*sysex));
    if (!state->sysex) return 0;
    while (offset < state->sysex_length) {
        struct rtpmidi_journal_sysex_log *log;
        uint8_t flags;
        if (sysex->count == RTPMIDI_JOURNAL_SYSEX_LOGS) return -1;
        log = &sysex->logs[sysex->count++];
        flags = state->sysex[offset++];
        log->single_packet_safe = !!(flags & 0x80);
        log->list_tool = !!(flags & 0x04);
        log->status = flags & 3;
        if (flags & 0x40) {
            if (offset >= state->sysex_length) return -1;
            log->has_tcount = 1;
            log->tcount = state->sysex[offset++];
        }
        if (flags & 0x20) {
            if (offset >= state->sysex_length) return -1;
            log->has_count = 1;
            log->count = state->sysex[offset++];
        }
        if (flags & 0x10) {
            uint8_t value;
            unsigned int bytes = 0;
            uint32_t first = 0;
            log->has_first = 1;
            do {
                if (offset >= state->sysex_length || bytes++ == 4)
                    return -1;
                value = state->sysex[offset++];
                first = (first << 7) | (value & 0x7f);
            } while (value & 0x80);
            log->first = first;
        }
        if (flags & 0x08) {
            size_t start = offset;
            while (offset < state->sysex_length &&
                   !(state->sysex[offset++] & 0x80)) {}
            if (offset == start ||
                !(state->sysex[offset - 1] & 0x80))
                return -1;
            log->data = state->sysex + start;
            log->data_length = offset - start;
        }
    }
    return sysex->count ? 1 : 0;
}

int rtpmidi_journal_decode_aftertouch(const struct rtpmidi_journal_channel *channel,
                                  struct rtpmidi_journal_aftertouch *aftertouch)
{
    const uint8_t *data;
    size_t length;
    size_t offset = 3;
    size_t size;
    size_t logs;
    if (!channel || !aftertouch || !channel->data || channel->length < 3)
        return -1;
    memset(aftertouch, 0, sizeof(*aftertouch));
    if (!(channel->chapters & 0x03)) return 0;
    data = channel->data;
    length = channel->length;
    if (channel->chapters & 0x80) {
        if (length - offset < 3) return -1;
        offset += 3;
    }
    if (channel->chapters & 0x40) {
        if (length - offset < 1) return -1;
        size = 1 + 2 * ((size_t)(data[offset] & 0x7f) + 1);
        if (size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x20) {
        if (length - offset < 2) return -1;
        size = ((size_t)(data[offset] & 3) << 8) | data[offset + 1];
        if (size < 2 || size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x10) {
        if (length - offset < 2) return -1;
        offset += 2;
    }
    if (channel->chapters & 0x08) {
        size_t off_count;
        unsigned int low;
        unsigned int high;

        if (length - offset < 2) return -1;
        logs = data[offset] & 0x7f;
        low = data[offset + 1] >> 4;
        high = data[offset + 1] & 0x0f;
        if (low <= high)
            off_count = high - low + 1;
        else if (low == 15 && (high == 0 || high == 1))
            off_count = 0;
        else
            return -1;
        if (logs == 127 && low == 15 && high == 0) ++logs;
        size = 2 + logs * 2 + off_count;
        if (size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x04) {
        if (length - offset < 1) return -1;
        size = 1 + 2 * ((size_t)(data[offset] & 0x7f) + 1);
        if (size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x02) {
        if (length - offset < 1) return -1;
        aftertouch->has_channel_pressure = 1;
        aftertouch->channel_single_packet_safe = !!(data[offset] & 0x80);
        aftertouch->channel_pressure = data[offset] & 0x7f;
        ++offset;
    }
    if (channel->chapters & 0x01) {
        size_t i;
        uint8_t seen[16] = {0};

        if (length - offset < 1) return -1;
        logs = (data[offset] & 0x7f) + 1;
        size = 1 + 2 * logs;
        if (size > length - offset) return -1;
        ++offset;
        aftertouch->poly_count = logs;
        for (i = 0; i < logs; ++i) {
            struct rtpmidi_journal_poly_pressure *log = &aftertouch->poly[i];
            uint8_t number = data[offset] & 0x7f;
            if (seen[number / 8] & (0x80u >> (number % 8))) return -1;
            seen[number / 8] |= (uint8_t)(0x80u >> (number % 8));
            log->number = number;
            log->single_packet_safe = !!(data[offset] & 0x80);
            log->reset_notes = !!(data[offset + 1] & 0x80);
            log->pressure = data[offset + 1] & 0x7f;
            offset += 2;
        }
    }
    return offset == length ? 1 : -1;
}

int rtpmidi_journal_decode_notes(const struct rtpmidi_journal_channel *channel,
                            struct rtpmidi_journal_notes *notes)
{
    const uint8_t *data;
    size_t length;
    size_t offset = 3;
    size_t size;
    size_t logs;
    size_t off_count;
    unsigned int low;
    unsigned int high;
    uint8_t seen[16] = {0};
    size_t i;

    if (!channel || !notes || !channel->data ||
        channel->length < 3)
        return -1;
    memset(notes, 0, sizeof(*notes));
    if (!(channel->chapters & 0x08))
        return 0;
    data = channel->data;
    length = channel->length;
    if (channel->chapters & 0x80) {
        if (length - offset < 3) return -1;
        offset += 3; /* Chapter P. */
    }
    if (channel->chapters & 0x40) {
        if (length - offset < 1)
            return -1;
        size = 1 + 2 * ((size_t)(data[offset] & 0x7f) + 1);
        if (size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x20) {
        if (length - offset < 2) return -1;
        size = ((size_t)(data[offset] & 3) << 8) | data[offset + 1];
        if (size < 2 || size > length - offset) return -1;
        offset += size; /* Chapter M. */
    }
    if (channel->chapters & 0x10) {
        if (length - offset < 2) return -1;
        offset += 2; /* Chapter W. */
    }
    if (length - offset < 2) return -1;
    notes->offbits_single_packet_safe = !!(data[offset] & 0x80);
    logs = data[offset] & 0x7f;
    low = data[offset + 1] >> 4;
    high = data[offset + 1] & 0x0f;
    if (low <= high)
        off_count = high - low + 1;
    else if (low == 15 && (high == 0 || high == 1))
        off_count = 0;
    else
        return -1;
    if (logs == 127 && low == 15 && high == 0)
        ++logs;
    size = 2 + logs * 2 + off_count;
    if (size > length - offset) return -1;
    notes->log_count = logs;
    for (i = 0; i < logs; ++i) {
        const uint8_t *log = data + offset + 2 + i * 2;
        struct rtpmidi_journal_note_log *entry = &notes->logs[i];
        uint8_t number = log[0] & 0x7f;
        if (!((log[1] & 0x7f)) ||
            (seen[number / 8] & (0x80u >> (number % 8))))
            return -1;
        seen[number / 8] |= (uint8_t)(0x80u >> (number % 8));
        entry->number = number;
        entry->velocity = log[1] & 0x7f;
        entry->single_packet_safe = !!(log[0] & 0x80);
        entry->simultaneous = !!(log[1] & 0x80);
    }
    if (off_count)
        memcpy(notes->offbits + low,
               data + offset + 2 + logs * 2, off_count);
    for (i = 0; i < 16; ++i)
        if (seen[i] & notes->offbits[i])
            return -1;
    return 1;
}

int rtpmidi_journal_decode_note_extras(
    const struct rtpmidi_journal_channel *channel,
    struct rtpmidi_journal_note_extras *extras)
{
    const uint8_t *data;
    size_t length;
    size_t offset = 3;
    size_t size;
    size_t logs;
    size_t i;
    uint8_t seen_count[16] = {0};
    uint8_t seen_velocity[16] = {0};

    if (!channel || !extras || !channel->data || channel->length < 3)
        return -1;
    memset(extras, 0, sizeof(*extras));
    if (!(channel->chapters & 0x04))
        return 0;
    data = channel->data;
    length = channel->length;
    if (channel->chapters & 0x80) {
        if (length - offset < 3) return -1;
        offset += 3;
    }
    if (channel->chapters & 0x40) {
        if (length - offset < 1) return -1;
        size = 1 + 2 * ((size_t)(data[offset] & 0x7f) + 1);
        if (size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x20) {
        if (length - offset < 2) return -1;
        size = ((size_t)(data[offset] & 3) << 8) | data[offset + 1];
        if (size < 2 || size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x10) {
        if (length - offset < 2) return -1;
        offset += 2;
    }
    if (channel->chapters & 0x08) {
        size_t off_count;
        unsigned int low;
        unsigned int high;

        if (length - offset < 2) return -1;
        logs = data[offset] & 0x7f;
        low = data[offset + 1] >> 4;
        high = data[offset + 1] & 0x0f;
        if (low <= high)
            off_count = high - low + 1;
        else if (low == 15 && (high == 0 || high == 1))
            off_count = 0;
        else
            return -1;
        if (logs == 127 && low == 15 && high == 0) ++logs;
        size = 2 + logs * 2 + off_count;
        if (size > length - offset) return -1;
        offset += size;
    }
    if (length - offset < 1) return -1;
    logs = (size_t)(data[offset] & 0x7f) + 1;
    size = 1 + logs * 2;
    if (size > length - offset) return -1;
    extras->count = logs;
    for (i = 0; i < logs; ++i) {
        const uint8_t *raw = data + offset + 1 + i * 2;
        struct rtpmidi_journal_note_extra *entry = &extras->logs[i];
        uint8_t number = raw[0] & 0x7f;
        uint8_t mask = (uint8_t)(0x80u >> (number % 8));
        uint8_t *seen = raw[1] & 0x80 ? seen_velocity : seen_count;
        if (seen[number / 8] & mask) return -1;
        seen[number / 8] |= mask;
        entry->number = number;
        entry->velocity = !!(raw[1] & 0x80);
        entry->value = raw[1] & 0x7f;
        entry->single_packet_safe = !!(raw[0] & 0x80);
    }
    return 1;
}

int rtpmidi_journal_decode_channel_state(
    const struct rtpmidi_journal_channel *channel,
    struct rtpmidi_journal_channel_state *state)
{
    const uint8_t *data;
    size_t length;
    size_t offset = 3;
    size_t size;
    if (!channel || !state || !channel->data || channel->length < 3)
        return -1;
    memset(state, 0, sizeof(*state));
    if (!(channel->chapters & (0x80 | 0x10)))
        return 0;
    data = channel->data;
    length = channel->length;
    if (channel->chapters & 0x80) {
        if (length - offset < 3) return -1;
        state->has_program = 1;
        state->program_single_packet_safe = !!(data[offset] & 0x80);
        state->program = data[offset] & 0x7f;
        state->has_bank = !!(data[offset + 1] & 0x80);
        state->bank_msb = data[offset + 1] & 0x7f;
        state->bank_lsb = data[offset + 2] & 0x7f;
        offset += 3;
    }
    if (channel->chapters & 0x40) {
        if (length - offset < 1)
            return -1;
        size = 1 + 2 * ((size_t)(data[offset] & 0x7f) + 1);
        if (size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x20) {
        if (length - offset < 2) return -1;
        size = ((size_t)(data[offset] & 3) << 8) | data[offset + 1];
        if (size < 2 || size > length - offset) return -1;
        offset += size;
    }
    if (channel->chapters & 0x10) {
        if (length - offset < 2) return -1;
        state->has_pitch = 1;
        state->pitch_single_packet_safe = !!(data[offset] & 0x80);
        state->pitch_lsb = data[offset] & 0x7f;
        state->pitch_msb = data[offset + 1] & 0x7f;
    }
    return 1;
}

int rtpmidi_journal_decode_controls(const struct rtpmidi_journal_channel *channel,
                                struct rtpmidi_journal_controls *controls)
{
    const uint8_t *data;
    size_t length;
    size_t offset = 3;
    size_t count;
    size_t i;
    uint8_t seen[128] = {0};
    uint8_t last_tool[128] = {0};
    uint8_t command[128] = {0};
    uint8_t command_mask[128] = {0};
    uint8_t expected_mask[128] = {0};
    uint8_t repeated[128] = {0};
    uint8_t tool;
    if (!channel || !controls || !channel->data || channel->length < 3)
        return -1;
    memset(controls, 0, sizeof(*controls));
    if (!(channel->chapters & 0x40))
        return 0;
    data = channel->data;
    length = channel->length;
    controls->enhanced = !!(data[0] & 0x04);
    if (channel->chapters & 0x80) {
        if (length - offset < 3) return -1;
        offset += 3;
    }
    if (length - offset < 1) return -1;
    count = (size_t)(data[offset] & 0x7f) + 1;
    if (1 + count * 2 > length - offset) return -1;
    controls->count = count;
    for (i = 0; i < count; ++i) {
        const uint8_t *log = data + offset + 1 + i * 2;
        struct rtpmidi_journal_control_log *entry = &controls->logs[i];
        uint8_t number = log[0] & 0x7f;
        entry->number = number;
        entry->alternate = !!(log[1] & 0x80);
        entry->count_tool = entry->alternate && !!(log[1] & 0x40);
        tool = !entry->alternate ? 2 : entry->count_tool ? 1 : 4;
        if (!controls->enhanced) {
            if (seen[number] & tool)
                return -1;
            seen[number] |= tool;
        } else {
            if (last_tool[number] && tool <= last_tool[number]) {
                if (!repeated[number]) {
                    if (!(command_mask[number] & 1)) return -1;
                    expected_mask[number] = command_mask[number] & 6;
                    repeated[number] = 1;
                } else if ((command_mask[number] & 6) !=
                           expected_mask[number]) {
                    return -1;
                }
                if (command[number] == 255) return -1;
                ++command[number];
                command_mask[number] = 0;
            }
            if (command_mask[number] & tool) return -1;
            command_mask[number] |= tool;
            last_tool[number] = tool;
            entry->command = command[number];
        }
        entry->value = log[1] & (entry->alternate ? 0x3f : 0x7f);
        entry->single_packet_safe = !!(log[0] & 0x80);
    }
    if (controls->enhanced) {
        for (i = 0; i < 128; ++i) {
            if (repeated[i] && ((command_mask[i] & 6) != expected_mask[i]))
                return -1;
        }
    }
    return 1;
}

int rtpmidi_journal_decode_parameters(const struct rtpmidi_journal_channel *channel,
                                 struct rtpmidi_journal_parameters *parameters)
{
    const uint8_t *data;
    const uint8_t *chapter;
    size_t length;
    size_t offset = 3;
    size_t chapter_length;
    size_t position;
    size_t i;
    uint8_t header;
    int compressed;

    if (!channel || !parameters || !channel->data || channel->length < 3)
        return -1;
    memset(parameters, 0, sizeof(*parameters));
    if (!(channel->chapters & 0x20)) return 0;
    data = channel->data;
    length = channel->length;
    if (channel->chapters & 0x80) {
        if (length - offset < 3) return -1;
        offset += 3;
    }
    if (channel->chapters & 0x40) {
        size_t size;
        if (length - offset < 1) return -1;
        size = 1 + 2 * ((size_t)(data[offset] & 0x7f) + 1);
        if (size > length - offset) return -1;
        offset += size;
    }
    if (length - offset < 2) return -1;
    chapter = data + offset;
    header = chapter[0];
    chapter_length = ((size_t)(header & 3) << 8) | chapter[1];
    if (chapter_length < 2 || chapter_length > length - offset ||
        ((header & 0x18) == 0x18))
        return -1;
    parameters->single_packet_safe = !!(header & 0x80);
    parameters->has_pending = !!(header & 0x40);
    parameters->transaction_open = !!(header & 0x20);
    position = 2;
    if (parameters->has_pending) {
        if (position >= chapter_length || parameters->transaction_open)
            return -1;
        parameters->pending_nrpn = !!(chapter[position] & 0x80);
        parameters->pending_msb = chapter[position] & 0x7f;
        ++position;
    }
    compressed = !!(header & 0x04) && !!(header & 0x18);
    while (position < chapter_length) {
        struct rtpmidi_journal_parameter_log *log;
        uint8_t toc;
        uint8_t lsb;
        uint8_t msb;
        int nrpn;
        if (parameters->count == 128) return -1;
        if (chapter_length - position < (size_t)(compressed ? 2 : 3))
            return -1;
        lsb = chapter[position] & 0x7f;
        if (compressed) {
            msb = 0;
            nrpn = !!(header & 0x08);
            toc = chapter[position + 1];
            position += 2;
        } else {
            nrpn = !!(chapter[position + 1] & 0x80);
            msb = chapter[position + 1] & 0x7f;
            toc = chapter[position + 2];
            position += 3;
        }
        if (lsb == 0x7f && msb == 0x7f) return -1;
        for (i = 0; i < parameters->count; ++i)
            if (parameters->logs[i].number == (uint16_t)((msb << 7) | lsb) &&
                parameters->logs[i].nrpn == nrpn)
                return -1;
        if ((toc & 0xf0) && !(toc & 0x02)) return -1;
        if ((toc & 0x08) && !(toc & 0x04)) return -1;
        log = &parameters->logs[parameters->count++];
        log->number = (uint16_t)((msb << 7) | lsb);
        log->nrpn = (uint8_t)nrpn;
        log->single_packet_safe = !!(chapter[position -
            (compressed ? 2 : 3)] & 0x80);
        log->value_tool = !!(toc & 0x02);
        log->count_tool = !!(toc & 0x04);
        if (toc & 0x80) {
            if (position >= chapter_length) return -1;
            log->has_entry_msb = 1;
            log->entry_msb = chapter[position++] & 0x7f;
        }
        if (toc & 0x40) {
            if (position >= chapter_length) return -1;
            log->has_entry_lsb = 1;
            log->entry_lsb = chapter[position++] & 0x7f;
        }
        if (toc & 0x20) {
            unsigned int magnitude;
            if (chapter_length - position < 2) return -1;
            magnitude = ((unsigned int)(chapter[position] & 0x3f) << 8) |
                        chapter[position + 1];
            log->has_adjust = 1;
            log->adjust = (int16_t)((chapter[position] & 0x80) ?
                                    -(int)magnitude : (int)magnitude);
            position += 2;
        }
        if (toc & 0x10) {
            if (chapter_length - position < 2) return -1;
            position += 2;
        }
        if (toc & 0x08) {
            if (position >= chapter_length) return -1;
            log->has_count = 1;
            log->count = chapter[position++] & 0x7f;
        }
    }
    return position == chapter_length ? 1 : -1;
}

void rtpmidi_reader_init(struct rtpmidi_reader *reader,
                        const struct rtpmidi_packet *packet)
{
    reader->packet = packet;
    reader->offset = 0;
    reader->timestamp = packet->timestamp;
    reader->running_status = 0;
    reader->undefined = 0;
}

int rtpmidi_reader_next(struct rtpmidi_reader *reader,
                       struct rtpmidi_event *event)
{
    const struct rtpmidi_packet *packet;
    const uint8_t *midi;
    size_t offset;
    uint32_t delta = 0;
    size_t i;
    size_t needed;
    uint8_t status;
    uint8_t byte;
    int has_status;

    if (!reader || !event || !reader->packet)
        return -1;
    packet = reader->packet;
    midi = packet->midi;
again:
    delta = 0;
    offset = reader->offset;
    if (offset >= packet->midi_length)
        return 0;

    if (offset || packet->z) {
        for (i = 0; i < 4; ++i) {
            if (offset >= packet->midi_length)
                return -1;
            byte = midi[offset++];
            delta = (delta << 7) | (byte & 0x7f);
            if (!(byte & 0x80))
                break;
        }
        if (i == 4)
            return -1;
        if (offset == packet->midi_length)
            return 0; /* A trailing delta time is legal. */
    }
    status = midi[offset];
    has_status = !!(status & 0x80);
    if (has_status) {
        /* Running status is for Voice and Mode messages; any other
           status stops it, System Real Time excepted (MIDI 1.0, pages 5
           and 30; RFC 6295 section 3.2 says the same of the list) */
        ++offset;
        if (status >= 0x80 && status <= 0xef)
            reader->running_status = status;
        else if (status < 0xf8)
            reader->running_status = 0;
    } else {
        status = reader->running_status;
        if (!status)
            return -1;
    }
    if (status == 0xf0 || status == 0xf7) {
        size_t start = offset - 1;
        while (offset < packet->midi_length && !(midi[offset] & 0x80))
            ++offset;
        if (offset >= packet->midi_length ||
            (midi[offset] != 0xf0 && midi[offset] != 0xf7 &&
             !(status == 0xf7 && midi[offset] == 0xf4)))
            return -1;
        event->length = 0;
        event->sysex = midi + start;
        event->sysex_length = offset + 1 - start;
        reader->timestamp += delta;
        event->timestamp = reader->timestamp;
        reader->offset = offset + 1;
        return 1;
    }
    if (status >= 0x80 && status <= 0xef)
        needed = ((status & 0xf0) == 0xc0 ||
                  (status & 0xf0) == 0xd0) ? 1 : 2;
    else if (status == 0xf1 || status == 0xf3)
        needed = 1;
    else if (status == 0xf2)
        needed = 2;
    else if (status == 0xf6)
        needed = 0;
    else if (status >= 0xf8 && status != 0xf9 && status != 0xfd)
        needed = 0;
    else {
        /* F4, F5, F9, FD. RFC 6295 section 3.2 forbids them in the list
           unless the session was set up to carry them, and says how they
           are coded then: F9 and FD as System Real Time, one octet; F4
           and F5 with their data octets and an F7 to close them. MIDI 1.0
           (page 6) has a receiver ignore an undefined status "without
           causing any problems to the system": it is stepped over, in
           either form, counted, and the commands around it are taken. */
        if (status == 0xf4 || status == 0xf5) {
            while (offset < packet->midi_length && !(midi[offset] & 0x80))
                ++offset;
            if (offset >= packet->midi_length || midi[offset] != 0xf7)
                return -1;
            ++offset;
        }
        reader->timestamp += delta;
        reader->offset = offset;
        ++reader->undefined;
        goto again;
    }
    if (packet->midi_length - offset < needed)
        return -1;
    event->bytes[0] = status;
    event->bytes[1] = 0;
    event->bytes[2] = 0;
    for (i = 0; i < needed; ++i) {
        if (midi[offset + i] & 0x80)
            return -1;
        event->bytes[i + 1] = midi[offset + i];
    }
    event->length = (uint8_t)(needed + 1);
    event->sysex = NULL;
    event->sysex_length = 0;
    reader->timestamp += delta;
    event->timestamp = reader->timestamp;
    reader->offset = offset + needed;
    return 1;
}

void rtpmidi_sysex_buffer(struct rtpmidi_sysex_assembler *assembler,
                          uint8_t *buffer, size_t capacity)
{
    memset(assembler, 0, sizeof(*assembler));
    assembler->bytes = buffer;
    assembler->capacity = buffer ? capacity : 0;
}

void rtpmidi_sysex_reset(struct rtpmidi_sysex_assembler *assembler)
{
    assembler->length = 0;
    assembler->timestamp = 0;
    assembler->last_sequence = 0;
    assembler->active = 0;
}

int rtpmidi_sysex_feed(struct rtpmidi_sysex_assembler *assembler,
                  const struct rtpmidi_event *event, uint16_t sequence,
                  const uint8_t **message, size_t *length)
{
    const uint8_t *segment;
    size_t size;
    uint8_t head;
    uint8_t tail;
    if (!assembler || !event || !message || !length ||
        !event->sysex || event->sysex_length < 2)
        return -1;
    segment = event->sysex;
    size = event->sysex_length;
    head = segment[0];
    tail = segment[size - 1];
    if (head == 0xf7 && tail == 0xf4) {
        rtpmidi_sysex_reset(assembler);
        return 0;
    }
    if (head == 0xf0) {
        if (size > assembler->capacity) {
            rtpmidi_sysex_reset(assembler);
            return -1;
        }
        memcpy(assembler->bytes, segment, size - (tail == 0xf0));
        assembler->length = size - (tail == 0xf0);
        assembler->timestamp = event->timestamp;
        assembler->active = tail == 0xf0;
    } else if (head == 0xf7 && assembler->active &&
               (sequence == assembler->last_sequence ||
                sequence == (uint16_t)(assembler->last_sequence + 1))) {
        if (assembler->length + size - 1 - (tail == 0xf0) > assembler->capacity) {
            rtpmidi_sysex_reset(assembler);
            return -1;
        }
        memcpy(assembler->bytes + assembler->length, segment + 1,
               size - 1 - (tail == 0xf0));
        assembler->length += size - 1 - (tail == 0xf0);
        assembler->active = tail == 0xf0;
    } else {
        rtpmidi_sysex_reset(assembler);
        return -1;
    }
    assembler->last_sequence = sequence;
    if (!assembler->active) {
        *message = assembler->bytes;
        *length = assembler->length;
        return 1;
    }
    return 0;
}
