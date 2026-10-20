/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef UMP_H
#define UMP_H

#include <stddef.h>
#include <stdint.h>

/* MIDI 2.0 Universal MIDI Packet, after the MIDI Association's "Universal
 * MIDI Packet (UMP) Format and MIDI 2.0 Protocol", M2-104-UM version
 * 1.1.2. The section numbers in this file and in ump.c are that
 * document's; the code was read against it on 2026-10-12.
 * A UMP is one to four 32-bit words in host order; the first word's top
 * nibble is the message type. CAMD carries MIDI 1.0 byte messages, so
 * these helpers translate between the two. No OS calls occur here. */

enum ump_type {
    UMP_UTILITY = 0x0,
    UMP_SYSTEM = 0x1,          /* System Common and Real Time */
    UMP_MIDI1_VOICE = 0x2,     /* MIDI 1.0 Channel Voice */
    UMP_DATA64 = 0x3,          /* SysEx7 */
    UMP_MIDI2_VOICE = 0x4,     /* MIDI 2.0 Channel Voice */
    UMP_DATA128 = 0x5,         /* SysEx8, Mixed Data Set */
    UMP_FLEX = 0xd,
    UMP_STREAM = 0xf
};

/* Words in a UMP of the message type in the first word's top nibble
 * (section 2.1.4, table 4). The reserved types have sizes too. */
unsigned ump_words(uint32_t first_word);

/* Scaling between resolutions, e.g. 7-bit velocity to 16-bit and back:
 * up by the min-center-max method (appendix D.1.3), down by dropping the
 * low bits (D.1.4). "MIDI 2.0 Bit Scaling and Resolution", M2-115-U
 * version 1.0.2, which that appendix defers to, gives the same two methods
 * in its section 3, and the code was read against it too. */
uint32_t ump_scale_up(uint32_t value, unsigned from_bits, unsigned to_bits);
uint32_t ump_scale_down(uint32_t value, unsigned from_bits, unsigned to_bits);
/* Down by rounding to the nearest and keeping within the range: the
 * zero-extension method of M2-115-U section 4.4, for values that are
 * numbers in a unit and not a share of a range. */
uint32_t ump_scale_down_rounding(uint32_t value, unsigned from_bits,
                                 unsigned to_bits);

/* MIDI 1.0 byte stream to UMP. Feed bytes as they arrive (running status
 * and real time bytes inside SysEx are handled); each complete message
 * is emitted as MT 1 (section 7.6), MT 2 (7.3) or a series of MT 3 SysEx7
 * packets (7.7). The status bytes F4, F5, F9 and FD, which table 17 marks
 * reserved, are not passed on. */
struct ump_from_midi1 {
    uint8_t group;
    uint8_t status;               /* running status, 0 for none */
    uint8_t data[2];
    uint8_t have;
    uint8_t need;
    uint8_t in_sysex;
    uint8_t sysex[6];
    uint8_t sysex_count;
    uint8_t sysex_started;        /* a Start packet has been emitted */
};

typedef void (*ump_sink)(void *context, const uint32_t *words, unsigned count);

void ump_from_midi1_init(struct ump_from_midi1 *state, uint8_t group);
void ump_from_midi1_byte(struct ump_from_midi1 *state, uint8_t byte,
                            ump_sink sink, void *context);

/* What a translation to MIDI 1.0 left out, by reason. */
struct ump_dropped {
    unsigned long other_group;      /* not this translator's group */
    unsigned long no_midi1_form;    /* per-note and relative controllers,
                                       per-note pitch bend and management
                                       (appendix D.2.8) */
    unsigned long not_translated;   /* delta clockstamps, SysEx8, mixed
                                       data, flex data, stream and
                                       reserved types */
    unsigned long reserved_status;  /* a reserved status (table 17) or a
                                       malformed packet */
    unsigned long sysex_broken;     /* ended by another message, or a
                                       piece without its start (7.7.1) */
    unsigned long sysex_too_long;   /* longer than the buffer */
};

