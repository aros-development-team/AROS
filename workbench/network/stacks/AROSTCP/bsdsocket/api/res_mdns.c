/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Names in .local, of the resolver module (api/resolver_ops.h), by
 * multicast DNS: a .local name is the links' (RFC 6762 section 3), never
 * the DNS's. mdnsd holds what the links say, and dnssd.library is how one
 * asks it; the resolver is one more of its clients.
 *
 * The stack does not depend on mDNS, to build or to run. The library is
 * found by its name when a look-up needs it, opened for the look-up and
 * closed after it, in the calling task, as all of the resolver runs: no
 * state is shared between tasks and nothing is left to close when the
 * module stops. Without dnssd.library, or with no mdnsd running, a .local
 * name is not found, and nothing else changes. Nothing of mDNS is needed
 * to build it either: the library's public header, <dnssd/dnssd.h>, is
 * one of the network's common includes.
 *
 * mdnsd never resolves a name through this resolver, so there is no loop.
 */

#include <conf.h>

#include <exec/types.h>
#include <aros/libcall.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/nameser.h>         /* MAXDNAME */
#include <string.h>

#include <kern/amiga_includes.h>
#include <api/amiga_api.h>
#include <api/res_mdns.h>

#include <net/netcore_map.h>        /* always the last include */

/*
 * How long a look-up waits for an answer: the whole of it, from the
 * question to the last address, in seconds (DNSSD_Timeout). A host that is
 * there answers in a fraction of a second, and the look-up ends a quarter
 * of a second after the first address; the time is spent in full only for
 * a name no host has.
 */
#define RES_MDNS_TIMEOUT    3

/*
 * DNSSD_LookupAddressSyncA(), vector 10 of dnssd.library (its dnssd.conf):
 * the library's <proto/dnssd.h> is made when the library is built, which
 * the stack does not wait for.
 */
static LONG
lookup_address_sync(struct Library *DNSSDBase, CONST_STRPTR name,
    struct DNSSDAddress *addresses, ULONG *count, struct TagItem *tags)
{
    return AROS_LC4(LONG, DNSSD_LookupAddressSyncA,
        AROS_LCA(CONST_STRPTR, name, A0),
        AROS_LCA(struct DNSSDAddress *, addresses, A1),
        AROS_LCA(ULONG *, count, A2),
        AROS_LCA(struct TagItem *, tags, A3),
        struct Library *, DNSSDBase, 10, DNSSD);
}

int
res_mdns_name(const char *name)
{
    size_t len = strnlen(name, MAXDNAME + 2);

    if (len > MAXDNAME + 1)
        return (0);                     /* no name: the DNS's to refuse */
    if (len > 0 && name[len - 1] == '.')
        len--;
    /* "x.local" at least: a label, its dot, "local" */
    return (len > 6 && name[len - 6] == '.' &&
            strncasecmp(name + len - 5, "local", 5) == 0 &&
            name[len - 7] != '.');
}

int
res_mdns_linklocal6(const struct DNSSDAddress *a)
{
    return (a->da_Family == DNSSD_FAMILY_IPV6 &&
            a->da_Bytes[0] == 0xfe && (a->da_Bytes[1] & 0xc0) == 0x80);
}

int
res_mdns_lookup(struct SocketBase *libPtr, const char *name, int family,
    struct DNSSDAddress *out, int *herr)
{
    struct Library *dnssdbase;
    struct TagItem tags[3];
    ULONG count = RES_MDNS_ADDRESSES, i;
    LONG rc;
    int n = 0;

    if ((dnssdbase = OpenLibrary((CONST_STRPTR)DNSSDNAME, DNSSD_VERSION)) == NULL) {
        *herr = HOST_NOT_FOUND;         /* no mDNS on this system */
        return (0);
    }
    tags[0].ti_Tag = DNSSD_Family;
    tags[0].ti_Data = family == AF_INET ? DNSSD_FAMILY_IPV4 :
                      family == AF_INET6 ? DNSSD_FAMILY_IPV6 : DNSSD_FAMILY_ANY;
    tags[1].ti_Tag = DNSSD_Timeout;
    tags[1].ti_Data = RES_MDNS_TIMEOUT;
    tags[2].ti_Tag = TAG_DONE;
    rc = lookup_address_sync(dnssdbase, (CONST_STRPTR)name, out, &count, tags);
    CloseLibrary(dnssdbase);

    switch (rc) {
    case DNSSD_OK:
        break;
    case DNSSD_ERR_NOENGINE:            /* mdnsd does not run */
    case DNSSD_ERR_BUSY:                /* mdnsd has no room for it now */
    case DNSSD_ERR_STALE:               /* mdnsd restarted during it */
        *herr = TRY_AGAIN;
        return (0);
    case DNSSD_ERR_CANCELLED:           /* broken off, as a DNS query is */
        writeErrnoValue(libPtr, EINTR);
        *herr = TRY_AGAIN;
        return (0);
    case DNSSD_ERR_NOMEM:
        writeErrnoValue(libPtr, ENOMEM);
        *herr = NETDB_INTERNAL;
        return (0);
    default:                            /* nobody answered in time; a bad name */
        *herr = HOST_NOT_FOUND;
        return (0);
    }

    /* kept in place: those of the family asked, each once - an address
     * learned on two links is one, but for an IPv6 link-local one, whose
     * zone tells them apart */
    for (i = 0; i < count && i < RES_MDNS_ADDRESSES; i++) {
        const struct DNSSDAddress *a = &out[i];
        int af = a->da_Family == DNSSD_FAMILY_IPV4 ? AF_INET :
                 a->da_Family == DNSSD_FAMILY_IPV6 ? AF_INET6 : AF_UNSPEC;
        int j;

        if (af == AF_UNSPEC || a->da_Length != (af == AF_INET ? 4 : 16) ||
            (family != AF_UNSPEC && af != family))
            continue;
        for (j = 0; j < n; j++)
            if (out[j].da_Family == a->da_Family &&
                memcmp(out[j].da_Bytes, a->da_Bytes, a->da_Length) == 0 &&
                (!res_mdns_linklocal6(a) || out[j].da_Interface == a->da_Interface))
                break;
        if (j < n)
            continue;
        if (n != (int)i)
            out[n] = *a;
        n++;
    }
    if (n == 0)
        *herr = NO_DATA;                /* answered, but no address of the family */
    return (n);
}
