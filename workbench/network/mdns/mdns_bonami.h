/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_bonami.h - bonami.library's port into mdnsd, until it goes
 *
 * bonami.library's clients send struct MDNSIPCMsg (mdns_ipc.h) to the
 * daemon's MDNS_IPC_PORT. mdns_client.c takes them: a registration is made
 * a canonical one (dnssd_reg.h) for mdns_reg.c, which knows nothing of
 * Bonami; browsing and record questions keep their own queues
 * (mdns_discovery.c, mdns_query.c). Only the files of this port and the
 * daemon's loop (mdnsd.c) include this header.
 */
#ifndef MDNSD_MDNS_BONAMI_H
#define MDNSD_MDNS_BONAMI_H

#include <bonami/mdns.h>

#include "mdnsd.h"
#include "mdns_ipc.h"

/* mdns_client.c */
void mdnsClientMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
void mdnsClientReap(struct MDNSDaemon *md);

/* mdns_discovery.c: browsing and resolving, with their event queues. */
LONG mdnsDiscoveryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
void mdnsDiscoveryReap(struct MDNSDaemon *md);
void mdnsDiscoveryTick(struct MDNSDaemon *md);
void mdnsDiscoveryReset(struct MDNSDaemon *md);
void mdnsDiscoveryRelease(struct MDNSDaemon *md, ULONG taskID);
void mdnsDiscoveryCancel(struct MDNSDaemon *md);
void mdnsDiscoveryFree(struct MDNSDaemon *md);

/* mdns_query.c: generic record questions and their event queues. */
LONG mdnsQueryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg);
void mdnsQueryTick(struct MDNSDaemon *md);
void mdnsQueryReset(struct MDNSDaemon *md);
void mdnsQueryRelease(struct MDNSDaemon *md, ULONG owner);
void mdnsQueryReap(struct MDNSDaemon *md);
void mdnsQueryCancel(struct MDNSDaemon *md);
void mdnsQueryFree(struct MDNSDaemon *md);

#endif /* MDNSD_MDNS_BONAMI_H */
