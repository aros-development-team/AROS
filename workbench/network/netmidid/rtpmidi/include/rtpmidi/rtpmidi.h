/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef RTPMIDI_H
#define RTPMIDI_H

#include <stddef.h>
#include <stdint.h>

/* The payload type AppleMIDI sessions use. It is a dynamic type that such a
 * session has no way to negotiate, so every implementation uses this one. */
#define RTPMIDI_PAYLOAD_TYPE 97

/* An RTP-MIDI packet (RFC 6295). The MIDI list and any recovery journal are
 * exposed as bounded byte spans for separate decoding. A header with
 * contributing sources, an extension or padding is accepted and those parts
 * skipped; the marker bit is not required to be either way. */
struct rtpmidi_packet {
    uint16_t sequence;
    uint32_t timestamp;
    uint32_t ssrc;
    const uint8_t *midi;
    size_t midi_length;
    const uint8_t *journal;
    size_t journal_length;
    uint8_t z;
    uint8_t p;
};

struct rtpmidi_journal_channel {
    const uint8_t *data;
    size_t length;
    uint8_t number;
    uint8_t chapters;
    uint8_t single_packet_safe;
};

struct rtpmidi_journal {
    uint16_t checkpoint;
    uint8_t single_packet_safe;
    uint8_t enhanced_controllers;
    const uint8_t *system;
    size_t system_length;
    struct rtpmidi_journal_channel channels[16];
    size_t channel_count;
};

/* Decode journal framing. Chapter contents remain opaque. */
int rtpmidi_journal_decode(const uint8_t *data, size_t length,
                      struct rtpmidi_journal *journal);
int rtpmidi_journal_covers_gap(uint16_t previous, uint16_t current,
                          uint16_t checkpoint);

struct rtpmidi_journal_system_state {
    int has_reset;
    int reset_single_packet_safe;
    uint8_t reset_count;
    int has_tune_request;
    int tune_single_packet_safe;
    uint8_t tune_count;
    int has_song_select;
    int song_single_packet_safe;
    uint8_t song;
    int has_active_sense;
    int active_sense_single_packet_safe;
    uint8_t active_sense_count;
    int has_sequencer;
    int sequencer_single_packet_safe;
    int sequencer_running;
    int downbeat_played;
    int has_clock;
    uint32_t clock;
    int has_time_tools;
    uint32_t time_tools;
    int has_mtc;
    int mtc_single_packet_safe;
    int mtc_has_complete;
    int mtc_complete_quarter_frame;
    int mtc_has_partial;
    int mtc_reverse;
    uint8_t mtc_point;
    uint8_t mtc_complete[8];
    uint8_t mtc_partial[8];
    const uint8_t *sysex;
    size_t sysex_length;
};

/* Decode standard commands in System Chapters D, V, Q, and F. Chapter X is
 * exposed as a bounded span for rtpmidi_journal_decode_sysex(). */
int rtpmidi_journal_decode_system(const struct rtpmidi_journal *journal,
                             struct rtpmidi_journal_system_state *state);

#define RTPMIDI_JOURNAL_SYSEX_LOGS 32
struct rtpmidi_journal_sysex_log {
    int single_packet_safe;
    int list_tool;
    uint8_t status;
    int has_tcount;
    uint8_t tcount;
    int has_count;
    uint8_t count;
    int has_first;
    uint32_t first;
    const uint8_t *data;
    size_t data_length;
};

struct rtpmidi_journal_sysex {
    struct rtpmidi_journal_sysex_log logs[RTPMIDI_JOURNAL_SYSEX_LOGS];
    size_t count;
};

/* Decode Chapter X command logs. */
int rtpmidi_journal_decode_sysex(const struct rtpmidi_journal_system_state *state,
                            struct rtpmidi_journal_sysex *sysex);

struct rtpmidi_journal_note_log {
    uint8_t number;
    uint8_t velocity;
    uint8_t simultaneous;
    uint8_t single_packet_safe;
};

struct rtpmidi_journal_notes {
    uint8_t offbits[16];
    int offbits_single_packet_safe;
    struct rtpmidi_journal_note_log logs[128];
    size_t log_count;
};

/* Decode Chapter N. 0: absent, 1: decoded, -1: malformed. */
int rtpmidi_journal_decode_notes(const struct rtpmidi_journal_channel *channel,
                            struct rtpmidi_journal_notes *notes);

struct rtpmidi_journal_note_extra {
    uint8_t number;
    uint8_t value;
    uint8_t velocity;
    uint8_t single_packet_safe;
};

struct rtpmidi_journal_note_extras {
    struct rtpmidi_journal_note_extra logs[128];
    size_t count;
};

/* Decode Chapter E note reference counts and release velocities. */
int rtpmidi_journal_decode_note_extras(
    const struct rtpmidi_journal_channel *channel,
    struct rtpmidi_journal_note_extras *extras);

struct rtpmidi_journal_channel_state {
    int has_program;
    int program_single_packet_safe;
    uint8_t program;
    int has_bank;
    uint8_t bank_msb;
    uint8_t bank_lsb;
    int has_pitch;
    int pitch_single_packet_safe;
    uint8_t pitch_lsb;
    uint8_t pitch_msb;
};

/* Decode Chapters P and W. 0: neither present, 1: decoded, -1: malformed. */
int rtpmidi_journal_decode_channel_state(
    const struct rtpmidi_journal_channel *channel,
    struct rtpmidi_journal_channel_state *state);

struct rtpmidi_journal_control_log {
    uint8_t number;
    uint8_t value;
    uint8_t alternate;
    uint8_t count_tool;
    uint8_t single_packet_safe;
    uint8_t command;
};

