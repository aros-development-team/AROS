/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "ump/ump.h"

#include <string.h>

/* Section 2.1.4, table 4. */
unsigned ump_words(uint32_t first_word)
{
    static const unsigned char words[16] = {
        1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4
    };
    return words[first_word >> 28];
}

/* Appendix D.1.4. */
uint32_t ump_scale_down(uint32_t value, unsigned from_bits, unsigned to_bits)
{
    return value >> (from_bits - to_bits);
}

/* M2-115-U section 4.4.1. */
uint32_t ump_scale_down_rounding(uint32_t value, unsigned from_bits,
                                 unsigned to_bits)
{
    unsigned scale_bits = from_bits - to_bits;
    uint64_t shifted;
    uint64_t maximum = ((uint64_t)1 << to_bits) - 1;

    if (!scale_bits)
        return value;
    shifted = ((uint64_t)value + ((uint64_t)1 << (scale_bits - 1))) >>
              scale_bits;
    return (uint32_t)(shifted > maximum ? maximum : shifted);
}

/* Appendix D.1.3 and M2-115-U section 3.3.1, the min-center-max method. */
uint32_t ump_scale_up(uint32_t value, unsigned from_bits, unsigned to_bits)
{
    unsigned scale_bits = to_bits - from_bits;
    uint32_t shifted = value << scale_bits;
    uint32_t center = 1u << (from_bits - 1);
    unsigned repeat_bits;
    uint32_t repeat;

    /* Values up to the center scale by shifting, so that the center maps
       to the center; above it the lower bits are filled by repeating the
       source bits below the top one, so that the maximum maps to the
       maximum. */
    if (value <= center)
        return shifted;
    repeat_bits = from_bits - 1;
    repeat = value & ((1u << repeat_bits) - 1);
    if (scale_bits > repeat_bits)
        repeat <<= scale_bits - repeat_bits;
    else
        repeat >>= repeat_bits - scale_bits;
    while (repeat) {
        shifted |= repeat;
        repeat >>= repeat_bits;
    }
    return shifted;
}

/* MIDI 1.0 to UMP */

void ump_from_midi1_init(struct ump_from_midi1 *state, uint8_t group)
{
    memset(state, 0, sizeof(*state));
    state->group = group & 0x0f;
}

/* The system status bytes a UMP may carry (section 7.6, table 17). F0 and
 * F7 belong to SysEx, which has its own packets; F4, F5, F9 and FD are
 * reserved. */
static int system_status_defined(uint8_t status)
{
    return status == 0xf1 || status == 0xf2 || status == 0xf3 ||
           status == 0xf6 || status == 0xf8 || status == 0xfa ||
           status == 0xfb || status == 0xfc || status == 0xfe ||
           status == 0xff;
}

/* Tables II, V and VI of the MIDI 1.0 Detailed Specification. */
static unsigned midi1_data_bytes(uint8_t status)
{
    switch (status & 0xf0) {
    case 0xc0:
    case 0xd0:
        return 1;
    case 0xf0:
        switch (status) {
        case 0xf1:
        case 0xf3:
            return 1;
        case 0xf2:
            return 2;
        default:
            return 0;
        }
    default:
        return 2;
    }
}

static void emit_sysex7(struct ump_from_midi1 *state, unsigned kind,
                        ump_sink sink, void *context)
{
    uint32_t words[2];
    const uint8_t *b = state->sysex;

    words[0] = ((uint32_t)UMP_DATA64 << 28) | ((uint32_t)state->group << 24) |
               ((uint32_t)kind << 20) | ((uint32_t)state->sysex_count << 16) |
               ((uint32_t)b[0] << 8) | b[1];
    words[1] = ((uint32_t)b[2] << 24) | ((uint32_t)b[3] << 16) |
               ((uint32_t)b[4] << 8) | b[5];
    sink(context, words, 2);
    memset(state->sysex, 0, sizeof(state->sysex));
    state->sysex_count = 0;
}

/* SysEx7 packet kinds */
enum { SYSEX7_COMPLETE, SYSEX7_START, SYSEX7_CONTINUE, SYSEX7_END };

