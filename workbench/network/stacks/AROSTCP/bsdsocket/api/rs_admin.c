/*
 * Copyright (C) 2005-2026, The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * bsdsocket.library: the route, interface and name-server calls of the
 * Roadshow interface that do work (vectors 69-71, 73, 77, 86, 87, 89,
 * 117), in the administration module (kern/admin_ops.h). Each is the C
 * function behind its vector, with the vector's arguments and the base
 * last; the vector itself is a stub of the core (kern/admin_stubs.c),
 * which finds these through the module's table. So are the calls that
 * give a list back and those that are not implemented: they need nothing
 * of this file.
 *
 * Moved from api/amiga_roadshow.c, otherwise unchanged but for the
 * formatting of numbers, which the core's ultoa() and if_fullname() do
 * instead of snprintf() (a module does not carry the C library's
 * formatter).
 */

#include <conf.h>

#ifdef __CONFIG_ROADSHOW__

#include <aros/libcall.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <libraries/bsdsocket.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/synch.h>
#include <netinet/in.h>
#include <net/route.h>
#include <net/if.h>
#include <net/radix.h>
#include <net/if_sana.h>
#include <net/netmodule.h>
#include <sys/ioctl.h>
#include <protos/net/route_protos.h>
#include <syslog.h>
#include <api/amiga_api.h>
#include <api/apicalls.h>
#include <kern/amiga_netdb.h>

#include <string.h>

#include <netdb.h>

#include <sys/errno.h>

#include <net/netcore.h>            /* the core's names, as the module's table has them */
#include <kern/admin_ops.h>

#include <net/netcore_map.h>        /* always the last include */

/*
 * Helper: fill a sockaddr_in from a network-byte-order IPv4 address.
 */
static inline void
fill_sockaddr_in(struct sockaddr_in *sa, ULONG addr)
{
    memset(sa, 0, sizeof(*sa));
    sa->sin_family = AF_INET;
    sa->sin_len    = sizeof(struct sockaddr_in);
    sa->sin_addr.s_addr = addr;
}

/*
 * Parse RTA_* route tags into destination, gateway, netmask, and flags.
 * Returns 0 on success, or an errno value on failure.
 */
static int
parse_route_tags(struct TagItem *tags, ULONG *dst_out, ULONG *gw_out,
                 ULONG *mask_out, ULONG *flags_out)
{
    struct TagItem *tag;
    struct TagItem *tstate = tags;
    ULONG dst = 0, gw = 0, mask = 0;
    ULONG flags = RTF_UP | RTF_STATIC;
    int have_dst = 0;

    if(tags == NULL)
        return EINVAL;

    while((tag = NextTagItem(&tstate)) != NULL) {
        switch(tag->ti_Tag) {
        case RTA_Destination:
            dst = (ULONG)tag->ti_Data;
            have_dst = 1;
            break;
        case RTA_DestinationHost:
            dst = (ULONG)tag->ti_Data;
            flags |= RTF_HOST;
            mask = 0xFFFFFFFF;
            have_dst = 1;
            break;
        case RTA_DestinationNet:
            dst = (ULONG)tag->ti_Data;
            flags &= ~RTF_HOST;
            /* Derive classful netmask from destination address */
            {
                ULONG ha = ntohl(dst);
                if((ha & 0x80000000) == 0)
                    mask = htonl(0xFF000000);       /* Class A */
                else if((ha & 0xC0000000) == 0x80000000)
                    mask = htonl(0xFFFF0000);       /* Class B */
                else
                    mask = htonl(0xFFFFFF00);       /* Class C */
            }
            have_dst = 1;
            break;
        case RTA_Gateway:
            gw = (ULONG)tag->ti_Data;
            if(gw != 0)
                flags |= RTF_GATEWAY;
            break;
        case RTA_DefaultGateway:
            dst = INADDR_ANY;
            mask = INADDR_ANY;
            gw = (ULONG)tag->ti_Data;
            flags |= RTF_GATEWAY;
            flags &= ~RTF_HOST;
            have_dst = 1;
            break;
        }
    }

    if(!have_dst)
        return EINVAL;

    /* Default to host route if no mask specified and not a default route */
    if(!(flags & RTF_HOST) && mask == 0 && dst != INADDR_ANY) {
        flags |= RTF_HOST;
        mask = 0xFFFFFFFF;
    }

    *dst_out   = dst;
    *gw_out    = gw;
    *mask_out  = mask;
    *flags_out = flags;
    return 0;
}

