/*
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Minimal 4.4BSD-style sysctl framework for AROSTCP.  It backs the
 * miami.library MiamiSysCtl() entry point and wires the variables the
 * modules register with their sysctl names (net/netmodule.h: IPv4's
 * net.inet.ip, the transports' net.inet.tcp and net.inet.udp, a loaded
 * module's) into the net MIB subtree.  This is deliberately a small MIB, not
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
#include <net/netmodule.h>

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
 * net.<family>.<protocol>.<id>
 *
 * Every name is a variable a module registered under it (net/netmodule.h):
 * a family's, a transport's, a loaded module's. The core does not know
 * which there are. When the module is not part of the running stack the
 * name is unknown, like any other that does not exist.
 */
/* (NVT_SYSCTL, net/netmodule.h: nv_addr is a function that serves the name itself) */
typedef int netvar_sysctl_t(const struct netvar *, void *oldp, size_t *oldlenp,
                            void *newp, size_t newlen);

int
net_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
           void *newp, size_t newlen)
{
    const struct netvar *nv;
    int known;

    /* Need at least {family, protocol}. */
    if(namelen < 2)
        return (EISDIR);

    nv = netvar_find_mib(name[0], name[1], namelen == 3 ? name[2] : -1, &known);
    if(!known)
        return (name[0] == PF_INET ? ENOPROTOOPT : EOPNOTSUPP);
    /* All sysctl names at this level are terminal. */
    if(namelen != 3)
        return (ENOTDIR);
    if(nv == NULL)
        return (ENOPROTOOPT);

    switch(nv->nv_type) {
    case NVT_INT:
        if(nv->nv_size != sizeof(int))
            break;
        return (sysctl_int(oldp, oldlenp, newp, newlen, (int *)nv->nv_addr));
    case NVT_STATS:
        return (sysctl_rdstruct(oldp, oldlenp, newp, nv->nv_addr, (int)nv->nv_size));
    case NVT_SYSCTL:
        return ((*(netvar_sysctl_t *)nv->nv_addr)(nv, oldp, oldlenp, newp, newlen));
    }
    return (ENOPROTOOPT);
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