static void emit_short(struct ump_from_midi1 *state, uint8_t status,
                       uint8_t data1, uint8_t data2, ump_sink sink, void *context)
{
    uint32_t word;
    unsigned type = status >= 0xf0 ? UMP_SYSTEM : UMP_MIDI1_VOICE;

    word = ((uint32_t)type << 28) | ((uint32_t)state->group << 24) |
           ((uint32_t)status << 16) | ((uint32_t)data1 << 8) | data2;
    sink(context, &word, 1);
}

/* The rules of a MIDI 1.0 byte stream are those of the Complete MIDI 1.0
 * Detailed Specification 96.1 (version 4.2.1); pages are its own. */
void ump_from_midi1_byte(struct ump_from_midi1 *state, uint8_t byte,
                            ump_sink sink, void *context)
{
    if (byte >= 0xf8) {
        /* System Real Time may come anywhere, between the bytes of
         * another message and inside SysEx, and is no new status
         * (page 30). F9 and FD are undefined and left out (page 6). */
        if (system_status_defined(byte))
            emit_short(state, byte, 0, 0, sink, context);
        return;
    }
    if (state->in_sysex) {
        if (byte & 0x80) {
            /* EOX ends a SysEx, and so does any other status but Real
             * Time, which then starts its own message (page 29); a new
             * F0 is one of them. */
            emit_sysex7(state, state->sysex_started ? SYSEX7_END : SYSEX7_COMPLETE,
                        sink, context);
            state->in_sysex = 0;
            if (byte == 0xf7)
                return;
        } else {
            if (state->sysex_count == 6) {
                emit_sysex7(state, state->sysex_started ? SYSEX7_CONTINUE : SYSEX7_START,
                            sink, context);
                state->sysex_started = 1;
            }
            state->sysex[state->sysex_count++] = byte;
            return;
        }
    }
    if (byte == 0xf0) {
        state->in_sysex = 1;
        state->sysex_started = 0;
        state->sysex_count = 0;
        state->status = 0;
        return;
    }
    if (byte & 0x80) {
        /* A new status is adopted even if the message before it was not
         * complete (page 5). Running status is for Voice and Mode
         * messages only, and any other status stops it: a System Common
         * message, an EOX with no SysEx to end, the undefined F4 and F5. */
        state->have = 0;
        state->status = 0;
        if (byte == 0xf7)
            return;
        state->need = (uint8_t)midi1_data_bytes(byte);
        if (state->need == 0) {
            if (system_status_defined(byte))
                emit_short(state, byte, 0, 0, sink, context);
            return;
        }
        state->status = byte;
        return;
    }
    /* data with no status before it is ignored (page 6) */
    if (!state->status)
        return;
    state->data[state->have++] = byte;
    if (state->have < state->need)
        return;
    emit_short(state, state->status, state->data[0],
               state->need == 2 ? state->data[1] : 0, sink, context);
    state->have = 0;
    if (state->status >= 0xf0)
        state->status = 0;
}

/* UMP to MIDI 1.0 */

void ump_to_midi1_init(struct ump_to_midi1 *state, uint8_t group)
{
    memset(state, 0, sizeof(*state));
    state->group = group & 0x0f;
}

int ump_jr_after_clock(const struct ump_to_midi1 *state, int32_t *ticks)
{
    if (!state || !ticks || !state->jr.have_clock ||
        !state->jr.have_timestamp)
        return 0;
    /* both wrap at sixteen bits, and a clock comes at least every 250 ms,
       far inside half a turn: the difference as a signed sixteen bits */
    *ticks = (int16_t)(uint16_t)(state->jr.timestamp - state->jr.clock);
    return 1;
}

void ump_jr_clock_init(struct ump_jr_clock *clock)
{
    memset(clock, 0, sizeof(*clock));
}

void ump_jr_clock_seen(struct ump_jr_clock *clock, uint16_t sender_clock,
                       uint64_t local_us)
{
    int64_t offset;
    unsigned i;

    if (clock->known &&
        local_us - clock->last_local_us > UMP_JR_CLOCK_GAP_US) {
        clock->known = 0;           /* how often it wrapped is not known */
        clock->count = clock->next = 0;
    }
    if (clock->known)
        clock->sender_ticks += (uint16_t)(sender_clock - clock->last);
    else
        clock->sender_ticks = sender_clock;
    clock->known = 1;
    clock->last = sender_clock;
    clock->last_local_us = local_us;
    offset = (int64_t)local_us -
             (int64_t)(clock->sender_ticks * UMP_JR_TICK_US);
    clock->kept[clock->next] = offset;
    clock->next = (clock->next + 1) % UMP_JR_CLOCKS_KEPT;
    if (clock->count < UMP_JR_CLOCKS_KEPT)
        ++clock->count;
    for (i = 0; i < clock->count; ++i)
        if (clock->kept[i] < offset)
            offset = clock->kept[i];
    clock->offset_us = offset;
}