/* Format an in_addr as a dotted-decimal string into buf (must be >= 16 bytes) */
static void fmt_ipaddr(char *buf, struct in_addr addr)
{
    unsigned char *p = (unsigned char *)&addr.s_addr;
    int i;

    for(i = 0; i < 4; i++) {
        buf += ultoa(p[i], buf);
        *buf++ = (i < 3) ? '.' : '\0';
    }
}

long
rsadm_AddRouteTagList(struct TagItem *tags, struct SocketBase *libPtr)
{
    struct sockaddr_in dst, gw, mask;
    ULONG dst_addr, gw_addr, mask_addr, flags;
    int error;

    error = parse_route_tags(tags, &dst_addr, &gw_addr, &mask_addr, &flags);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    fill_sockaddr_in(&dst, dst_addr);
    fill_sockaddr_in(&gw,  gw_addr);
    fill_sockaddr_in(&mask, mask_addr);

    error = rtrequest(RTM_ADD,
                      (struct sockaddr *)&dst,
                      (struct sockaddr *)&gw,
                      (struct sockaddr *)&mask,
                      (int)flags, NULL);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;
}

long
rsadm_DeleteRouteTagList(struct TagItem *tags, struct SocketBase *libPtr)
{
    struct sockaddr_in dst, mask;
    ULONG dst_addr, gw_addr, mask_addr, flags;
    int error;

    error = parse_route_tags(tags, &dst_addr, &gw_addr, &mask_addr, &flags);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    fill_sockaddr_in(&dst, dst_addr);
    fill_sockaddr_in(&mask, mask_addr);

    error = rtrequest(RTM_DELETE,
                      (struct sockaddr *)&dst,
                      NULL,
                      (struct sockaddr *)&mask,
                      0, NULL);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;
}

long
rsadm_ChangeRouteTagList(struct TagItem *tags, struct SocketBase *libPtr)
{
    struct sockaddr_in dst, gw, mask;
    ULONG dst_addr, gw_addr, mask_addr, flags;
    int error;

    error = parse_route_tags(tags, &dst_addr, &gw_addr, &mask_addr, &flags);
    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }

    fill_sockaddr_in(&dst, dst_addr);
    fill_sockaddr_in(&gw,  gw_addr);
    fill_sockaddr_in(&mask, mask_addr);

    /*
     * RTM_CHANGE is not supported by rtrequest(), so we perform
     * an atomic delete + add under a single splnet section.
     */
    {
        spl_t s = splnet();

        /* Delete the existing route (ignore ESRCH — it may not exist) */
        (void)rtrequest(RTM_DELETE,
                        (struct sockaddr *)&dst,
                        NULL,
                        (struct sockaddr *)&mask,
                        0, NULL);

        /* Re-add with the new parameters */
        error = rtrequest(RTM_ADD,
                          (struct sockaddr *)&dst,
                          (struct sockaddr *)&gw,
                          (struct sockaddr *)&mask,
                          (int)flags, NULL);

        splx(s);
    }

    if(error) {
        writeErrnoValue(libPtr, error);
        return -1;
    }
    return 0;
}

/*
 * Walk radix sub-tree calling func for each leaf node.
 * Returns 0 on success or the first non-zero return from func.
 */
static int
route_walk(struct radix_node *rn,
           int (*func)(struct radix_node *, void *), void *arg)
{
    int error;
    for(;;) {
        while(rn->rn_b >= 0)
            rn = rn->rn_l;
        if((error = (*func)(rn, arg)) != 0)
            return error;
        while(rn->rn_p->rn_r == rn) {
            rn = rn->rn_p;
            if(rn->rn_flags & RNF_ROOT)
                return 0;
        }
        rn = rn->rn_p->rn_r;
    }
}

