/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_PORTS_H
#define NETMIDI_PORTS_H

#include <stddef.h>
#include <stdint.h>

/* Which port of a driver a session's peer is given. A port may be kept
 * for a peer by its session name: that peer is given it
 * every time, so that what a user connected to "the iPad" finds it again
 * after it left and came back. A peer nobody named takes the first port
 * that is kept for no one and free. Portable C: no OS calls. */

#define NETMIDI_PORTS_MAX 16
#define NETMIDI_PORT_NAME_MAX 63

struct netmidi_ports {
    int count;                                      /* ports there are */
    char kept[NETMIDI_PORTS_MAX][NETMIDI_PORT_NAME_MAX + 1];
    int owner[NETMIDI_PORTS_MAX];                 /* 0 for none */
};

void netmidi_ports_init(struct netmidi_ports *ports, int count);
/* Keeps port for the peer called name. Returns -1 for a port that is not
 * there or is kept already, a name kept elsewhere, or one too long.
 * name must terminate within MAX + 1 readable bytes, or provide that many
 * bytes so an overlong name can be rejected. */
int netmidi_ports_keep(struct netmidi_ports *ports, int port,
                         const char *name);
/* A peer has connected: owner is whatever the caller knows it by, not 0.
 * Returns its port, or -1 when the one kept for its name is taken by
 * another of that name, or none is free. */
int netmidi_ports_take(struct netmidi_ports *ports, const uint8_t *name,
                         size_t name_length, int owner);
/* The port owner has, or -1. */
int netmidi_ports_of(const struct netmidi_ports *ports, int owner);
/* The owner of port, or 0. */
int netmidi_ports_owner(const struct netmidi_ports *ports, int port);
void netmidi_ports_release(struct netmidi_ports *ports, int owner);

#endif
