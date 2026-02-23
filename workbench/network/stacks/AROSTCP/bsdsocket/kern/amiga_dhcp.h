#ifndef KERN_AMIGA_DHCP_H
#define KERN_AMIGA_DHCP_H

/*
 * DHCP client state for a single protocol family.
 * One instance is defined in netinet/in_proto.c  (IPv4, aros_dhcpv4)
 * and one in netinet6/in6_proto.c               (IPv6, aros_dhcpv6).
 * args holds the combined argument string for all interfaces needing DHCP,
 * e.g. "-q net0 net1" — large enough for several interface names.
 */
#define AROS_DHCP_ARGS_LEN  256

struct aros_dhcp_state {
    pid_t   pid;                        /* PID of running dhclient, or NULL  */
    char    args[AROS_DHCP_ARGS_LEN];  /* argument string passed to process */
};

extern struct aros_dhcp_state aros_dhcpv4;
#if INET6 && DHCP6
extern struct aros_dhcp_state aros_dhcpv6;
#endif

void run_dhclient(struct ifnet *ifp);
void kill_dhclient(struct ifnet *ifp);
void run_dhcp(void);

#if INET6 && DHCP6
void run_dhclient6(struct ifnet *ifp);
void kill_dhclient6(struct ifnet *ifp);
#endif

#endif /* KERN_AMIGA_DHCP_H */

