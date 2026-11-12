/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * THE PACKET SERVICES MODULE, "packet" (net/packet_ops.h): the filter and
 * NAT engine and packet capture. A library file of its own,
 * packet.atcp, loaded when it is first needed - a filter or NAT
 * request, a bpf_open() - and never by a packet; in a monolithic program
 * (or while conf/group-packet.cfg puts it there) part of the program,
 * known by name and started the same way.
 *
 * It needs no protocol module: the engine hooks the packet points of
 * whichever families run (net/pfil.h), and a family started later is
 * hooked as it starts. On a stack with IPv6 alone it filters IPv6.
 *
 * Starting it: the engine takes the filter and NAT requests
 * (ipf_module_start()), the capture tap is set, and the capture calls'
 * table is put into the core's demand slot - last, so that nothing calls
 * in before the rest is ready.
 *
 * Stopping it is refused (EBUSY) while a capture handle is open, while the
 * filter is enabled, while it holds a filter or NAT rule: a firewall must
 * not vanish because a module was unloaded. The administrator disables and
 * flushes first (ipf -D -Fa, ipnat -C), closes the captures, then unloads.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/netmodule.h>
#include <net/pfil.h>
#include "bpf.h"
#include "ipfilter.h"
#include <net/packet_ops.h>

#include <net/netcore_map.h>        /* always the last include */

#ifdef __CONFIG_ROADSHOW__
static const struct packet_ops packet_ops_table = {
    sizeof(struct packet_ops),
    pkt_bpf_open,
    pkt_bpf_close,
    pkt_bpf_read,
    pkt_bpf_write,
    pkt_bpf_set_notify_mask,
    pkt_bpf_set_interrupt_mask,
    pkt_bpf_ioctl,
    pkt_bpf_data_waiting
};
#endif

static int
packet_start(struct netmodule *nm)
{
    int error;

    if((error = ipf_module_start(nm)) != 0)
        return (error);
    (void)nettap_set(bpf_mtap, bpf_mtap2);
#ifdef __CONFIG_ROADSHOW__
    (void)netdemand_set(PACKET_MODULE, &packet_ops_table);
#endif
    return (0);
}

/* 0, or why a stop would be refused; nothing is changed */
static int
packet_can_stop(struct netmodule *nm)
{
    if(bpf_busy() != 0) {
        __log(LOG_NOTICE, "packet cannot stop: a capture (bpf_open()) is open.");
        return (EBUSY);
    }
    if(ipf_module_busy()) {
        __log(LOG_NOTICE, "packet cannot stop: the filter is enabled or holds rules "
              "(ipf -D -Fa, ipnat -C first).");
        return (EBUSY);
    }
    return (0);
}

static int
packet_stop(struct netmodule *nm)
{
    int error;

    if((error = packet_can_stop(nm)) != 0)
        return (error);
    /* first what can still refuse: a task inside a capture call */
#ifdef __CONFIG_ROADSHOW__
    if((error = netdemand_set(PACKET_MODULE, NULL)) != 0)
        return (error);
#endif
    (void)nettap_set(NULL, NULL);
    ipf_module_stop();
    bpf_cleanup();
    return (0);
}

/*
 * WHEN THE STACK ENDS with the module running: it holds nothing that is
 * not the core's - rules, capture buffers and filter programs are memory
 * of the core's pool, freed in one piece; no device, library, port or
 * signal of its own. So nm_shutdown is NULL.
 */
NETMODULE_DESCRIPTOR(packet_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, PACKET_MODULE, 0,
    packet_start, packet_stop, packet_can_stop,
    0, NULL, NULL,
    NULL,           /* needs no other module */
    NULL,           /* test switch: ENV:AROSTCP/Without_packet, as every module by name */
    NULL,           /* nm_library: the loader's */
    NULL            /* nm_shutdown */
};
NETMODULE_BUILTIN(packet_module, 40)