int ump_jr_clock_due(const struct ump_jr_clock *clock, uint16_t timestamp,
                     uint64_t *local_us)
{
    int64_t ticks;

    if (!clock || !local_us || !clock->known)
        return 0;
    /* near the last clock, before it or after: a signed sixteen bits */
    ticks = (int64_t)clock->sender_ticks +
            (int16_t)(uint16_t)(timestamp - clock->last);
    *local_us = (uint64_t)(ticks * UMP_JR_TICK_US + clock->offset_us);
    return 1;
}

void ump_to_midi1_buffer(struct ump_to_midi1 *state, uint8_t *buffer,
                         size_t capacity)
{
    state->sysex = buffer;
    state->sysex_capacity = buffer ? capacity : 0;
    state->sysex_length = 0;
    state->in_sysex = 0;
    state->sysex_overflow = 0;
}

static void put3(ump_midi1_sink sink, void *context, uint8_t a, uint8_t b, uint8_t c)
{
    uint8_t bytes[3] = { a, b, c };
    sink(context, bytes, 3);
}

static void put_cc(ump_midi1_sink sink, void *context, uint8_t channel,
                   uint8_t controller, uint8_t value)
{
    put3(sink, context, (uint8_t)(0xb0 | channel), controller, value);
}

/* Appendix D.2: the default translation of MIDI 2.0 Channel Voice (section
 * 7.4) to MIDI 1.0. Returns 0 for a message that has no MIDI 1.0 form. */
static int midi2_voice_down(const uint32_t *words, ump_midi1_sink sink, void *context)
{
    unsigned opcode = (words[0] >> 20) & 0x0f;
    uint8_t channel = (uint8_t)((words[0] >> 16) & 0x0f);
    uint8_t byte2 = (uint8_t)(words[0] >> 8);
    uint8_t byte3 = (uint8_t)words[0];
    uint32_t data = words[1];
    uint8_t bytes[2];
    uint32_t value14;

    switch (opcode) {
    case 0x8: /* Note Off */
        put3(sink, context, (uint8_t)(0x80 | channel), byte2 & 0x7f,
             (uint8_t)ump_scale_down(data >> 16, 16, 7));
        break;
    case 0x9: { /* Note On: 0 would mean Note Off in MIDI 1.0 (D.2.1) */
        uint8_t velocity = (uint8_t)ump_scale_down(data >> 16, 16, 7);
        put3(sink, context, (uint8_t)(0x90 | channel), byte2 & 0x7f,
             velocity ? velocity : 1);
        break;
    }
    case 0xa: /* Poly Pressure */
        put3(sink, context, (uint8_t)(0xa0 | channel), byte2 & 0x7f,
             (uint8_t)ump_scale_down(data, 32, 7));
        break;
    case 0xb: /* Control Change */
        put_cc(sink, context, channel, byte2 & 0x7f,
               (uint8_t)ump_scale_down(data, 32, 7));
        break;
    case 0x2: /* Registered Controller: RPN */
    case 0x3: /* Assignable Controller: NRPN */
        /* Four messages, as D.2.3 and its figure 97 give them: the bank
           and the index select, the top fourteen bits are the data. The
           parameter is left selected; the specification asks for no
           more. */
        /* Which fourteen bits: M2-115-U section 4.1 has the registered
           controllers with an index below 32 scaled as numbers, which
           on the way down means rounded. Five of them the UMP
           specification gives a layout of their own (7.4.7.1), with the
           low bits to be ignored. Everything else is a share of a range
           and loses its low bits. */
        if (opcode == 0x2 && (byte3 & 0x7f) < 32 &&
            !((byte2 & 0x7f) == 0 &&
              ((byte3 & 0x7f) == 0 || (byte3 & 0x7f) == 2 ||
               (byte3 & 0x7f) == 3 || (byte3 & 0x7f) == 4 ||
               (byte3 & 0x7f) == 6)))
            value14 = ump_scale_down_rounding(data, 32, 14);
        else
            value14 = ump_scale_down(data, 32, 14);
        put_cc(sink, context, channel, opcode == 0x2 ? 101 : 99, byte2 & 0x7f);
        put_cc(sink, context, channel, opcode == 0x2 ? 100 : 98, byte3 & 0x7f);
        put_cc(sink, context, channel, 6, (uint8_t)(value14 >> 7));
        put_cc(sink, context, channel, 38, (uint8_t)(value14 & 0x7f));
        break;
    case 0xc: /* Program Change, with the bank first when Bank Valid is
                 set (D.2.4; the fields are those of figure 64) */
        if (byte3 & 0x01) {
            put_cc(sink, context, channel, 0, (uint8_t)((data >> 8) & 0x7f));
            put_cc(sink, context, channel, 32, (uint8_t)(data & 0x7f));
        }
        bytes[0] = (uint8_t)(0xc0 | channel);
        bytes[1] = (uint8_t)((data >> 24) & 0x7f);
        sink(context, bytes, 2);
        break;
    case 0xd: /* Channel Pressure */
        bytes[0] = (uint8_t)(0xd0 | channel);
        bytes[1] = (uint8_t)ump_scale_down(data, 32, 7);
        sink(context, bytes, 2);
        break;
    case 0xe: /* Pitch Bend */
        value14 = ump_scale_down(data, 32, 14);
        put3(sink, context, (uint8_t)(0xe0 | channel), (uint8_t)(value14 & 0x7f),
             (uint8_t)(value14 >> 7));
        break;
    default:
        /* Per-note and relative controllers, per-note pitch bend and
           management have no MIDI 1.0 form (D.2.8). */
        return 0;
    }
    return 1;
}

