/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "rtpmidi/peers.h"

#include <string.h>

int applemidi_peers_init(struct applemidi_peers *peers, uint32_t local_ssrc,
                  const uint8_t *name, size_t name_length)
{
    if (!peers || (!name && name_length) || name_length >= sizeof(peers->name))
        return -1;
    memset(peers, 0, sizeof(*peers));
    peers->local_ssrc = local_ssrc;
    if (name_length)
        memcpy(peers->name, name, name_length);
    peers->name_length = name_length;
    peers->accept = APPLEMIDI_ACCEPT_ANYONE;
    peers->limit = APPLEMIDI_PEERS_MAX;
    return 0;
}

int applemidi_peers_allow(struct applemidi_peers *peers, uint32_t address,
                          const char *name)
{
    struct applemidi_allowed *entry;
    size_t length = name ?
        applemidi_text_length(name, APPLEMIDI_PEER_NAME_MAX) : 0;

    if (!peers || peers->allowed_count == APPLEMIDI_ALLOWED_MAX ||
        (!address && !length) || length > APPLEMIDI_PEER_NAME_MAX)
        return -1;
    entry = &peers->allowed[peers->allowed_count++];
    memset(entry, 0, sizeof(*entry));
    entry->address = address;
    if (length)
        memcpy(entry->name, name, length);
    return 0;
}

/* Whether the rule lets this invitation in. */
static int may_connect(const struct applemidi_peers *peers, uint32_t address,
                       const struct applemidi_packet *in)
{
    int i;

    if (peers->accept == APPLEMIDI_ACCEPT_ANYONE)
        return 1;
    if (peers->accept != APPLEMIDI_ACCEPT_LISTED)
        return 0;
    for (i = 0; i < peers->allowed_count; ++i) {
        const struct applemidi_allowed *entry = &peers->allowed[i];
        size_t length = applemidi_text_length(entry->name,
                                              APPLEMIDI_PEER_NAME_MAX);
        if (entry->address && entry->address == address)
            return 1;
        if (length && in->name && in->name_length == length &&
            memcmp(in->name, entry->name, length) == 0)
            return 1;
    }
    return 0;
}

static int find(const struct applemidi_peers *peers, uint32_t address, uint16_t control_port)
{
    int i;
    for (i = 0; i < APPLEMIDI_PEERS_MAX; ++i)
        if (peers->peer[i].used && peers->peer[i].address == address &&
            peers->peer[i].control_port == control_port)
            return i;
    return -1;
}

static int allocate(struct applemidi_peers *peers, uint32_t address, uint16_t control_port,
                    uint32_t now_ms)
{
    int i;
    int used = 0;
    for (i = 0; i < APPLEMIDI_PEERS_MAX; ++i)
        used += peers->peer[i].used;
    if (used >= peers->limit)
        return -1;
    for (i = 0; i < APPLEMIDI_PEERS_MAX; ++i) {
        struct applemidi_peer *peer = &peers->peer[i];
        if (peer->used)
            continue;
        memset(peer, 0, sizeof(*peer));
        if (applemidi_session_init(&peer->session, peers->local_ssrc,
                            peers->name, peers->name_length))
            return -1;
        peer->used = 1;
        peer->address = address;
        peer->control_port = control_port;
        peer->last_seen_ms = now_ms;
        return i;
    }
    return -1;
}

static void release(struct applemidi_peers *peers, int index)
{
    memset(&peers->peer[index], 0, sizeof(peers->peer[index]));
}

static void remember_name(struct applemidi_peer *peer, const struct applemidi_packet *in)
{
    size_t length = in->name_length;
    if (!in->name || !length)
        return;
    length = applemidi_name_fit(in->name, length, APPLEMIDI_PEER_NAME_MAX);
    memcpy(peer->name, in->name, length);
    peer->name[length] = 0;
}

