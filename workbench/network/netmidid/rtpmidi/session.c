/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "rtpmidi/session.h"

#include <string.h>

static void make_packet(const struct applemidi_session *session,
                        enum applemidi_command command,
                        struct applemidi_packet *out)
{
    memset(out, 0, sizeof(*out));
    out->command = command;
    out->token = session->token;
    out->ssrc = session->local_ssrc;
    if (command != APPLEMIDI_NO && command != APPLEMIDI_BY) {
        out->name = session->name;
        out->name_length = session->name_length;
    }
}

int applemidi_session_init(struct applemidi_session *session, uint32_t local_ssrc,
                    const uint8_t *name, size_t name_length)
{
    if (!session || (!name && name_length) || name_length >= sizeof(session->name))
        return -1;
    memset(session, 0, sizeof(*session));
    session->local_ssrc = local_ssrc;
    if (name_length)
        memcpy(session->name, name, name_length);
    session->name_length = name_length;
    return 0;
}

int applemidi_session_invite(struct applemidi_session *session, uint32_t token,
                      struct applemidi_packet *out)
{
    if (!session || !out || session->phase != APPLEMIDI_SESSION_IDLE)
        return -1;
    session->token = token;
    session->phase = APPLEMIDI_SESSION_INVITING_CONTROL;
    make_packet(session, APPLEMIDI_IN, out);
    return 0;
}

int applemidi_session_retry(const struct applemidi_session *session,
                     struct applemidi_packet *out, int *out_data_port)
{
    if (!session || !out || !out_data_port ||
        (session->phase != APPLEMIDI_SESSION_INVITING_CONTROL &&
         session->phase != APPLEMIDI_SESSION_INVITING_DATA))
        return -1;
    make_packet(session, APPLEMIDI_IN, out);
    *out_data_port = session->phase == APPLEMIDI_SESSION_INVITING_DATA;
    return 0;
}

int applemidi_session_receive(struct applemidi_session *session, int on_data_port,
                       const struct applemidi_packet *in,
                       struct applemidi_packet *out, int *out_data_port)
{
    if (!session || !in || !out || !out_data_port ||
        (on_data_port != 0 && on_data_port != 1))
        return -1;
    *out_data_port = 0;

    if (in->command == APPLEMIDI_BY) {
        if (session->phase != APPLEMIDI_SESSION_IDLE &&
            in->ssrc == session->peer_ssrc && in->token == session->token)
            session->phase = APPLEMIDI_SESSION_IDLE;
        return 0;
    }

    if ((in->command == APPLEMIDI_IN || in->command == APPLEMIDI_OK) &&
        in->ssrc == session->local_ssrc) {
        /* Two ends of a stream cannot share an SSRC. */
        if (in->command == APPLEMIDI_OK &&
            (session->phase != APPLEMIDI_SESSION_INVITING_CONTROL &&
             session->phase != APPLEMIDI_SESSION_INVITING_DATA))
            return 0;
        if (in->command == APPLEMIDI_OK && in->token != session->token)
            return 0;
        ++session->ssrc_collisions;
        memset(out, 0, sizeof(*out));
        out->command = in->command == APPLEMIDI_IN ? APPLEMIDI_NO :
                                                     APPLEMIDI_BY;
        out->token = in->token;
        out->ssrc = session->local_ssrc;
        if (in->command == APPLEMIDI_OK)
            session->phase = APPLEMIDI_SESSION_IDLE;
        return 1;
    }
    if (in->command == APPLEMIDI_IN) {
        if (!on_data_port && session->phase == APPLEMIDI_SESSION_IDLE) {
            session->token = in->token;
            session->peer_ssrc = in->ssrc;
            session->phase = APPLEMIDI_SESSION_WAITING_DATA;
        } else if (!on_data_port &&
                   (session->phase == APPLEMIDI_SESSION_WAITING_DATA ||
                    session->phase == APPLEMIDI_SESSION_CONNECTED) &&
                   (in->token != session->token ||
                    in->ssrc != session->peer_ssrc)) {
            /* The peer began again without saying goodbye. */
            session->token = in->token;
            session->peer_ssrc = in->ssrc;
            session->phase = APPLEMIDI_SESSION_WAITING_DATA;
            ++session->restarts;
        } else if (in->token != session->token ||
                   in->ssrc != session->peer_ssrc ||
                   !((!on_data_port && session->phase == APPLEMIDI_SESSION_WAITING_DATA) ||
                     (on_data_port && (session->phase == APPLEMIDI_SESSION_WAITING_DATA ||
                                       session->phase == APPLEMIDI_SESSION_CONNECTED)))) {
            return 0;
        }
        if (on_data_port)
            session->phase = APPLEMIDI_SESSION_CONNECTED;
        make_packet(session, APPLEMIDI_OK, out);
        *out_data_port = on_data_port;
        return 1;
    }

    if (in->command == APPLEMIDI_NO &&
        (session->phase == APPLEMIDI_SESSION_INVITING_CONTROL ||
         session->phase == APPLEMIDI_SESSION_INVITING_DATA) &&
        in->token == session->token) {
        session->phase = APPLEMIDI_SESSION_IDLE;
        return 0;
    }

    if (in->command != APPLEMIDI_OK || in->token != session->token)
        return 0;
    if (!on_data_port && session->phase == APPLEMIDI_SESSION_INVITING_CONTROL) {
        session->peer_ssrc = in->ssrc;
        session->phase = APPLEMIDI_SESSION_INVITING_DATA;
        make_packet(session, APPLEMIDI_IN, out);
        *out_data_port = 1;
        return 1;
    }
    if (on_data_port && session->phase == APPLEMIDI_SESSION_INVITING_DATA &&
        in->ssrc == session->peer_ssrc)
        session->phase = APPLEMIDI_SESSION_CONNECTED;
    return 0;
}

int applemidi_session_renew(struct applemidi_session *session,
                            uint32_t local_ssrc)
{
    if (!session || session->phase != APPLEMIDI_SESSION_IDLE)
        return -1;
    session->local_ssrc = local_ssrc;
    return 0;
}

int applemidi_session_end(struct applemidi_session *session, struct applemidi_packet *out)
{
    if (!session || !out || session->phase == APPLEMIDI_SESSION_IDLE)
        return -1;
    make_packet(session, APPLEMIDI_BY, out);
    session->phase = APPLEMIDI_SESSION_IDLE;
    return 0;
}