/* A SysEx message in progress is cut off by a message that may not come
 * inside one (section 7.7.1). */
static void sysex_cut(struct ump_to_midi1 *state)
{
    if (state->in_sysex) {
        state->in_sysex = 0;
        state->sysex_length = 0;
        state->sysex_overflow = 0;
        ++state->dropped.sysex_broken;
    }
}

static void sysex_byte(struct ump_to_midi1 *state, uint8_t byte)
{
    /* one place is kept for the F7 */
    if (state->sysex_overflow || !state->sysex ||
        state->sysex_length + 1 >= state->sysex_capacity)
        state->sysex_overflow = 1;
    else
        state->sysex[state->sysex_length++] = byte;
}

/* Section 7.7. */
static void sysex7(struct ump_to_midi1 *state, const uint32_t *words,
                   ump_midi1_sink sink, void *context)
{
    unsigned kind = (words[0] >> 20) & 0x0f;
    unsigned n = (words[0] >> 16) & 0x0f;
    uint8_t data[6];
    unsigned i;

    if (n > 6 || kind > SYSEX7_END) {
        sysex_cut(state);
        ++state->dropped.reserved_status;
        return;
    }
    data[0] = (uint8_t)(words[0] >> 8);
    data[1] = (uint8_t)words[0];
    data[2] = (uint8_t)(words[1] >> 24);
    data[3] = (uint8_t)(words[1] >> 16);
    data[4] = (uint8_t)(words[1] >> 8);
    data[5] = (uint8_t)words[1];
    if (kind == SYSEX7_COMPLETE || kind == SYSEX7_START) {
        sysex_cut(state);           /* a start inside a message ends it */
        state->in_sysex = 1;
        state->sysex_length = 0;
        state->sysex_overflow = 0;
        sysex_byte(state, 0xf0);
    } else if (!state->in_sysex) {
        ++state->dropped.sysex_broken;  /* a piece without its start */
        return;
    }
    for (i = 0; i < n; ++i)
        sysex_byte(state, data[i] & 0x7f);
    if (kind == SYSEX7_COMPLETE || kind == SYSEX7_END) {
        state->in_sysex = 0;
        if (state->sysex_overflow) {
            ++state->dropped.sysex_too_long;
        } else {
            state->sysex[state->sysex_length++] = 0xf7;
            sink(context, state->sysex, state->sysex_length);
        }
        state->sysex_length = 0;
        state->sysex_overflow = 0;
    }
}