#define RT_ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/*
 * Callback context for GetRouteInfo routing table dump.
 */
struct getri_ctx {
    LONG  filter_flags;   /* RTF_* flags to filter by (0 = all) */
    int   pass;           /* 0 = size computation, 1 = data copy */
    int   size;           /* accumulated/needed size */
    char *buf;            /* output buffer (pass 1 only) */
};

static int
getri_callback(struct radix_node *rn, void *arg)
{
    struct getri_ctx *ctx = (struct getri_ctx *)arg;
    struct rtentry *rt;
    struct sockaddr *sa;
    int entry_size;

    for(; rn; rn = rn->rn_dupedkey) {
        if(rn->rn_flags & RNF_ROOT)
            continue;
        rt = (struct rtentry *)rn;
        if(ctx->filter_flags && !(rt->rt_flags & ctx->filter_flags))
            continue;

        /* Compute entry size: rt_msghdr + packed sockaddrs */
        entry_size = sizeof(struct rt_msghdr);
        if((sa = rt_key(rt)) != NULL)
            entry_size += RT_ROUNDUP(sa->sa_len);
        if((sa = rt->rt_gateway) != NULL)
            entry_size += RT_ROUNDUP(sa->sa_len);
        if((sa = rt_mask(rt)) != NULL)
            entry_size += RT_ROUNDUP(sa->sa_len);

        if(ctx->pass == 0) {
            ctx->size += entry_size;
        } else {
            struct rt_msghdr *rtm = (struct rt_msghdr *)ctx->buf;
            char *cp = (char *)(rtm + 1);
            int addrs = 0;

            memset(rtm, 0, sizeof(*rtm));
            rtm->rtm_version = RTM_VERSION;
            rtm->rtm_type    = RTM_GET;
            rtm->rtm_flags   = rt->rt_flags;
            rtm->rtm_use     = rt->rt_use;
            rtm->rtm_rmx     = rt->rt_rmx;
            rtm->rtm_rmx.rmx_expire = rt_expire_out(rt->rt_rmx.rmx_expire);
            if(rt->rt_ifp)
                rtm->rtm_index = rt->rt_ifp->if_index;

            if((sa = rt_key(rt)) != NULL) {
                int n = RT_ROUNDUP(sa->sa_len);
                memcpy(cp, sa, sa->sa_len);
                cp += n;
                addrs |= RTA_DST;
            }
            if((sa = rt->rt_gateway) != NULL) {
                int n = RT_ROUNDUP(sa->sa_len);
                memcpy(cp, sa, sa->sa_len);
                cp += n;
                addrs |= RTA_GATEWAY;
            }
            if((sa = rt_mask(rt)) != NULL) {
                int n = RT_ROUNDUP(sa->sa_len);
                memcpy(cp, sa, sa->sa_len);
                cp += n;
                addrs |= RTA_NETMASK;
            }

            rtm->rtm_addrs  = addrs;
            rtm->rtm_msglen = entry_size;
            ctx->buf += entry_size;
        }
    }
    return 0;
}

/*
 * IPv6's neighbours are not in the routing table: with RTF_LLINFO asked
 * for, IPv6 gives them as rows of their own (NFC_LLINFO_ROWS,
 * net/netmodule.h). Without IPv6 there are none.
 */
static void
getri_neighbours(struct getri_ctx *ctx, LONG address_family, LONG flags)
{
    struct nfc_llinfo_rows rows;

    if(!(flags & RTF_LLINFO) || (address_family != 0 && address_family != AF_INET6))
        return;
    rows.nr_size = 0;
    rows.nr_buf = ctx->pass == 0 ? NULL : ctx->buf;
    if(netfamily_control(AF_INET6, NULL, NFC_LLINFO_ROWS, (caddr_t)&rows, NULL) != 0)
        return;
    if(ctx->pass == 0)
        ctx->size += rows.nr_size;
    else
        ctx->buf = rows.nr_buf;
}

