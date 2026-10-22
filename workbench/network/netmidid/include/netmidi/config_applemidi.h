/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_CONFIG_APPLEMIDI_H
#define NETMIDI_CONFIG_APPLEMIDI_H

#include <stdint.h>
#include <stdio.h>

/* AppleMIDI's keys of the network MIDI configuration. Ports are control
 * ports; the data socket uses the following port. */
struct netmidi_applemidi_config {
    uint16_t local_port;        /* "local_port" */
    /* "peer_ip" and "peer_port": a peer to invite */
    uint16_t peer_port;
    char peer_ip[64];
    /* "applemidi.port.<n> = <name>": port n of the driver applemidi is
     * kept for the peer whose session has that name. */
    char port_name[16][64];
    /* "max_peers": peers at once, 1 to 8; 0 for all there is room for */
    int max_peers;
    /* "accept": anyone, nobody, or a list of addresses and session names
     * with commas between. */
    char accept[256];
};

void netmidi_applemidi_config_defaults(struct netmidi_applemidi_config *c);
/* One "key = value" line: 1 when the key is AppleMIDI's and its value
 * good, 0 when the key is not AppleMIDI's, -1 when the value is bad. */
int netmidi_applemidi_config_key(struct netmidi_applemidi_config *c,
                                 const char *key, const char *value);
/* After the whole file: 0, or -1 when the keys disagree. */
int netmidi_applemidi_config_check(const struct netmidi_applemidi_config *c);
void netmidi_applemidi_config_save(FILE *file,
                                   const struct netmidi_applemidi_config *c);

#endif