int ump_to_midi1(struct ump_to_midi1 *state, const uint32_t *words,
                    unsigned count, ump_midi1_sink sink, void *context)
{
    unsigned type = words[0] >> 28;
    unsigned needed = ump_words(words[0]);
    uint8_t status;

    if (count < needed)
        return -1;
    /* Utility and stream messages have no group and may come anywhere
       (7.7.1). Neither has a MIDI 1.0 form (D.2.9). Of the utility
       messages, the two of jitter reduction leave their time here
       (7.2.2.1, 7.2.2.2; figures 29 and 31: the status in bits 23 to
       20, the time in the low sixteen), and NOOP is nothing (7.2.1). */
    if (type == UMP_UTILITY) {
        switch ((words[0] >> 20) & 0x0f) {
        case 0x0:
            break;
        case 0x1:
            state->jr.clock = (uint16_t)words[0];
            state->jr.have_clock = 1;
            ++state->jr.clocks;
            break;
        case 0x2:
            state->jr.timestamp = (uint16_t)words[0];
            state->jr.have_timestamp = 1;
            break;
        default:
            ++state->dropped.not_translated;
            break;
        }
        return 0;
    }
    if (type == UMP_STREAM) {
        ++state->dropped.not_translated;
        return 0;
    }
    state->groups_seen |= (uint16_t)(1u << ((words[0] >> 24) & 0x0f));
    if (((words[0] >> 24) & 0x0f) != state->group) {
        ++state->dropped.other_group;
        return 0;
    }
    switch (type) {
    case UMP_SYSTEM:
        status = (uint8_t)(words[0] >> 16);
        if (!system_status_defined(status)) {
            sysex_cut(state);
            ++state->dropped.reserved_status;
            break;
        }
        if (status < 0xf8)
            sysex_cut(state);   /* System Real Time alone may interleave */
        if (status == 0xf1 || status == 0xf3) {
            uint8_t bytes[2] = { status, (uint8_t)((words[0] >> 8) & 0x7f) };
            sink(context, bytes, 2);
        } else if (status == 0xf2) {
            put3(sink, context, status, (uint8_t)((words[0] >> 8) & 0x7f),
                 (uint8_t)(words[0] & 0x7f));
        } else {
            sink(context, &status, 1);
        }
        break;
    case UMP_MIDI1_VOICE: {
        uint8_t bytes[3];
        sysex_cut(state);
        status = (uint8_t)(words[0] >> 16);
        if (status < 0x80 || status >= 0xf0) {
            ++state->dropped.reserved_status;
            break;
        }
        bytes[0] = status;
        bytes[1] = (uint8_t)((words[0] >> 8) & 0x7f);
        bytes[2] = (uint8_t)(words[0] & 0x7f);
        sink(context, bytes, midi1_data_bytes(status) + 1);
        break;
    }
    case UMP_DATA64:
        sysex7(state, words, sink, context);
        break;
    case UMP_MIDI2_VOICE:
        sysex_cut(state);
        if (!midi2_voice_down(words, sink, context))
            ++state->dropped.no_midi1_form;
        break;
    default:
        /* SysEx8, mixed data, flex data, the reserved types */
        sysex_cut(state);
        ++state->dropped.not_translated;
        break;
    }
    return 0;
}

/* UMP Stream messages */

unsigned ump_stream_status(const uint32_t *words)
{
    return (words[0] >> 16) & 0x3ff;
}

static uint32_t stream_word0(unsigned form, unsigned status)
{
    return ((uint32_t)UMP_STREAM << 28) | ((uint32_t)form << 26) |
           ((uint32_t)status << 16);
}

/* Section 7.1.2, figure 13: the static bit and the count of function
 * blocks in the second word's first byte, M2 and M1 at bits 9 and 8. */
void ump_endpoint_info(uint32_t *words, unsigned protocols, unsigned jr)
{
    /* UMP version 1.1; one function block, static. */
    words[0] = stream_word0(0, UMP_ENDPOINT_INFO) | 0x0101;
    words[1] = 0x81000000u |
               ((protocols & UMP_PROTOCOL_MIDI2) ? 0x200u : 0) |
               ((protocols & UMP_PROTOCOL_MIDI1) ? 0x100u : 0) | (jr & 0x3);
    words[2] = 0;
    words[3] = 0;
}

