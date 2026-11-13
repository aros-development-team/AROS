/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef APPLEMIDI_SESSION_H
#define APPLEMIDI_SESSION_H

#include "applemidi/applemidi.h"

/* One peer per session. The caller owns UDP sockets, peer address validation,
 * random token generation, retries and timeouts. No OS calls occur here. */
enum applemidi_session_phase {
    APPLEMIDI_SESSION_IDLE,
    APPLEMIDI_SESSION_INVITING_CONTROL,
    APPLEMIDI_SESSION_INVITING_DATA,
    APPLEMIDI_SESSION_WAITING_DATA,
    APPLEMIDI_SESSION_CONNECTED
};

struct applemidi_session {
    enum applemidi_session_phase phase;
    uint32_t local_ssrc;
    uint32_t peer_ssrc;
    uint32_t token;
    uint8_t name[64];
    size_t name_length;
    /* Counted for the caller, which compares before and after a call. */
    unsigned int restarts;          /* the peer began again: see below */
    unsigned int ssrc_collisions;   /* the peer has this side's SSRC */
};

/* The output packet points to session-owned name storage. The caller may
 * immediately pass it to applemidi_encode(). */
int applemidi_session_init(struct applemidi_session *session, uint32_t local_ssrc,
                    const uint8_t *name, size_t name_length);
int applemidi_session_invite(struct applemidi_session *session, uint32_t token,
                      struct applemidi_packet *out);
int applemidi_session_retry(const struct applemidi_session *session,
                     struct applemidi_packet *out, int *out_data_port);

/* Returns 1 when an outgoing packet is ready, 0 when no reply is needed,
 * -1 for invalid input. out_data_port selects N+1 rather than control N.
 *
 * A peer that went away without BY and comes back invites again with a new
 * token. Such an invitation on the control port, in a session that the
 * peer's side had reached, ends that session and begins the new one:
 * restarts is counted and the phase is APPLEMIDI_SESSION_WAITING_DATA
 * again. Whatever the caller kept for the old session has to go.
 *
 * A peer whose SSRC is this side's is refused, with NO for an invitation
 * and BY for an acceptance, and ssrc_collisions is counted. The caller
 * then takes another SSRC with applemidi_session_renew(). */
int applemidi_session_receive(struct applemidi_session *session, int on_data_port,
                       const struct applemidi_packet *in,
                       struct applemidi_packet *out, int *out_data_port);

/* Another SSRC for this side, in an idle session only. */
int applemidi_session_renew(struct applemidi_session *session,
                            uint32_t local_ssrc);

/* Emits BY on control port and resets the local state. */
int applemidi_session_end(struct applemidi_session *session, struct applemidi_packet *out);

#endif