/* UMP to MIDI 1.0 bytes, by the default translation of appendix D.2. MT 1
 * and MT 2 map directly; MIDI 2.0 Channel Voice is translated down (bank
 * and RPN/NRPN become their Control Change sequences); messages with no
 * MIDI 1.0 equivalent are dropped and counted. Only UMPs of one group are
 * taken.
 *
 * A SysEx message reaches the sink whole, F0 to F7, once its last packet
 * has come (D.2.7), and needs a buffer to be gathered in: without one
 * every SysEx message is counted as too long. A message of the same group
 * that is neither a further piece nor System Real Time ends a SysEx
 * message in progress (7.7.1), which is then dropped. */
/* The time a UMP stream carries (section 7.2.2), in the sender's ticks of
 * 1/31250 second, sixteen bits that wrap every 2.09712 seconds. A JR
 * Clock says what time it is at the sender as it leaves; a JR Timestamp
 * says when the messages after it are meant, until the next one. The
 * translator keeps the last of each; whoever calls it reads them in its
 * sink, and notes when a clock came by watching clocks go up. */
#define UMP_JR_TICKS_PER_SECOND 31250
struct ump_jr {
    uint8_t have_clock;
    uint8_t have_timestamp;
    uint16_t clock;
    uint16_t timestamp;
    unsigned long clocks;           /* JR Clock messages so far */
};

struct ump_to_midi1 {
    uint8_t group;
    uint8_t in_sysex;
    uint8_t sysex_overflow;
    uint8_t *sysex;
    size_t sysex_capacity;
    size_t sysex_length;
    struct ump_dropped dropped;
    struct ump_jr jr;
    uint16_t groups_seen;       /* a bit to a group that messages came for,
                                   this translator's and the others': how
                                   a caller learns which groups a peer
                                   uses and may want a translator for */
};

/* How long after the sender's last clock the messages now passing are
 * meant, in ticks; negative when before it. Returns 0 and leaves *ticks
 * alone until both a JR Clock and a JR Timestamp have come: such
 * messages are for at once (7.2.2.3). */
int ump_jr_after_clock(const struct ump_to_midi1 *state, int32_t *ticks);

/* The sender's clock against ours, from its JR Clock messages (7.2.2.1).
 * Each says what time it was at the sender as it left; it arrives a little
 * later, and not always by the same little. Of the last eight, the one
 * that arrived soonest after its own time is the measure, as the text
 * lets a receiver choose ("smoothing or averaging"). Our time is in
 * microseconds; a tick is 32 of them. After a second without a clock the
 * count starts anew: the sender's sixteen bits wrap every 2.09712 seconds
 * and it has to send one every 250 ms. */
#define UMP_JR_CLOCKS_KEPT 8
#define UMP_JR_TICK_US 32
#define UMP_JR_CLOCK_GAP_US 1000000u
struct ump_jr_clock {
    uint8_t known;
    uint16_t last;                  /* the last clock, as sent */
    uint64_t sender_ticks;          /* the same, counted on past its wraps */
    uint64_t last_local_us;
    int64_t kept[UMP_JR_CLOCKS_KEPT];   /* ours less the sender's, in us */
    unsigned count;
    unsigned next;
    int64_t offset_us;
};

void ump_jr_clock_init(struct ump_jr_clock *clock);
/* A JR Clock that arrived at local_us. */
void ump_jr_clock_seen(struct ump_jr_clock *clock, uint16_t sender_clock,
                       uint64_t local_us);
/* When, by our time, a message with this JR Timestamp is meant. Returns 0
 * until a clock has been seen. */
int ump_jr_clock_due(const struct ump_jr_clock *clock, uint16_t timestamp,
                     uint64_t *local_us);

typedef void (*ump_midi1_sink)(void *context, const uint8_t *bytes, size_t length);

void ump_to_midi1_init(struct ump_to_midi1 *state, uint8_t group);
/* Where SysEx messages are gathered. capacity counts F0 and F7. */
void ump_to_midi1_buffer(struct ump_to_midi1 *state, uint8_t *buffer,
                         size_t capacity);