int applemidi_peers_receive(struct applemidi_peers *peers, uint32_t address, uint16_t port,
                     int on_data_port, const struct applemidi_packet *in,
                     uint32_t now_ms, struct applemidi_packet *out,
                     int *out_data_port, int *index, enum applemidi_peer_event *event)
{
    uint16_t control_port;
    int i, result;
    unsigned int restarts, collisions;
    struct applemidi_peer *peer;

    if (!peers || !in || !out || !out_data_port || !index || !event ||
        (on_data_port != 0 && on_data_port != 1) || (on_data_port && port == 0))
        return -1;
    *index = -1;
    *event = APPLEMIDI_PEER_NONE;
    *out_data_port = 0;
    control_port = (uint16_t)(on_data_port ? port - 1 : port);

    i = find(peers, address, control_port);
    if (i < 0) {
        /* Only an invitation on the control port starts a peer. */
        if (in->command != APPLEMIDI_IN || on_data_port)
            return 0;
        if (!may_connect(peers, address, in) ||
            (i = allocate(peers, address, control_port, now_ms)) < 0) {
            ++peers->refused;
            memset(out, 0, sizeof(*out));
            out->command = APPLEMIDI_NO;
            out->token = in->token;
            out->ssrc = peers->local_ssrc;
            return 1;
        }
    }
    peer = &peers->peer[i];
    *index = i;
    peer->last_seen_ms = now_ms;
    if (in->command == APPLEMIDI_IN || in->command == APPLEMIDI_OK)
        remember_name(peer, in);

    restarts = peer->session.restarts;
    collisions = peer->session.ssrc_collisions;
    result = applemidi_session_receive(&peer->session, on_data_port, in, out, out_data_port);
    if (result < 0)
        return -1;
    if (peer->session.ssrc_collisions != collisions) {
        /* refused: it has our SSRC, which every session of ours carries */
        ++peers->ssrc_collisions;
        *event = peer->connected_reported ? APPLEMIDI_PEER_LEFT :
                                            APPLEMIDI_PEER_NONE;
        release(peers, i);
        *index = *event == APPLEMIDI_PEER_NONE ? -1 : i;
        return result;
    }
    if (peer->session.restarts != restarts) {
        peer->connected_reported = 0;
        *event = APPLEMIDI_PEER_RESTARTED;
        return result;
    }

    if (peer->session.phase == APPLEMIDI_SESSION_CONNECTED && !peer->connected_reported) {
        peer->connected_reported = 1;
        *event = APPLEMIDI_PEER_CONNECTED;
    } else if (peer->session.phase == APPLEMIDI_SESSION_IDLE &&
               (in->command == APPLEMIDI_BY || in->command == APPLEMIDI_NO)) {
        *event = peer->connected_reported || in->command == APPLEMIDI_NO ?
                 APPLEMIDI_PEER_LEFT : APPLEMIDI_PEER_NONE;
        release(peers, i);
        if (*event == APPLEMIDI_PEER_NONE)
            *event = APPLEMIDI_PEER_LEFT;
    }
    return result;
}

int applemidi_peers_invite(struct applemidi_peers *peers, uint32_t address, uint16_t control_port,
                    uint32_t token, uint32_t now_ms, struct applemidi_packet *out)
{
    int i;

    if (!peers || !out || find(peers, address, control_port) >= 0)
        return -1;
    i = allocate(peers, address, control_port, now_ms);
    if (i < 0)
        return -1;
    if (applemidi_session_invite(&peers->peer[i].session, token, out)) {
        release(peers, i);
        return -1;
    }
    return i;
}

int applemidi_peers_retry(const struct applemidi_peers *peers, int index,
                   struct applemidi_packet *out, int *out_data_port)
{
    if (!peers || !out || !out_data_port || index < 0 ||
        index >= APPLEMIDI_PEERS_MAX || !peers->peer[index].used)
        return -1;
    return applemidi_session_retry(&peers->peer[index].session, out, out_data_port);
}

int applemidi_peers_find_control(const struct applemidi_peers *peers,
                                 uint32_t address, uint16_t control_port)
{
    return peers ? find(peers, address, control_port) : -1;
}

int applemidi_peers_renew(struct applemidi_peers *peers, uint32_t local_ssrc)
{
    int i;
    if (!peers)
        return -1;
    for (i = 0; i < APPLEMIDI_PEERS_MAX; ++i)
        if (peers->peer[i].used)
            return -1;
    peers->local_ssrc = local_ssrc;
    return 0;
}

int applemidi_peers_find_data(const struct applemidi_peers *peers, uint32_t address,
                       uint16_t data_port)
{
    int i;
    if (!peers || data_port == 0)
        return -1;
    i = find(peers, address, (uint16_t)(data_port - 1));
    if (i >= 0 && peers->peer[i].session.phase != APPLEMIDI_SESSION_CONNECTED)
        return -1;
    return i;
}

int applemidi_peers_find_ssrc(const struct applemidi_peers *peers, uint32_t ssrc)
{
    int i;
    for (i = 0; peers && i < APPLEMIDI_PEERS_MAX; ++i)
        if (peers->peer[i].used && peers->peer[i].session.phase != APPLEMIDI_SESSION_IDLE &&
            peers->peer[i].session.peer_ssrc == ssrc)
            return i;
    return -1;
}

void applemidi_peers_seen(struct applemidi_peers *peers, int index, uint32_t now_ms)
{
    if (peers && index >= 0 && index < APPLEMIDI_PEERS_MAX && peers->peer[index].used)
        peers->peer[index].last_seen_ms = now_ms;
}

int applemidi_peers_end(struct applemidi_peers *peers, int index, struct applemidi_packet *out)
{
    int result = 0;

    if (!peers || !out || index < 0 || index >= APPLEMIDI_PEERS_MAX || !peers->peer[index].used)
        return -1;
    if (peers->peer[index].session.phase == APPLEMIDI_SESSION_IDLE ||
        applemidi_session_end(&peers->peer[index].session, out))
        result = -1;        /* nothing was established: no BY to send */
    release(peers, index);
    return result;
}

int applemidi_peers_expired(const struct applemidi_peers *peers, uint32_t now_ms,
                     uint32_t timeout_ms)
{
    int i;
    for (i = 0; peers && i < APPLEMIDI_PEERS_MAX; ++i)
        if (peers->peer[i].used && now_ms - peers->peer[i].last_seen_ms >= timeout_ms)
            return i;
    return -1;
}
