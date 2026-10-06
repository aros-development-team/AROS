/*
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Minimal 4.4BSD-style sysctl framework for AROSTCP.  It backs the
 * miami.library MiamiSysCtl() entry point and wires the per-protocol
 * handlers (ip_sysctl here; tcp_sysctl/udp_sysctl in netinet) into the
 * net.inet.{ip,tcp,udp} MIB subtree.  This is deliberately a small MIB, not
 * a general kernel sysctl: there is no CTL_KERN/CTL_HW tree and no PF_ROUTE
 * table dump.  Built only when ENABLE_SYSCTL is defined.
 */

#include <conf.h>

#ifdef ENABLE_SYSCTL

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <string.h>

/*
 * Copy the kernel int at *valp out to oldp (if any) and, if newp is given,
 * store a new int into *valp.  Standard 4.4BSD semantics.
 */
int
sysctl_int(void *oldp, size_t *oldlenp, void *newp, size_t newlen, int *valp)
{
    int error = 0;

    if(oldp && *oldlenp < sizeof(int))
        return (ENOMEM);
    if(newp && newlen != sizeof(int))
        return (EINVAL);
    *oldlenp = sizeof(int);
    if(oldp)
        bcopy(valp, oldp, sizeof(int));
    if(newp)
        bcopy(newp, valp, sizeof(int));
    return (error);
}

/*
 * As sysctl_int(), but the value is read-only: a write attempt fails EPERM.
 */
int
sysctl_rdint(void *oldp, size_t *oldlenp, void *newp, int val)
{
    if(oldp && *oldlenp < sizeof(int))
        return (ENOMEM);
    if(newp)
        return (EPERM);
    *oldlenp = sizeof(int);
    if(oldp)
        bcopy(&val, oldp, sizeof(int));
    return (0);
}

/*
 * Copy a read-only kernel structure of len bytes out to oldp.
 */
int
sysctl_rdstruct(void *oldp, size_t *oldlenp, void *newp, void *sp, int len)
{
    if(oldp && *oldlenp < (size_t)len)
        return (ENOMEM);
    if(newp)
        return (EPERM);
    *oldlenp = len;
    if(oldp)
        bcopy(sp, oldp, len);
    return (0);
}

/*
 * net.inet.ip
 */
int
ip_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
          void *newp, size_t newlen)
{
    extern int ipforwarding;
    extern int ipsendredirects;

    /* All sysctl names at this level are terminal. */
    if(namelen != 1)
        return (ENOTDIR);

    switch(name[0]) {
    case IPCTL_FORWARDING:
        return (sysctl_int(oldp, oldlenp, newp, newlen, &ipforwarding));
    case IPCTL_SENDREDIRECTS:
        return (sysctl_int(oldp, oldlenp, newp, newlen, &ipsendredirects));
    case IPCTL_STATS:
        return (sysctl_rdstruct(oldp, oldlenp, newp, &ipstat, sizeof ipstat));
    default:
        return (ENOPROTOOPT);
    }
    /* NOTREACHED */
}

/*
 * net.<family>.<protocol>...  Only the AF_INET protocols are served.
 */
int
net_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
           void *newp, size_t newlen)
{
    /* Need at least {family, protocol}. */
    if(namelen < 2)
        return (EISDIR);
    if(name[0] != PF_INET)
        return (EOPNOTSUPP);

    switch(name[1]) {
    case IPPROTO_IP:
        return (ip_sysctl(name + 2, namelen - 2, oldp, oldlenp, newp, newlen));
    case IPPROTO_TCP:
        return (tcp_sysctl(name + 2, namelen - 2, oldp, oldlenp, newp, newlen));
    case IPPROTO_UDP:
        return (udp_sysctl(name + 2, namelen - 2, oldp, oldlenp, newp, newlen));
    default:
        return (ENOPROTOOPT);
    }
    /* NOTREACHED */
}

/*
 * Top-level dispatch.  Only CTL_NET is served.
 */
int
kern_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
            void *newp, size_t newlen)
{
    if(namelen < 1)
        return (EINVAL);

    switch(name[0]) {
    case CTL_NET:
        return (net_sysctl(name + 1, namelen - 1, oldp, oldlenp, newp, newlen));
    default:
        return (EOPNOTSUPP);
    }
    /* NOTREACHED */
}

#endif /* ENABLE_SYSCTL */
