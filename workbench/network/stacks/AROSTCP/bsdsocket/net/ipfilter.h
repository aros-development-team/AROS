/*
 * net/ipfilter.h - IP Filter (ipf) kernel API
 *
 * The rule structures, the flags and the state the requests report are
 * the public header's (common/include/net/ipfilter.h, shared with ipf,
 * ipnat and the firewall service); this adds what is the engine's alone.
 * The engine runs as hooks on the packet hook points (net/pfil.h).
 *
 * Copyright (C) 2026 The AROS Dev Team
 */

#ifndef AROSTCP_NET_IPFILTER_H
#define AROSTCP_NET_IPFILTER_H

#ifdef ENABLE_PACKET_FILTER

#include <sys/types.h>
#include <netinet/in.h>
#include <net/ipfilter.h>

struct mbuf;
struct ifnet;
struct netmodule;

/*
 * The packet services module (net/packet_module.c) starts and stops the
 * engine: its requests (the ioctl numbers of the filter and NAT, IPv6
 * rules included) are taken and given back. The engine hooks the packet
 * points only while it is enabled. ipf_module_busy(): whether the module
 * must not be stopped - the filter is enabled or holds a rule (filter or
 * NAT): an administrator disables and flushes first (ipf -D -Fa,
 * ipnat -C), so that a firewall does not vanish with a module.
 */
int	ipf_module_start(struct netmodule *nm);
void	ipf_module_stop(void);
int	ipf_module_busy(void);

int	ipf_enabled(void);
void	ipf_enable(void);
void	ipf_disable(void);
int	ipf_add_rule(struct ipf_rule *rule);
int	ipf_insert_rule(struct ipf_rule *rule);
int	ipf_remove_rule(struct ipf_rule *rule);
int	ipf_add_rule6(struct ipf_rule6 *rule, int insert);
int	ipf_remove_rule6(struct ipf_rule6 *rule);
int	ipf_flush(int flags);
int	ipf_check(struct mbuf *m, struct ifnet *ifp, int dir);
int	ipf_check6(struct mbuf *m, struct ifnet *ifp, int dir);

int	ipnat_add_rule(struct ipnat_rule *rule);
int	ipnat_remove_rule(struct ipnat_rule *rule);
int	ipnat_clear_rules(void);
int	ipnat_flush_entries(void);

/* Global state */
extern struct ipf_state ipf_global;
extern struct ipnat_state ipnat_global;

#endif /* ENABLE_PACKET_FILTER */

#endif /* AROSTCP_NET_IPFILTER_H */