struct rtpmidi_journal_controls {
    struct rtpmidi_journal_control_log logs[128];
    size_t count;
    uint8_t enhanced;
};

/* Decode Chapter C logs; alternate tools are marked for separate handling. */
int rtpmidi_journal_decode_controls(const struct rtpmidi_journal_channel *channel,
                                struct rtpmidi_journal_controls *controls);

struct rtpmidi_journal_parameter_log {
    uint16_t number;
    uint8_t nrpn;
    uint8_t single_packet_safe;
    uint8_t value_tool;
    uint8_t count_tool;
    uint8_t has_entry_msb;
    uint8_t entry_msb;
    uint8_t has_entry_lsb;
    uint8_t entry_lsb;
    uint8_t has_adjust;
    int16_t adjust;
    uint8_t has_count;
    uint8_t count;
};

struct rtpmidi_journal_parameters {
    int single_packet_safe;
    int has_pending;
    int pending_nrpn;
    uint8_t pending_msb;
    int transaction_open;
    struct rtpmidi_journal_parameter_log logs[128];
    size_t count;
};

/* Decode Chapter M RPN/NRPN state and bounded parameter logs. */
int rtpmidi_journal_decode_parameters(const struct rtpmidi_journal_channel *channel,
                                 struct rtpmidi_journal_parameters *parameters);

struct rtpmidi_journal_poly_pressure {
    uint8_t number;
    uint8_t pressure;
    uint8_t reset_notes;
    uint8_t single_packet_safe;
};

struct rtpmidi_journal_aftertouch {
    int has_channel_pressure;
    int channel_single_packet_safe;
    uint8_t channel_pressure;
    struct rtpmidi_journal_poly_pressure poly[128];
    size_t poly_count;
};

/* Decode Chapters T and A. 0: absent, 1: decoded, -1: malformed. */
int rtpmidi_journal_decode_aftertouch(const struct rtpmidi_journal_channel *channel,
                                  struct rtpmidi_journal_aftertouch *aftertouch);

/* Iterates MIDI 1.0 short commands in a decoded RTP-MIDI command section.
 * SysEx and unsupported system commands return -2; malformed data returns
 * -1. A result of 1 contains a command, and 0 means end of list. */
struct rtpmidi_event {
    uint8_t bytes[3];
    uint8_t length;
    uint32_t timestamp;
    const uint8_t *sysex;
    size_t sysex_length;
};

/* The longest SysEx message taken, F0 and F7 counted, in either direction:
 * 32 KiB, which holds the bulk dumps of the instruments this was tried
 * against on paper (a DX7 voice bank is 4104 bytes, already past the 4096
 * this used to be). A stream lends its assembler a buffer of this size
 * while it has a peer. */
enum { RTPMIDI_SYSEX_MAX = 32768 };

struct rtpmidi_sysex_assembler {
    uint8_t *bytes;             /* the caller's, of capacity bytes */
    size_t capacity;
    size_t length;
    uint32_t timestamp;
    uint16_t last_sequence;
    uint8_t active;
};

/* Complete and segmented SysEx messages. A result of 1 provides a complete
 * F0..F7 message in *message; 0 means more segments are needed; -1 means a
 * malformed, lost, or oversized message was discarded. */
/* Where messages are gathered: a buffer of the caller's, which may be
 * NULL, when every message with more than one segment is refused. Call
 * it once before anything else; rtpmidi_sysex_reset keeps the buffer. */
void rtpmidi_sysex_buffer(struct rtpmidi_sysex_assembler *assembler,
                          uint8_t *buffer, size_t capacity);
void rtpmidi_sysex_reset(struct rtpmidi_sysex_assembler *assembler);
/* Observe every validated, fresh RTP packet before feeding its commands,
 * including packets without SysEx. A sequence gap abandons partial SysEx.
 * The caller must filter duplicate and older packets first. */
void rtpmidi_sysex_packet(struct rtpmidi_sysex_assembler *assembler,
                         uint16_t sequence);
int rtpmidi_sysex_feed(struct rtpmidi_sysex_assembler *assembler,
                  const struct rtpmidi_event *event, uint16_t sequence,
                  const uint8_t **message, size_t *length);

struct rtpmidi_reader {
    const struct rtpmidi_packet *packet;
    size_t offset;
    uint32_t timestamp;
    uint8_t running_status;    unsigned int undefined;     /* undefined statuses stepped over */
};

void rtpmidi_reader_init(struct rtpmidi_reader *reader,
                        const struct rtpmidi_packet *packet);
int rtpmidi_reader_next(struct rtpmidi_reader *reader,
                       struct rtpmidi_event *event);

/* Builds a single, complete MIDI 1.0 command with implicit delta time zero
 * and no recovery journal. Returns 0 for success, -1 for invalid input. */
/* The marker bit is sent clear always: that is what AppleMIDI
 * implementations do and one of those we met requires, against RFC 6295
 * section 2.1, which would set it when the list has commands. */
int rtpmidi_encode_short(uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                        const uint8_t *midi, size_t midi_length,
                        uint8_t *data, size_t capacity, size_t *length);
/* Whether these bytes are one whole message other than SysEx. */
int rtpmidi_short_valid(const uint8_t *midi, size_t midi_length);
int rtpmidi_encode_list(uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                       const uint8_t *midi, size_t midi_length,
                       uint8_t *data, size_t capacity, size_t *length);
int rtpmidi_decode(const uint8_t *data, size_t length,
                  struct rtpmidi_packet *packet);

#endif
