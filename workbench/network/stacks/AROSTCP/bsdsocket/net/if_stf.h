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
    ULONG              sc_seen;		/* reload generation its line was last read in */
};

extern struct stf_softc *stf_softc_list;

/*
 * The tunnel makes itself known to the core as a kind of tunnel interface
 * (struct nettunnel, net/netmodule.h); called by IPv6's module start.  A
 * tunnel is made through that registration (nettunnel_find()), not by
 * calling this file.
 */
void stf_attach(void);
/* ... and takes itself back, with every tunnel interface, when IPv6 stops */
void stf_detach(void);

/* inetsw[] pr_input handler for inbound IP protocol 41 (decapsulation). */
void stf_input(void *arg, ...);

#endif /* NET_IF_STF_H */
