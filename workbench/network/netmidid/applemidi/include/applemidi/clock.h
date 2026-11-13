/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef APPLEMIDI_CLOCK_H
#define APPLEMIDI_CLOCK_H

#include <stddef.h>
#include <stdint.h>

#include "applemidi/applemidi.h"

/* The clock exchange of one AppleMIDI session. The side that sent the
 * invitation starts each exchange with a CK that has its own time; the
 * other side answers with its time; the first answers again with its time
 * at that moment. Each side then knows the other's clock against its own.
 * Times are in the session's unit, a ten-thousandth of a second. The
 * caller owns the socket and the clock; no OS calls occur here. */

/* An exchange a second for the first three, then one in ten seconds; a
 * session whose inviter has sent no CK for a minute is taken for dead.
 * Ten seconds is what the other implementations read do: node-rtpmidi
 * 1.0.0 (Stream.js) and Benoit Bouchez's RTP-MIDI class (RTP_MIDI.cpp)
 * both begin with exchanges 1.5 seconds apart and go on with one in ten
 * seconds, and the second gives up a session after four of its timer's
 * turns, about forty seconds, without one. A longer interval than that
 * would lose us such a peer. The first three a second apart, and the
 * minute, are ours. */
#define APPLEMIDI_CLOCK_FIRST_EXCHANGES 3
#define APPLEMIDI_CLOCK_FIRST_INTERVAL 10000u
#define APPLEMIDI_CLOCK_INTERVAL 100000u
#define APPLEMIDI_CLOCK_TIMEOUT 600000u
/* Of this many exchanges, the last ones, the one with the shortest round
 * trip gives the offset: a CK held up on its way says less about the
 * clocks than one that was not. */
#define APPLEMIDI_CLOCK_KEPT 8

struct applemidi_clock {
    int initiator;              /* this side sent the invitation */
    int synchronized;           /* peer_to_local is known */
    int64_t peer_to_local;      /* add to a time of the peer's */
    unsigned int completed;     /* exchanges brought to their end */
    /* initiator */
    int started;                /* a first CK went out */
    int outstanding;            /* its answer has not come */
    uint64_t sent;              /* the time in the CK awaited */
    unsigned int early;         /* of the first exchanges, how many done */
    /* the other side */
    int answered;               /* a CK was answered and awaits its third */
    uint64_t peer_time;         /* the time it carried */
    uint64_t own_time;          /* the time we answered with */
    uint64_t heard;             /* when the peer last began an exchange */
    /* the last exchanges: how long each took and what offset it gave */
    struct {
        uint64_t round_trip;
        int64_t offset;
    } kept[APPLEMIDI_CLOCK_KEPT];
    unsigned int kept_count;
    unsigned int kept_next;
};

/* Convert the three CK timestamps to a peer-to-local clock offset. */
int applemidi_clock_offset(const uint64_t timestamps[3], int is_initiator,
                           int64_t *peer_to_local);

/* At the moment the session is connected. */
void applemidi_clock_init(struct applemidi_clock *clock, int initiator,
                          uint64_t now);

/* Call regularly. Returns 1 when out is a CK to send on the data port: the
 * first at once, then by the intervals above. The other side never sends
 * of its own accord. */
int applemidi_clock_poll(struct applemidi_clock *clock, uint32_t local_ssrc,
                         uint64_t now, struct applemidi_packet *out);

/* A CK from the session's peer; the caller has checked its SSRC. Returns 1
 * when out is the answer to send on the data port. A packet that belongs
 * to no exchange in progress is ignored. */
int applemidi_clock_receive(struct applemidi_clock *clock,
                            uint32_t local_ssrc,
                            const struct applemidi_packet *in, uint64_t now,
                            struct applemidi_packet *out);

/* Whether the peer that should be starting exchanges has gone silent.
 * A now sample older than heard is not evidence of silence. */
int applemidi_clock_expired(const struct applemidi_clock *clock,
                            uint64_t now);

#endif
