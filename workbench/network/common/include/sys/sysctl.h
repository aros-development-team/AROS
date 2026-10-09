#ifndef SYS_SYSCTL_H
#define SYS_SYSCTL_H
/*
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * Minimal 4.4BSD-style sysctl interface for AROSTCP.  It backs the
 * miami.library MiamiSysCtl() call and the per-protocol *_sysctl() handlers
 * (tcp_sysctl/udp_sysctl in netinet, ip_sysctl in kern/kern_sysctl.c).  Only
 * the net.inet.{ip,tcp,udp} subtree is served; it is not a general kernel
 * MIB.  Built only when ENABLE_SYSCTL is defined.
 */

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

/*
 * Top-level identifiers (CTL_*), as in 4.4BSD.  Only CTL_NET is served.
 */
#define CTL_UNSPEC      0
#define CTL_KERN        1
#define CTL_VM          2
#define CTL_FS          3
#define CTL_NET         4
#define CTL_DEBUG       5
#define CTL_HW          6
#define CTL_MACHDEP     7
#define CTL_MAXID       8

/*
 * Second-level names under CTL_NET are protocol families (PF_INET, from
 * <sys/socket.h>); third-level names are protocol numbers (IPPROTO_*, from
 * <netinet/in.h>); the fourth level is the per-protocol leaf below.
 */

/* net.inet.ip (IPPROTO_IP) leaves */
#define IPCTL_FORWARDING        1       /* act as router */
#define IPCTL_SENDREDIRECTS     2       /* may send redirects when forwarding */
#define IPCTL_DEFTTL            3       /* default TTL */
#define IPCTL_STATS             4       /* ipstat structure (read-only) */
#define IPCTL_MAXID             5

/* net.inet.tcp (TCPCTL_*) leaves live in <netinet/tcp_var.h>. */
/* net.inet.udp (UDPCTL_*) leaves live in <netinet/udp_var.h>. */

/*
 * Helper routines (kern/kern_sysctl.c).  Each copies a kernel value out to
 * oldp (honouring *oldlenp) and optionally stores a new value from newp.
 * They return 0 or a BSD error number.
 */
int sysctl_int(void *oldp, size_t *oldlenp, void *newp, size_t newlen, int *valp);
int sysctl_rdint(void *oldp, size_t *oldlenp, void *newp, int val);
int sysctl_rdstruct(void *oldp, size_t *oldlenp, void *newp, void *sp, int len);

/*
 * Dispatchers / per-protocol handlers.  name/namelen are the remaining MIB
 * path at each level.
 */
int kern_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
                void *newp, size_t newlen);
int net_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
               void *newp, size_t newlen);
int ip_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
              void *newp, size_t newlen);
int tcp_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
               void *newp, size_t newlen);
int udp_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
               void *newp, size_t newlen);

#endif /* SYS_SYSCTL_H */
