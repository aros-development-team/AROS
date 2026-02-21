/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 implementation:
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * in6_proto.c - IPv6 protocol domain and switch table.
 *
 * Defines the inet6 domain and the inet6sw[] protocol switch,
 * registering ICMPv6, UDP-over-IPv6, TCP-over-IPv6 and raw IPv6.
 *
 * Forward declarations use K&R unprototyped form (matching the pattern
 * used by netinet/in_proto.c) so that function pointer assignments into
 * the generic void/int (*)(void *, ...) protosw fields compile cleanly.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

/* --- ANSI function declarations matching actual signatures ---
 * pr_input  must be void (*)(void *args, ...)
 * pr_output must be int  (*)(void *args, ...)
 * pr_init / pr_fasttimo / pr_slowtimo / pr_drain: void (*)(void)
 */
void ip6_init(void);
void icmp6_input(void *args, ...);
void icmp6_init(void);
void icmp6_fasttimo(void);
void icmp6_slowtimo(void);
void icmp6_drain(void);

int  ip6_output(void *args, ...);
void rip6_input(void *args, ...);
int  rip6_output(void *args, ...);
int  rip6_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
                 struct mbuf *);

/* TCP/UDP shared with IPv4 */
#include <netinet/tcp_usrreq_protos.h>
#include <netinet/udp_usrreq_protos.h>
void tcp_input(void *args, ...);
#include <netinet/ip_input_protos.h>

/* ctloutput stubs: old BSD 5-arg signature */
int rip6_ctloutput(int op, struct socket *so, int level, int optname,
                   struct mbuf **m);
int icmp6_ctloutput(int op, struct socket *so, int level, int optname,
                    struct mbuf **m);
int ip6_ctloutput(int op, struct socket *so, int level, int optname,
                  struct mbuf **m);

extern struct domain inet6domain;

struct protosw inet6sw[] = {
    /* base IPv6 entry: handles outbound path and timers */
    { 0,            &inet6domain,   0,              0,
      NULL,         ip6_output,     NULL,           NULL,
      NULL,
      ip6_init,     NULL,           NULL,           NULL,
    },
    /* UDP over IPv6 */
    { SOCK_DGRAM,   &inet6domain,   IPPROTO_UDP,    PR_ATOMIC|PR_ADDR,
      udp_input,
      NULL,
      udp_ctlinput,
      ip6_ctloutput,
      udp_usrreq,
      NULL,         NULL,           NULL,           NULL,
    },
    /* TCP over IPv6 */
    { SOCK_STREAM,  &inet6domain,   IPPROTO_TCP,
      PR_CONNREQUIRED|PR_IMPLOPCL|PR_WANTRCVD,
      tcp_input,    NULL,           tcp_ctlinput,   tcp_ctloutput,
      tcp_usrreq,
      NULL,         tcp_fasttimo,   tcp_slowtimo,   tcp_drain,
    },
    /* ICMPv6 */
    { SOCK_RAW,     &inet6domain,   IPPROTO_ICMPV6, PR_ATOMIC|PR_ADDR,
      icmp6_input,
      rip6_output,
      NULL,
      icmp6_ctloutput,
      rip6_usrreq,
      icmp6_init,   icmp6_fasttimo, icmp6_slowtimo, icmp6_drain,
    },
    /* raw IPv6 */
    { SOCK_RAW,     &inet6domain,   IPPROTO_RAW,    PR_ATOMIC|PR_ADDR,
      rip6_input,   rip6_output,    NULL,           rip6_ctloutput,
      rip6_usrreq,
      NULL,         NULL,           NULL,           NULL,
    },
    /* raw IPv6 wildcard */
    { SOCK_RAW,     &inet6domain,   0,              PR_ATOMIC|PR_ADDR,
      rip6_input,   rip6_output,    NULL,           rip6_ctloutput,
      rip6_usrreq,
      NULL,         NULL,           NULL,           NULL,
    },
};

struct domain inet6domain = {
    AF_INET6, "internet6", NULL, NULL, NULL,
    inet6sw, &inet6sw[sizeof(inet6sw)/sizeof(inet6sw[0])], NULL
};

/*
 * ip6_ctloutput - SOL_IPV6 socket option handler.
 * Minimal stub: returns EINVAL for unknown options.
 */
int
ip6_ctloutput(int op, struct socket *so, int level, int optname,
              struct mbuf **m)
{
    if (level != IPPROTO_IPV6)
        return EINVAL;
    return 0;
}


#endif /* INET6 */
