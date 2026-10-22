/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDID_H
#define NETMIDID_H

/* netmidid's core and what a transport is to it.
 *
 * The core has what every network MIDI protocol needs alike: the
 * configuration, the clock, the UDP sockets, the way to CAMD through the
 * drivers' ports, the counts it publishes and the system's mDNS service.
 * A transport is one protocol's sessions: AppleMIDI (RTP-MIDI), Network
 * MIDI 2.0. It asks the core for sockets and ports and is called from the
 * core's loop; no transport knows of another. The calls are not named
 * send or close: on AROS those are macros of bsdsocket.library. */

#include <netmidi/config.h>
#include <rtpmidi/ports.h>
#include "camd.h"
#include "discovery.h"
#include "network.h"

#include <stddef.h>
#include <stdint.h>

/* What each report begins with: the name of what is running. */
#define TAG "netmidid: "

/* The CAMD drivers whose ports peers are given, one to a transport; a
 * driver's ports are numbered from its number times NETMIDI_CAMD_PORTS. */
#define NETMIDID_APPLEMIDI 0
#define NETMIDID_NETWORKMIDI2 1

#define NETMIDID_TRANSPORTS 2

/* The longest SysEx message the core hands a transport; each gathers
 * what it receives into a buffer at least this long. */
#define NETMIDID_SYSEX_MAX 32768

struct netmidid_transport;

struct netmidid {
    struct netmidi_network_config config;
    char session_name[64];      /* the name all transports go by */
    /* Added to the time every received event is due, in ten-thousandths of
       a second: room for the network's unevenness, paid for in delay. */
    uint32_t latency;
    /* where every peer shares one pair of clusters: port 0 for all, and
       nothing made public */
    int shared_port;
    struct netmidi_network_runtime network;
    struct netmidi_camd_runtime camd;
    /* the ports of each driver, given to peers by name or as they come */
    struct applemidi_ports ports[NETMIDI_CAMD_DRIVERS];
    /* which transport reads each socket */
    const struct netmidid_transport *socket_owner[NETMIDI_NETWORK_SOCKETS];
    /* what the system's mDNS service advertises for us: kept across a
       change of the configuration, which begins the rest anew */
    struct netmidi_discovery *discovery;
    uint64_t published;         /* when the counts were last told */
};

/* A transport. Its state is state_size bytes the core gives it, zeroed,
 * for a run, and self in each call; it keeps nothing of its own between
 * runs. Times are in ten-thousandths of a second, as netmidid_now() gives
 * them. Each call but open may be NULL where the transport has nothing to
 * do there. */
struct netmidid_transport {
    size_t state_size;
    /* Begins from d->config and d->options: sockets, ports kept by name,
     * what it advertises and looks for. 0 when it runs, 1 when the
     * configuration does not ask for it, -1 when it cannot (said why). */
    int (*open)(struct netmidid *d, void *self);
    /* Every transport is open and the mDNS service asked: what it does
     * first. -1 ends the run. */
    int (*start)(struct netmidid *d, void *self, uint64_t now);
    /* A datagram waits on a socket it asked for. */
    void (*receive)(struct netmidid *d, void *self, int number);
    /* When the next event it holds is due; 0 when it holds none. */
    int (*next_due)(struct netmidid *d, void *self, uint64_t *due);
    /* A turn of the loop: timers, retries and what is due. 1 when the
     * program has done what it was asked (never in the service). */
    int (*periodic)(struct netmidid *d, void *self, uint64_t now);
    /* A message a CAMD client sent to port (whole, SysEx checked): 0 when
     * it went to someone, or will at flush. */
    int (*send_midi)(struct netmidid *d, void *self, int port, const uint8_t *message,
                     size_t length);
    /* CAMD has nothing more this turn: what was gathered goes out. */
    void (*flush)(struct netmidid *d, void *self);
    /* Writes the counts of the ports its peers have, counts being indexed
     * by port; returns the peers it refused. */
    unsigned long (*counts)(struct netmidid *d, void *self,
                            struct netmidi_port_counts *counts);
    /* The run is over: goodbyes, while the sockets are still open. */
    void (*stop)(struct netmidid *d, void *self);
    /* After the sockets closed: what it holds goes. */
    void (*release)(struct netmidid *d, void *self);
};

extern const struct netmidid_transport netmidid_applemidi;
extern const struct netmidid_transport netmidid_midi2;

uint64_t netmidid_now(void);
uint64_t netmidid_elapsed(uint64_t now, uint64_t then);
/* A value nobody can guess, for an SSRC or a token. */
uint32_t netmidid_random(uint64_t now);
/* An IPv4 address in its dotted form, to host order: 0, or -1. */
int netmidid_ipv4(const char *text, uint32_t *address);

/* A UDP socket on port, whose datagrams go to the transport: its number,
 * or -1. */
int netmidid_socket(struct netmidid *d, const struct netmidid_transport *t,
                    uint16_t port);

/* Port index of the driver is kept for the peer called name: 0, or -1
 * when there is no such port. */
int netmidid_port_keep(struct netmidid *d, int driver, int index,
                       const char *name);
/* A port of the driver for the peer called name, the one kept for it or
 * the first free and kept for no one, owner (not 0) being who holds it;
 * the port's number among all, or -1 for none. Where all share, 0. */
int netmidid_port_take(struct netmidid *d, int driver, const uint8_t *name,
                       size_t name_length, int owner);
void netmidid_port_release(struct netmidid *d, int driver, int owner);


#endif