/* Section 7.1.6.3: the protocol in the first word's third byte, as in
 * figure 18 for the request. */
void ump_stream_config(uint32_t *words, unsigned protocol, unsigned jr)
{
    words[0] = stream_word0(0, UMP_STREAM_CONFIG_NOTIFY) |
               ((uint32_t)(protocol & 0xff) << 8) | (jr & 0x3);
    words[1] = 0;
    words[2] = 0;
    words[3] = 0;
}

/* Section 7.1.8, figure 22: active, block 0; user interface hint and
 * direction both ways; a MIDI 1.0 port whose bandwidth is not to be
 * restricted; one group; no MIDI-CI and no SysEx8. */
void ump_function_block_info(uint32_t *words, unsigned group)
{
    words[0] = stream_word0(0, UMP_FUNCTION_BLOCK_INFO) |
               0x8000u |               /* active, block 0 */
               (0x3u << 4) |           /* a sender and a receiver */
               (0x1u << 2) |           /* MIDI 1.0, not restricted */
               0x3u;                   /* bidirectional */
    words[1] = ((uint32_t)(group & 0x0f) << 24) | (1u << 16);
    words[2] = 0;
    words[3] = 0;
}

/* Section 7.1.9, figure 23: the block's number, then thirteen bytes. */
unsigned ump_function_block_name(uint32_t *words, size_t capacity_words,
                                 const char *text, size_t length)
{
    unsigned packets = (unsigned)(length ? (length + 12) / 13 : 1);
    unsigned packet, i, written = 0;

    if (length > UMP_FUNCTION_BLOCK_NAME_MAX ||
        (size_t)packets * 4 > capacity_words)
        return 0;
    for (packet = 0; packet < packets; ++packet) {
        uint8_t bytes[13];
        unsigned form;
        size_t offset = (size_t)packet * 13;

        memset(bytes, 0, sizeof(bytes));
        for (i = 0; i < 13 && offset + i < length; ++i)
            bytes[i] = (uint8_t)text[offset + i];
        form = packets == 1 ? 0 : packet == 0 ? 1 : packet == packets - 1 ? 3 : 2;
        words[written++] = stream_word0(form, UMP_FUNCTION_BLOCK_NAME) |
                           bytes[0];        /* block 0 in the byte before */
        for (i = 1; i < 13; i += 4)
            words[written++] = ((uint32_t)bytes[i] << 24) |
                               ((uint32_t)bytes[i + 1] << 16) |
                               ((uint32_t)bytes[i + 2] << 8) | bytes[i + 3];
    }
    return written;
}

/* Sections 7.1.4 and 7.1.5; the layout is that of figure 16. */
unsigned ump_stream_text(uint32_t *words, size_t capacity_words,
                            unsigned status, const char *text, size_t length)
{
    unsigned packets = (unsigned)(length ? (length + 13) / 14 : 1);
    unsigned packet, i, written = 0;
    if (status == UMP_ENDPOINT_NAME && length > UMP_ENDPOINT_NAME_MAX)
        return 0;
    if (status == UMP_PRODUCT_INSTANCE_ID) {
        size_t n;
        if (length > UMP_PRODUCT_INSTANCE_ID_MAX)
            return 0;
        for (n = 0; n < length; ++n)
            if (text[n] < 32 || text[n] > 126)
                return 0;
    }

    if ((size_t)packets * 4 > capacity_words)
        return 0;
    for (packet = 0; packet < packets; ++packet) {
        uint8_t bytes[14];
        unsigned form;
        size_t offset = (size_t)packet * 14;

        memset(bytes, 0, sizeof(bytes));
        for (i = 0; i < 14 && offset + i < length; ++i)
            bytes[i] = (uint8_t)text[offset + i];
        form = packets == 1 ? 0 : packet == 0 ? 1 : packet == packets - 1 ? 3 : 2;
        words[written++] = stream_word0(form, status) | ((uint32_t)bytes[0] << 8) | bytes[1];
        for (i = 2; i < 14; i += 4)
            words[written++] = ((uint32_t)bytes[i] << 24) | ((uint32_t)bytes[i + 1] << 16) |
                               ((uint32_t)bytes[i + 2] << 8) | bytes[i + 3];
    }
    return written;
}
