/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef APPLEMIDI_PEERS_H
#define APPLEMIDI_PEERS_H

#include <stddef.h>
#include <stdint.h>

#include "rtpmidi/session.h"

/* Several AppleMIDI sessions at once, one per peer, so that a Mac and an
 * iPad can both be connected. Each peer is one session (session.h) found
 * by its address: the peer's control port, its data port being the next
 * one. The caller owns the sockets and the clock and decodes and encodes
 * the packets; no OS calls occur here. */

#define APPLEMIDI_PEERS_MAX 8
#define APPLEMIDI_PEER_NAME_MAX 63
#define APPLEMIDI_ALLOWED_MAX 8

/* Whose invitations are taken: what Apple's own setup calls "who may
 * connect to me". It says nothing of whom we invite. */
enum applemidi_accept {
    APPLEMIDI_ACCEPT_NOBODY,
    APPLEMIDI_ACCEPT_ANYONE,
    APPLEMIDI_ACCEPT_LISTED     /* those of the list below */
};

/* One who may connect: by address, or by the name its invitation carries.
 * A name is whatever the peer says it is, so it keeps out mistakes, not
 * anyone who means to come in. */
struct applemidi_allowed {
    uint32_t address;           /* IPv4, host order; 0 for none */
    char name[APPLEMIDI_PEER_NAME_MAX + 1];     /* empty for none */
};

struct applemidi_peer {
    int used;
    uint32_t address;           /* IPv4, host order */
    uint16_t control_port;      /* the peer's; data is control_port + 1 */
    struct applemidi_session session;
    char name[APPLEMIDI_PEER_NAME_MAX + 1];
    uint32_t last_seen_ms;
    int connected_reported;
};

enum applemidi_peer_event {
    APPLEMIDI_PEER_NONE,
    APPLEMIDI_PEER_CONNECTED,          /* the session reached the data port */
    APPLEMIDI_PEER_LEFT,               /* BY received, refused or expired */
    APPLEMIDI_PEER_RESTARTED           /* it invited again without BY: what
                                          was kept for it has to go, and
                                          CONNECTED follows as for a new one */
};

struct applemidi_peers {
    struct applemidi_peer peer[APPLEMIDI_PEERS_MAX];
    uint32_t local_ssrc;
    uint8_t name[64];
    size_t name_length;
    int accept;                 /* enum applemidi_accept; anyone at first */
    struct applemidi_allowed allowed[APPLEMIDI_ALLOWED_MAX];
    int allowed_count;
    unsigned int refused;       /* invitations answered with NO by the rule
                                   or for want of room */
    unsigned int ssrc_collisions;   /* peers refused for having our SSRC */
    int limit;                  /* peers at once: APPLEMIDI_PEERS_MAX after
                                   applemidi_peers_init, or fewer when the
                                   caller says so; one more is refused
                                   with NO and counted in refused */
};

/* How long a caller waits, in the session's unit, a ten-thousandth of a
 * second. The table keeps no time itself; these are for whoever runs it,
 * so that every runtime waits alike.
 *
 * An invitation is sent again every second, twelve times, and then given
 * up: what Benoit Bouchez's RTP-MIDI class does (RTP_MIDI.cpp), and close
 * to node-rtpmidi's 1.5 seconds, forty times (Stream.js). */
#define APPLEMIDI_INVITE_INTERVAL 10000u
#define APPLEMIDI_INVITE_TRIES 12
/* A peer that invited us on the control port and never does on the data
 * port is forgotten after twelve seconds: as long as we would go on
 * inviting it ourselves. */
#define APPLEMIDI_DATA_INVITE_TIMEOUT 120000u
/* A peer wanted by name that does not answer is invited again after five
 * seconds, then after twice as long each time, up to a minute: it is
 * advertised, so it is there, and may be refusing us on purpose. */
#define APPLEMIDI_WANTED_FIRST_WAIT 50000u
#define APPLEMIDI_WANTED_LONGEST_WAIT 600000u

int applemidi_peers_init(struct applemidi_peers *peers, uint32_t local_ssrc,
                  const uint8_t *name, size_t name_length);

/* Adds one who may connect under APPLEMIDI_ACCEPT_LISTED, by address
 * (name NULL) or by name (address 0). Returns -1 when the list is full or
 * neither is given. */
int applemidi_peers_allow(struct applemidi_peers *peers, uint32_t address,
                          const char *name);

/* Handles an AppleMIDI control packet from address:port, which is the
 * peer's control port or, with on_data_port, its data port. When a reply
 * is due, out is filled and *out_data_port says which local port sends
 * it; the function returns 1. It returns 0 when nothing is to be sent,
 * -1 for invalid input. *index is the peer concerned (-1 when none) and
 * *event what happened to it. A new peer is accepted when there is room
 * and accept is set; otherwise its invitation is answered with NO. */
int applemidi_peers_receive(struct applemidi_peers *peers, uint32_t address, uint16_t port,
                     int on_data_port, const struct applemidi_packet *in,
                     uint32_t now_ms, struct applemidi_packet *out,
                     int *out_data_port, int *index, enum applemidi_peer_event *event);

/* Invites address:control_port. Returns the peer index, -1 when the table
 * is full or the peer is already known. out is the IN to send on the
 * local control port. */
int applemidi_peers_invite(struct applemidi_peers *peers, uint32_t address, uint16_t control_port,
                    uint32_t token, uint32_t now_ms, struct applemidi_packet *out);

/* Rebuilds the outstanding IN for a peer whose invitation timed out.
 * out_data_port identifies whether it belongs on the data socket. */
int applemidi_peers_retry(const struct applemidi_peers *peers, int index,
                   struct applemidi_packet *out, int *out_data_port);

/* The peer whose control port is address:control_port, in any phase, or
 * -1. */
int applemidi_peers_find_control(const struct applemidi_peers *peers,
                                 uint32_t address, uint16_t control_port);
/* Another SSRC for this side. Every session carries the one SSRC, so this
 * is only possible while the table is empty; -1 otherwise. */
int applemidi_peers_renew(struct applemidi_peers *peers, uint32_t local_ssrc);
/* The connected peer that sends RTP from address:data_port, or -1. */
int applemidi_peers_find_data(const struct applemidi_peers *peers, uint32_t address,
                       uint16_t data_port);
/* The peer with this SSRC, or -1. */
int applemidi_peers_find_ssrc(const struct applemidi_peers *peers, uint32_t ssrc);

/* Records that the peer was heard from (CK, RTP). */
void applemidi_peers_seen(struct applemidi_peers *peers, int index, uint32_t now_ms);

/* Ends one peer: out is the BY to send on the local control port.
 * Returns -1 when the index is not in use. */
int applemidi_peers_end(struct applemidi_peers *peers, int index, struct applemidi_packet *out);

/* The first peer silent for timeout_ms, which the caller then ends with
 * applemidi_peers_end(), or -1. */
int applemidi_peers_expired(const struct applemidi_peers *peers, uint32_t now_ms,
                     uint32_t timeout_ms);

#endif
