/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef APPLEMIDI_PORTS_H
#define APPLEMIDI_PORTS_H

#include <stddef.h>
#include <stdint.h>

/* Which port of a driver a session's peer is given. A port may be kept
 * for a peer by the name its invitation carries: that peer is given it
 * every time, so that what a user connected to "the iPad" finds it again
 * after it left and came back. A peer nobody named takes the first port
 * that is kept for no one and free. Portable C: no OS calls. */

#define APPLEMIDI_PORTS_MAX 16
#define APPLEMIDI_PORT_NAME_MAX 63

struct applemidi_ports {
    int count;                                      /* ports there are */
    char kept[APPLEMIDI_PORTS_MAX][APPLEMIDI_PORT_NAME_MAX + 1];
    int owner[APPLEMIDI_PORTS_MAX];                 /* 0 for none */
};

void applemidi_ports_init(struct applemidi_ports *ports, int count);
/* Keeps port for the peer called name. Returns -1 for a port that is not
 * there or is kept already, a name kept elsewhere, or one too long. */
int applemidi_ports_keep(struct applemidi_ports *ports, int port,
                         const char *name);
/* A peer has connected: owner is whatever the caller knows it by, not 0.
 * Returns its port, or -1 when the one kept for its name is taken by
 * another of that name, or none is free. */
int applemidi_ports_take(struct applemidi_ports *ports, const uint8_t *name,
                         size_t name_length, int owner);
/* The port owner has, or -1. */
int applemidi_ports_of(const struct applemidi_ports *ports, int owner);
/* The owner of port, or 0. */
int applemidi_ports_owner(const struct applemidi_ports *ports, int port);
void applemidi_ports_release(struct applemidi_ports *ports, int owner);

#endif