struct rt_msghdr *
rsadm_GetRouteInfo(LONG address_family, LONG flags, struct SocketBase *libPtr)
{
    struct radix_node_head *rnh;
    struct getri_ctx ctx;
    struct rt_msghdr *result;
    spl_t s;
    int total_size;

    /* Pass 0: compute total buffer size under lock */
    ctx.filter_flags = flags;
    ctx.pass = 0;
    ctx.size = 0;
    ctx.buf  = NULL;

    s = splnet();
    for(rnh = rnh_list; rnh; rnh = rnh->rnh_next) {
        if(rnh->rnh_af == 0)
            continue;
        if(address_family && rnh->rnh_af != (u_char)address_family)
            continue;
        if(rnh->rnh_treetop)
            route_walk(rnh->rnh_treetop, getri_callback, &ctx);
    }
    getri_neighbours(&ctx, address_family, flags);

    total_size = ctx.size;
    if(total_size == 0) {
        splx(s);
        writeErrnoValue(libPtr, ESRCH);
        return NULL;
    }

    /*
     * Allocate buffer with space for a zero-length sentinel.
     * Note: bsd_malloc under splnet is acceptable here — AROSTCP
     * uses a simple Forbid/Permit model and the allocator is safe
     * to call at any spl level.
     */
    result = (struct rt_msghdr *)bsd_malloc(total_size + sizeof(struct rt_msghdr),
                                            NULL, NULL);
    if(result == NULL) {
        splx(s);
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }

    /* Pass 1: fill the buffer (still under lock — table cannot change) */
    ctx.pass = 1;
    ctx.size = 0;
    ctx.buf  = (char *)result;

    for(rnh = rnh_list; rnh; rnh = rnh->rnh_next) {
        if(rnh->rnh_af == 0)
            continue;
        if(address_family && rnh->rnh_af != (u_char)address_family)
            continue;
        if(rnh->rnh_treetop)
            route_walk(rnh->rnh_treetop, getri_callback, &ctx);
    }
    getri_neighbours(&ctx, address_family, flags);
    splx(s);

    /* Write sentinel: zero-length rt_msghdr marks end of list */
    memset(ctx.buf, 0, sizeof(struct rt_msghdr));

    return result;
}

/*
 * ObtainInterfaceList - Roadshow's list of the interfaces: one struct Node
 * per interface, its name ("eth0", "lo0": what QueryInterfaceTagList() and
 * the ioctls take) in ln_Name, the name stored with the node. Free it with
 * ReleaseInterfaceList(). NULL with ENOMEM when memory runs out.
 */
