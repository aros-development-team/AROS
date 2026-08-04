/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * 6in4 (RFC 4213 / SIT) IPv6-in-IPv4 tunnel pseudo-interface.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NET_IF_STF_H
#define NET_IF_STF_H

#include <sys/types.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

struct ssconfig;	/* net/sana2config.h */

/*
 * Tunnel interface descriptor.  As with struct sana_softc, the visible
 * "struct ifnet" MUST be the first member so an ifnet* can be cast to a
 * stf_softc*.  The tunnel carries only the outer IPv4 endpoints; there is
 * no link-layer address (if_addrlen == 0).
 */
struct stf_softc {
    struct ifnet       sc_if;		/* network-visible interface (first!) */
    struct in_addr     sc_src;		/* outer IPv4 local endpoint  (TSRC) */
    struct in_addr     sc_dst;		/* outer IPv4 remote endpoint (TDST) */
    UBYTE              sc_ttl;		/* outer IPv4 TTL */
    UBYTE              sc_busy;		/* re-entrancy guard (loop protection) */
    struct route       sc_route;	/* cached route to sc_dst */
    UBYTE              sc_name[IFNAMSIZ];
    struct stf_softc  *sc_next;		/* list of tunnel interfaces */
};

extern struct stf_softc *stf_softc_list;

/* Create and attach a tunnel pseudo-interface (called from addifent). */
struct ifnet *stf_make(struct ssconfig *ifc, struct in_addr src,
                       struct in_addr dst, UBYTE ttl);

/* inetsw[] pr_input handler for inbound IP protocol 41 (decapsulation). */
void stf_input(void *arg, ...);

#endif /* NET_IF_STF_H */
