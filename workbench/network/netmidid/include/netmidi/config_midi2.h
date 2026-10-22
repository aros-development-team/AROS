/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_CONFIG_MIDI2_H
#define NETMIDI_CONFIG_MIDI2_H

#include <stdint.h>
#include <stdio.h>

/* Network MIDI 2.0's keys of the network MIDI configuration. */
struct netmidi_midi2_config {
    /* "midi2_port": the UDP port Network MIDI 2.0 is answered on; without
     * it there is none */
    uint16_t port;
    /* A Network MIDI 2.0 Host to invite, as a Client: at "midi2_peer_ip"
     * and "midi2_peer_port". It needs midi2_port. */
    char peer_ip[64];
    uint16_t peer_port;
    /* "networkmidi2.port.<n> = <name>": port n of the driver networkmidi2
     * is kept for the peer whose endpoint has that name. */
    char port_name[16][64];
};

void netmidi_midi2_config_defaults(struct netmidi_midi2_config *c);
/* One "key = value" line: 1 when the key is Network MIDI 2.0's and its
 * value good, 0 when the key is not its, -1 when the value is bad. */
int netmidi_midi2_config_key(struct netmidi_midi2_config *c,
                             const char *key, const char *value);
/* After the whole file: 0, or -1 when the keys disagree. */
int netmidi_midi2_config_check(const struct netmidi_midi2_config *c);
void netmidi_midi2_config_save(FILE *file,
                               const struct netmidi_midi2_config *c);

#endif