struct List *
rsadm_ObtainInterfaceList(struct SocketBase *libPtr)
{
    struct ifnet *ifp;
    struct List *list;
    spl_t s;

    if((list = AllocVec(sizeof(*list), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }
    NewList(list);
    s = splnet();
    for(ifp = ifnet_list; ifp; ifp = ifp->if_next) {
        struct Node *n = AllocVec(sizeof(*n) + IFNAMSIZ + 4, MEMF_PUBLIC | MEMF_CLEAR);

        if(n == NULL) {
            struct Node *f;

            splx(s);
            while((f = RemHead(list)) != NULL)
                FreeVec(f);
            FreeVec(list);
            writeErrnoValue(libPtr, ENOMEM);
            return NULL;
        }
        n->ln_Name = (char *)(n + 1);
        if_fullname(ifp, n->ln_Name, IFNAMSIZ + 4);
        AddTail(list, n);
    }
    splx(s);
    return list;
}

/*
 * A name server as the calls below are given it: an IPv4 or an IPv6
 * address as text. Roadshow knows IPv4 servers only; a machine without
 * IPv4 has no other kind than IPv6, so both are taken. An IPv6 server that
 * needs a zone (link-local) cannot be said this way: it goes with its
 * interface (DNS6=, a Router Advertisement), as on a NAMESERVER line.
 * Fills in the family and the address of nsn; 0 if the text is neither.
 */
static int
dns_server_parse(CONST_STRPTR address, struct NameserventNode *nsn,
                 struct SocketBase *libPtr)
{
    bzero((caddr_t)&nsn->nsn_Addr6, sizeof(nsn->nsn_Addr6));
    if(__inet_aton(address, &nsn->nsn_Ent.ns_addr)) {
        nsn->nsn_Family = AF_INET;
        return 1;
    }
    nsn->nsn_Ent.ns_addr.s_addr = 0;
    if(__inet_pton(AF_INET6, (const char *)address, &nsn->nsn_Addr6, libPtr) == 1 &&
            !IN6_IS_ADDR_LINKLOCAL(&nsn->nsn_Addr6) &&
            !IN6_IS_ADDR_UNSPECIFIED(&nsn->nsn_Addr6) &&
            !IN6_IS_ADDR_MULTICAST(&nsn->nsn_Addr6)) {
        nsn->nsn_Family = AF_INET6;
        return 1;
    }
    return 0;
}

/* The same server: family (0 is IPv4, from old creators) and address. */
static int
dns_server_same(const struct NameserventNode *a, const struct NameserventNode *b)
{
    if(a->nsn_Family == AF_INET6 || b->nsn_Family == AF_INET6)
        return a->nsn_Family == b->nsn_Family &&
               IN6_ARE_ADDR_EQUAL(&a->nsn_Addr6, &b->nsn_Addr6);
    return a->nsn_Ent.ns_addr.s_addr == b->nsn_Ent.ns_addr.s_addr;
}

/* A server as text for ObtainDomainNameServerList(); buf of 46 at least. */
static void
dns_server_text(const struct NameserventNode *nsn, char *buf, int size,
                struct SocketBase *libPtr)
{
    if(nsn->nsn_Family != AF_INET6)
        fmt_ipaddr(buf, nsn->nsn_Ent.ns_addr);
    else if(__inet_ntop(AF_INET6, &nsn->nsn_Addr6, buf, size, libPtr) == NULL)
        buf[0] = '\0';
}

LONG
rsadm_AddDomainNameServer(STRPTR address, struct SocketBase *libPtr)
{
    struct NameserventNode parsed;
    struct NameserventNode *nsn;

    if(address == NULL) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    /* Parse the address string, of either family */
    if(!dns_server_parse(address, &parsed, libPtr)) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    if((nsn = bsd_malloc(sizeof(struct NameserventNode), NULL, NULL)) == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return -1;
    }

    nsn->nsn_EntSize = sizeof(nsn->nsn_Ent);
    nsn->nsn_Family = parsed.nsn_Family;
    nsn->nsn_Ent.ns_addr.s_addr = parsed.nsn_Ent.ns_addr.s_addr;
    nsn->nsn_Addr6 = parsed.nsn_Addr6;
    nsn->nsn_OwnerIndex = 0;
    nsn->nsn_Expire = 0;

    ObtainSemaphore(&DynDB.dyn_Lock);
    ndb_ops_locked();
    AddTail((struct List *)&DynDB.dyn_NameServers, (struct Node *)nsn);
    ndb_Serial++;
    ReleaseSemaphore(&DynDB.dyn_Lock);

    return 0;
}

LONG
rsadm_RemoveDomainNameServer(STRPTR address, struct SocketBase *libPtr)
{
    struct NameserventNode target;
    struct MinNode *node, *nnode;
    int found = 0;

    if(address == NULL) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    if(!dns_server_parse(address, &target, libPtr)) {
        writeErrnoValue(libPtr, EINVAL);
        return -1;
    }

    ObtainSemaphore(&DynDB.dyn_Lock);
    ndb_ops_locked();
    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ;) {
        struct NameserventNode *nsn = (struct NameserventNode *)node;
        nnode = node->mln_Succ;
        if(dns_server_same(nsn, &target)) {
            Remove((struct Node *)node);
            bsd_free(node, NULL);
            found = 1;
            break;
        }
        node = nnode;
    }
    if(found)
        ndb_Serial++;
    ReleaseSemaphore(&DynDB.dyn_Lock);

    if(!found) {
        writeErrnoValue(libPtr, ENOENT);
        return -1;
    }
    return 0;
}

struct List *
rsadm_ObtainDomainNameServerList(struct SocketBase *libPtr)
{
    struct List *list;
    struct MinNode *node;

    list = bsd_malloc(sizeof(struct List), NULL, NULL);
    if(list == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }
    NewList(list);

    ObtainSemaphoreShared(&ndb_Lock);

    /* Walk static nameservers from the NetDB */
    if(NDB) {
        for(node = NDB->ndb_NameServers.mlh_Head; node->mln_Succ;
                node = node->mln_Succ) {
            struct NameserventNode *nsn = (struct NameserventNode *)node;
            char ipstr[16];
            struct DomainNameServerNode *dnsn;
            int slen;

            if(nsn->nsn_Family == AF_INET6) continue; /* IPv4-only Roadshow API */
            fmt_ipaddr(ipstr, nsn->nsn_Ent.ns_addr);
            slen = strnlen(ipstr, sizeof(ipstr)) + 1;

            dnsn = bsd_malloc(sizeof(struct DomainNameServerNode) + slen,
                              NULL, NULL);
            if(dnsn == NULL)
                continue;
            dnsn->dnsn_Size = sizeof(struct DomainNameServerNode) + slen;
            dnsn->dnsn_Address = (STRPTR)(dnsn + 1);
            memcpy(dnsn->dnsn_Address, ipstr, slen);
            dnsn->dnsn_UseCount = -1;  /* static entry */
            AddTail(list, (struct Node *)dnsn);
        }
    }

    /* Walk dynamic nameservers from DynDB */
    ObtainSemaphoreShared(&DynDB.dyn_Lock);
    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ;
            node = node->mln_Succ) {
        struct NameserventNode *nsn = (struct NameserventNode *)node;
        char ipstr[16];
        struct DomainNameServerNode *dnsn;
        int slen;

        if(nsn->nsn_Family == AF_INET6) continue; /* IPv4-only Roadshow API */
        fmt_ipaddr(ipstr, nsn->nsn_Ent.ns_addr);
        slen = strnlen(ipstr, sizeof(ipstr)) + 1;

        dnsn = bsd_malloc(sizeof(struct DomainNameServerNode) + slen,
                          NULL, NULL);
        if(dnsn == NULL)
            continue;
        dnsn->dnsn_Size = sizeof(struct DomainNameServerNode) + slen;
        dnsn->dnsn_Address = (STRPTR)(dnsn + 1);
        memcpy(dnsn->dnsn_Address, ipstr, slen);
        dnsn->dnsn_UseCount = 1;  /* dynamic entry */
        AddTail(list, (struct Node *)dnsn);
    }
    ReleaseSemaphore(&DynDB.dyn_Lock);

    ReleaseSemaphore(&ndb_Lock);

    return list;
}

/* ------------------------------------------------------------------ */
/* Slot 117: GetDefaultDomainName                                     */
/* ------------------------------------------------------------------ */
LONG
rsadm_GetDefaultDomainName(STRPTR buffer, LONG buffer_size, struct SocketBase *libPtr)
{
    struct DomainentNode *dn;

    if (buffer == NULL || buffer_size <= 0)
        return 0;

    LOCK_R_NDB(NDB);
    dn = (struct DomainentNode *)NDB->ndb_Domains.mlh_Head;
    if (dn->dn_Node.mln_Succ != NULL && dn->dn_Ent.d_name != NULL) {
        size_t len = strnlen(dn->dn_Ent.d_name, (size_t)buffer_size - 1);
        memcpy(buffer, dn->dn_Ent.d_name, len);
        buffer[len] = '\0';
        UNLOCK_NDB(NDB);
        return 1; /* TRUE */
    }
    UNLOCK_NDB(NDB);

    buffer[0] = '\0';
    return 0; /* FALSE */
}

#endif
