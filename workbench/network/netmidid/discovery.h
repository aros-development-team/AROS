/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_DISCOVERY_H
#define NETMIDI_DISCOVERY_H

#include <stdint.h>

/* Our name on the network, through the system's mDNS service:
 * bonami.library and mdnsd. We hold no socket on port 5353 and answer no
 * question ourselves; the service does, for every program on the machine.
 * Where there is no such service nothing is advertised and the calls say
 * so. */

#define NETMIDI_DISCOVERY_SERVICES 2
#define NETMIDI_DISCOVERY_NAME 64   /* one DNS label and its end */
#define NETMIDI_DISCOVERY_TYPE 32

struct netmidi_discovery {
    void *library;                  /* bonami.library, opened */
    int engine;                     /* the service answered */
    struct {
        char name[NETMIDI_DISCOVERY_NAME];
        char type[NETMIDI_DISCOVERY_TYPE];
        uint16_t port;
        int wanted;                 /* asked for since the last begin */
        long refused;               /* why it was last refused, or 0 */
        void *reference;            /* the registration, or NULL */
    } service[NETMIDI_DISCOVERY_SERVICES];
    int count;
    uint64_t last_attempt;
};

/* Once, before anything else. */
void netmidi_discovery_init(struct netmidi_discovery *discovery);
/* Before the services are asked for again, as when the configuration
 * changed: one asked for again as it was keeps its registration, and the
 * network sees no change; the others are withdrawn at the next poll. */
void netmidi_discovery_begin(struct netmidi_discovery *discovery);
/* Asks for an instance of type on port to be advertised under name. Both
 * are copied. Nothing is sent here. */
int netmidi_discovery_add(struct netmidi_discovery *discovery,
                          const char *name, const char *type, uint16_t port);
/* Withdraws what is no longer asked for and registers what is not yet
 * registered. The service may start after the program, or stop and start
 * again with the network, so this is called regularly; it tries at most
 * once in ten seconds. now is in ten-thousandths of a second. Returns the
 * number of services registered now, -1 where there is no service to
 * ask. */
int netmidi_discovery_poll(struct netmidi_discovery *discovery,
                           uint64_t now);
/* Withdraws everything, with the goodbye the service sends for it. */
void netmidi_discovery_close(struct netmidi_discovery *discovery);

#endif