/* Returns -1 when words holds fewer words than the UMP needs. */
int ump_to_midi1(struct ump_to_midi1 *state, const uint32_t *words,
                    unsigned count, ump_midi1_sink sink, void *context);

/* UMP Stream messages (MT 0xF, section 7.1) an endpoint answers discovery
 * with. */
enum ump_stream_status {
    UMP_ENDPOINT_DISCOVERY = 0x000,
    UMP_ENDPOINT_INFO = 0x001,
    UMP_DEVICE_IDENTITY = 0x002,
    UMP_ENDPOINT_NAME = 0x003,
    UMP_PRODUCT_INSTANCE_ID = 0x004,
    UMP_STREAM_CONFIG_REQUEST = 0x005,
    UMP_STREAM_CONFIG_NOTIFY = 0x006,
    UMP_FUNCTION_BLOCK_DISCOVERY = 0x010,
    UMP_FUNCTION_BLOCK_INFO = 0x011,
    UMP_FUNCTION_BLOCK_NAME = 0x012
};

enum {
    UMP_FILTER_ENDPOINT_INFO = 0x01,
    UMP_FILTER_DEVICE_IDENTITY = 0x02,
    UMP_FILTER_ENDPOINT_NAME = 0x04,
    UMP_FILTER_PRODUCT_INSTANCE_ID = 0x08,
    UMP_FILTER_STREAM_CONFIG = 0x10
};

enum { UMP_PROTOCOL_MIDI1 = 0x01, UMP_PROTOCOL_MIDI2 = 0x02 };
/* Jitter reduction timestamps, as Endpoint Info and Stream Configuration
 * carry them in their two lowest bits (figures 13, 18 and 19). */
enum { UMP_JR_TRANSMIT = 0x01, UMP_JR_RECEIVE = 0x02 };

unsigned ump_stream_status(const uint32_t *words);

/* The one function block an endpoint here declares (sections 7.1.7 to
 * 7.1.9): a MIDI 1.0 port in both directions, of one group. A host that
 * shows its user function blocks has this one to show. */
#define UMP_FUNCTION_BLOCK_NAME_MAX 91
enum { UMP_FUNCTION_BLOCK_FILTER_INFO = 0x01,
       UMP_FUNCTION_BLOCK_FILTER_NAME = 0x02 };
/* Function Block Info Notification for block 0 (figure 22). Writes 4
 * words. */
void ump_function_block_info(uint32_t *words, unsigned group);
/* Function Block Name Notification for block 0 (figure 23): thirteen
 * bytes of the name to a packet. Returns the words written, 0 when the
 * name is longer than 91 bytes or does not fit. */
unsigned ump_function_block_name(uint32_t *words, size_t capacity_words,
                                 const char *text, size_t length);
/* Endpoint Info Notification (7.1.2): UMP 1.1, no function blocks, which
 * the specification allows and which leaves Function Block Discovery
 * without an answer to give (7.1.8), and the protocols given
 * (UMP_PROTOCOL_* bits). Writes 4 words. */
void ump_endpoint_info(uint32_t *words, unsigned protocols, unsigned jr);
/* Stream Configuration Notification (7.1.6.3) for protocol, with no
 * jitter-reduction timestamps either way. Writes 4 words. */
void ump_stream_config(uint32_t *words, unsigned protocol, unsigned jr);
/* Endpoint Name (7.1.4) or Product Instance Id (7.1.5) Notification: text
 * split over as many 4-word UMPs as needed (14 bytes each). Returns the
 * words written, 0 when capacity is too small or the text is not one the
 * specification allows: a name of at most 98 bytes, an id of at most 42
 * bytes of ASCII from 32 to 126. */
#define UMP_ENDPOINT_NAME_MAX 98
#define UMP_PRODUCT_INSTANCE_ID_MAX 42
unsigned ump_stream_text(uint32_t *words, size_t capacity_words,
                            unsigned status, const char *text, size_t length);

#endif
