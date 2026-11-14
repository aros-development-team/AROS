/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#include <conf.h>

#include <proto/dos.h>

#include <dos/dos.h>
#include <dos/rdargs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <api/amiga_api.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_includes.h>
#include <kern/amiga_config.h>
#include <kern/amiga_netdb.h>
#include <kern/accesscontrol.h>

#include "net/netdbpaths.h"
#include <net/sana2config.h>
#include <net/netmodule.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <protos/net/if_protos.h>
#include <kern/netconfig_ops.h>

LONG __inet_aton(CONST_STRPTR cp, struct in_addr *addr);  /* api/amiga_libcalls.c */

/*
 * The network database: what the configuration readers fill in and every
 * look-up reads (NDB and its lock), the dynamic database and its updates
 * while the stack runs (an interface's DNS= servers, DNS servers and
 * domains from Router Advertisements), and init_netdb()'s own part of the
 * start. The readers themselves - db/netdb and the files it includes, the
 * interface lines - are the module "netconfig" (kern/netconfig_module.c,
 * kern/netconfig_ops.h), loaded for a read and dropped after.
 */

/*
 * Global pointer for the NetDataBase
 */
struct NetDataBase *NDB = NULL;

/*
 * Global lock for the NetDatabase
 */
struct SignalSemaphore ndb_Lock;

/*
 * Dynamic items Database (for entries supplied by DHCP)
 */
struct DynDataBase DynDB;

/*
 * NDB Update Counter (used to keep resolver cache up to date)
 */
ULONG ndb_Serial;


/* Copy an address */
static int inline setaddr(struct sockaddr_in *sa, char *addr, u_short af)
{
    sa->sin_len = sizeof(struct sockaddr_in);
    sa->sin_family = af;
    return __inet_aton(addr, &sa->sin_addr);
}

/*
 * Minimal in-kernel IPv6 address parser (adapted from KAME inet_pton6).
 * The core's, for the DNS= servers of an interface (queued and applied
 * while the stack runs); the configuration reader parses with it too.
 */
int
inet6_aton(const char *src, struct in6_addr *dst)
{
    static const char xdigits_l[] = "0123456789abcdef",
                                    xdigits_u[] = "0123456789ABCDEF";
    u_char tmp[16], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, saw_xdigit;
    u_int val;

    memset((tp = tmp), 0, 16);
    endp  = tp + 16;
    colonp = NULL;
    if(*src == ':')
        if(*++src != ':')
            return 0;
    curtok = src;
    saw_xdigit = 0;
    val = 0;
    while((ch = *src++) != '\0') {
        const char *pch;
        if((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if(pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if(val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if(ch == ':') {
            curtok = src;
            if(!saw_xdigit) {
                if(colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            if(tp + 2 > endp)
                return 0;
            *tp++ = (u_char)((val >> 8) & 0xff);
            *tp++ = (u_char)(val & 0xff);
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        return 0;
    }
    if(saw_xdigit) {
        if(tp + 2 > endp)
            return 0;
        *tp++ = (u_char)((val >> 8) & 0xff);
        *tp++ = (u_char)(val & 0xff);
    }
    if(colonp != NULL) {
        const int n = tp - colonp;
        int i;
        for(i = 1; i <= n; i++) {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if(tp != endp)
        return 0;
    memcpy(dst, tmp, 16);
    return 1;
}

/*
 * Per-interface DNS servers.
 *
 * The DNS=/DNS6= tokens on an interface line give that interface's DNS
 * servers.  They are registered as dynamic nameservers tagged with the owning
 * interface (nsn_OwnerIndex), so the resolver picks them up (res_init merges
 * DynDB.dyn_NameServers) and they can be dropped again when the interface goes
 * away.  A server may be given as an IPv4 or an IPv6 literal (either token may
 * carry either family); the node is tagged with nsn_Family accordingly.
 */
/*
 * Which of an interface's entries go: IFDNS_CONFIGURED its DNS= servers
 * (replaced by ndb_set_locked()), IFDNS_LEARNED the servers and domains
 * learned from Router Advertisements (those with an end: the link went, and
 * with it the network that advertised them), IFDNS_ALL both (the interface
 * goes down or away).
 */
#define IFDNS_CONFIGURED 1
#define IFDNS_LEARNED    2
#define IFDNS_ALL        (IFDNS_CONFIGURED | IFDNS_LEARNED)

static void ifdns_remove_locked(ULONG index, int which)
{
    struct MinNode *node, *next;

    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ; node = next) {
        struct NameserventNode *nsn = (struct NameserventNode *)node;

        next = node->mln_Succ;
        if(nsn->nsn_OwnerIndex == index &&
                (which & (nsn->nsn_Expire != 0 ? IFDNS_LEARNED : IFDNS_CONFIGURED))) {
            Remove((struct Node *)node);
            bsd_free(node, M_NETDB);
        }
    }
    /* a domain is only ever learned (DNSSL): DNS= gives servers alone */
    if(!(which & IFDNS_LEARNED))
        return;
    for(node = DynDB.dyn_Domains.mlh_Head; node->mln_Succ; node = next) {
        next = node->mln_Succ;
        if(((struct DomainentNode *)node)->dn_OwnerIndex == index) {
            Remove((struct Node *)node);
            bsd_free(node, M_NETDB);
        }
    }
}

static int ifdns_add_csv_locked(ULONG index, const UBYTE *csv)
{
    const UBYTE *p = csv;
    char buf[64];       /* one server address string (IPv4 or IPv6 literal) */
    int added = 0;

    while(p && *p) {
        /* Bound the scan to the buffer: strnlen() never over-reads past a
         * missing terminator, and a token that does not fit is skipped. */
        size_t avail = strnlen((const char *)p, sizeof(buf));
        const UBYTE *comma = (const UBYTE *)memchr(p, ',', avail);
        size_t n = comma ? (size_t)(comma - p) : avail;
        struct sockaddr_in sin;
        struct in6_addr in6;

        if(n > 0 && n < sizeof(buf)) {
            struct NameserventNode *nsn = NULL;

            memcpy(buf, p, n);
            buf[n] = '\0';
            memset(&sin, 0, sizeof(sin));
            if(setaddr(&sin, buf, AF_INET)) {
                /* IPv4 server */
                if((nsn = bsd_malloc(sizeof(*nsn), M_NETDB, M_WAITOK)) != NULL) {
                    nsn->nsn_EntSize = sizeof(nsn->nsn_Ent);
                    nsn->nsn_Family = AF_INET;
                    nsn->nsn_Ent.ns_addr.s_addr = sin.sin_addr.s_addr;
                }
            } else if(inet6_aton(buf, &in6)) {
                /* IPv6 server */
                if((nsn = bsd_malloc(sizeof(*nsn), M_NETDB, M_WAITOK)) != NULL) {
                    nsn->nsn_EntSize = sizeof(nsn->nsn_Ent);
                    nsn->nsn_Family = AF_INET6;
                    nsn->nsn_Ent.ns_addr.s_addr = 0;
                    nsn->nsn_Addr6 = in6;
                }
            }
            if(nsn) {
                nsn->nsn_OwnerIndex = index;
                nsn->nsn_Expire = 0;
                AddTail((struct List *)&DynDB.dyn_NameServers,
                        (struct Node *)nsn);
                added++;
            }
        }
        /* Advance past this token; the comma offset is within the bounded
         * region, so there is no unterminated-string read here either. */
        p = (comma && comma[1]) ? comma + 1 : NULL;
    }
    return added;
}

/*
 * The stack task does not wait for DynDB.dyn_Lock: an application holds it
 * from ClearDynNameServ()/ClearDynDomain() to EndDynNameServ()/EndDynDomain()
 * (miami.library) for as long as it likes, and the stack would stand still
 * meanwhile. What the stack task must change without it - an interface's
 * DNS= servers set, its entries dropped as it goes down or away, everything
 * flushed for a reload - is queued, copied, and applied in order by whoever
 * holds the lock next, before any change of its own (ndb_ops_locked()):
 * the holder's own later calls, or ndb_ra_timer() within a tick of the
 * lock being free. Entries name their interface by index, which is never
 * reused, so those of an interface already gone cannot dangle meanwhile.
 * Changes learned from Router Advertisements are not queued: the next
 * advertisement repeats them.
 */
#define NDB_OP_SET      1       /* an interface's DNS= servers */
#define NDB_OP_CLEAR    2       /* all of an interface's entries */
#define NDB_OP_FLUSH    3       /* every dynamic entry */
#define NDB_OP_FORGET   4       /* an interface's learned entries */

struct ndb_op {
    struct MinNode node;
    UBYTE  op;
    ULONG  index;
    char  *v4, *v6;             /* NDB_OP_SET: copies, in the node */
};

static struct MinList ndb_ops;  /* NewList()ed by init_netdb() */

/* Without the lock, the stack task queues; any other task may wait. */
static BOOL ndb_lock(void)
{
    extern struct Task *AROSTCP_Task;

    if(FindTask(NULL) != AROSTCP_Task) {
        ObtainSemaphore(&DynDB.dyn_Lock);
        return TRUE;
    }
    return AttemptSemaphore(&DynDB.dyn_Lock);
}

static void ndb_flush_locked(void)
{
    struct MinNode *node, *nnode;

    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ; node = nnode) {
        nnode = node->mln_Succ;
        bsd_free(node, NULL);
    }
    NewList((struct List *)&DynDB.dyn_NameServers);
    for(node = DynDB.dyn_Domains.mlh_Head; node->mln_Succ; node = nnode) {
        nnode = node->mln_Succ;
        bsd_free(node, NULL);
    }
    NewList((struct List *)&DynDB.dyn_Domains);
}

/* With the lock: an interface's DNS= servers, replacing the earlier ones. */
static int ndb_set_locked(ULONG index, const UBYTE *v4csv, const UBYTE *v6csv)
{
    int added = 0;

    ifdns_remove_locked(index, IFDNS_CONFIGURED);
    if(v4csv && *v4csv)
        added += ifdns_add_csv_locked(index, v4csv);
    if(v6csv && *v6csv)
        added += ifdns_add_csv_locked(index, v6csv);
    return added;
}

/* Queue op; FALSE if there is no memory for it. */
static BOOL ndb_queue(UBYTE op, ULONG index, const UBYTE *v4, const UBYTE *v6)
{
    size_t l4 = v4 ? strnlen((const char *)v4, CONFIGLINELEN) + 1 : 0;
    size_t l6 = v6 ? strnlen((const char *)v6, CONFIGLINELEN) + 1 : 0;
    struct ndb_op *o = bsd_malloc(sizeof(*o) + l4 + l6, M_NETDB, M_WAITOK);

    if(o == NULL)
        return FALSE;
    o->op = op;
    o->index = index;
    o->v4 = o->v6 = NULL;
    if(l4) {
        o->v4 = (char *)(o + 1);
        memcpy(o->v4, v4, l4 - 1);
        o->v4[l4 - 1] = '\0';
    }
    if(l6) {
        o->v6 = (char *)(o + 1) + l4;
        memcpy(o->v6, v6, l6 - 1);
        o->v6[l6 - 1] = '\0';
    }
    Forbid();
    AddTail((struct List *)&ndb_ops, (struct Node *)o);
    Permit();
    return TRUE;
}

/* Apply what the stack task queued, in order. Call with the lock held,
 * before changing anything. Whether there was anything. */
int ndb_ops_locked(void)
{
    struct ndb_op *o;
    int n = 0;

    for(;;) {
        Forbid();
        o = (struct ndb_op *)RemHead((struct List *)&ndb_ops);
        Permit();
        if(o == NULL)
            break;
        if(o->op == NDB_OP_FLUSH)
            ndb_flush_locked();
        else if(o->op == NDB_OP_CLEAR)
            ifdns_remove_locked(o->index, IFDNS_ALL);
        else if(o->op == NDB_OP_FORGET)
            ifdns_remove_locked(o->index, IFDNS_LEARNED);
        else
            ndb_set_locked(o->index, (UBYTE *)o->v4, (UBYTE *)o->v6);
        bsd_free(o, M_NETDB);
        n++;
    }
    if(n)
        ndb_Serial++;
    return n;
}

/* Replace interface ifp's per-interface DNS servers with those in the two
 * comma-separated lists (either may be NULL/empty). */
void ifnet_set_dns(struct ifnet *ifp, const UBYTE *v4csv, const UBYTE *v6csv)
{
    int added;

    if(!ndb_lock()) {
        if(ndb_queue(NDB_OP_SET, ifp->if_index, v4csv, v6csv))
            return;
        ObtainSemaphore(&DynDB.dyn_Lock);   /* no memory to queue it: wait */
    }
    ndb_ops_locked();
    added = ndb_set_locked(ifp->if_index, v4csv, v6csv);
    ReleaseSemaphore(&DynDB.dyn_Lock);
    ndb_Serial++;       /* make the resolver refresh its nameserver list */

    if(added > 0)
        __log(LOG_NOTICE, "%s%d: registered %d DNS server(s)\n",
              ifp->if_name, ifp->if_unit, added);
}

static void ifnet_remove_dns(struct ifnet *ifp, UBYTE op, int which)
{
    if(!ndb_lock()) {
        if(ndb_queue(op, ifp->if_index, NULL, NULL))
            return;
        ObtainSemaphore(&DynDB.dyn_Lock);
    }
    ndb_ops_locked();
    ifdns_remove_locked(ifp->if_index, which);
    ReleaseSemaphore(&DynDB.dyn_Lock);
    ndb_Serial++;
}

/* Drop all of ifp's DNS servers and search domains, those from its DNS=
 * tokens and those learned from Router Advertisements (interface going down
 * or away). */
void ifnet_clear_dns(struct ifnet *ifp)
{
    ifnet_remove_dns(ifp, NDB_OP_CLEAR, IFDNS_ALL);
}

/* Drop what ifp learned from Router Advertisements; its DNS= servers are
 * the configuration's and stay (the link went: the network that advertised
 * the others may not be there when it is back). */
void ifnet_forget_dns(struct ifnet *ifp)
{
    ifnet_remove_dns(ifp, NDB_OP_FORGET, IFDNS_LEARNED);
}

/*
 * DNS configuration from Router Advertisements (RFC 8106).
 *
 * The servers of an RDNSS option and the domains of a DNSSL option go into
 * the dynamic database tagged with the interface they were heard on, as the
 * DNS= tokens' servers are, and with when they end: each advertisement
 * renews them, a lifetime of 0 ends them at once (section 5.1), and
 * ndb_ra_timer() drops them when their time is up.  The static database
 * (NAMESERVER, DOMAIN) is never touched, and the resolver keeps trying its
 * entries first.  At most NDB_RA_MAX of each are kept per interface; a new
 * one takes the place of the one that ends soonest (section 5.3.1).
 *
 * This runs in the stack task, which must not wait for the lock: an
 * application holds it from ClearDynNameServ() or ClearDynDomain() until
 * EndDynNameServ() or EndDynDomain() (miami.library), for as long as it
 * likes. So the lock is only attempted; without it the option is passed
 * over and taken from the next advertisement, as ndb_ra_timer() does.
 */
#define NDB_RA_MAX 3

/* Seconds on the monotonic clock: setting the system time moves no ends. */
static ULONG ndb_ra_now(void)
{
    struct timeval tv;

    GetUpTime(&tv);
    return tv.tv_sec;
}

/* When an entry with this lifetime ends; never 0, which marks the others. */
static ULONG ndb_ra_end(ULONG lifetime)
{
    ULONG now = ndb_ra_now(), end = now + lifetime;

    if(lifetime == NDB_RA_FOREVER)
        return NDB_RA_FOREVER;
    if(end < now || end == NDB_RA_FOREVER)
        return NDB_RA_FOREVER - 1;
    return end;
}

void ndb_ra_server(struct ifnet *ifp, const struct in6_addr *addr, ULONG lifetime)
{
    struct NameserventNode *nsn, *found = NULL, *soonest = NULL;
    int n = 0, changed = 0;

    if(!AttemptSemaphore(&DynDB.dyn_Lock))
        return;             /* the next advertisement gives it again */
    ndb_ops_locked();
    for(nsn = (struct NameserventNode *)DynDB.dyn_NameServers.mlh_Head;
            nsn->nsn_Node.mln_Succ;
            nsn = (struct NameserventNode *)nsn->nsn_Node.mln_Succ) {
        if(nsn->nsn_OwnerIndex != (ULONG)ifp->if_index || nsn->nsn_Expire == 0)
            continue;
        if(IN6_ARE_ADDR_EQUAL(&nsn->nsn_Addr6, addr))
            found = nsn;
        if(soonest == NULL || nsn->nsn_Expire < soonest->nsn_Expire)
            soonest = nsn;
        n++;
    }
    if(found != NULL && lifetime == 0) {
        Remove((struct Node *)found);
        bsd_free(found, M_NETDB);
        changed = 1;
    } else if(found != NULL) {
        found->nsn_Expire = ndb_ra_end(lifetime);
    } else if(lifetime != 0) {
        if(n >= NDB_RA_MAX)
            found = soonest;
        else if((found = bsd_malloc(sizeof(*found), M_NETDB, M_WAITOK)) != NULL) {
            found->nsn_EntSize = sizeof(found->nsn_Ent);
            found->nsn_Family = AF_INET6;
            found->nsn_Ent.ns_addr.s_addr = 0;
            found->nsn_OwnerIndex = ifp->if_index;
            AddTail((struct List *)&DynDB.dyn_NameServers, (struct Node *)found);
        }
        if(found != NULL) {
            found->nsn_Addr6 = *addr;
            found->nsn_Expire = ndb_ra_end(lifetime);
            changed = 1;
        }
    }
    ReleaseSemaphore(&DynDB.dyn_Lock);
    if(changed)
        ndb_Serial++;       /* make the resolver refresh its nameserver list */
}

void ndb_ra_domain(struct ifnet *ifp, const char *name, ULONG lifetime)
{
    struct DomainentNode *dn, *found = NULL, *soonest = NULL;
    int n = 0, changed = 0;
    size_t len = strnlen(name, MAXHOSTNAMELEN) + 1;

    if(!AttemptSemaphore(&DynDB.dyn_Lock))
        return;             /* the next advertisement gives it again */
    ndb_ops_locked();
    for(dn = (struct DomainentNode *)DynDB.dyn_Domains.mlh_Head;
            dn->dn_Node.mln_Succ;
            dn = (struct DomainentNode *)dn->dn_Node.mln_Succ) {
        if(dn->dn_OwnerIndex != (ULONG)ifp->if_index || dn->dn_Expire == 0)
            continue;
        if(strcasecmp(dn->dn_Ent.d_name, name) == 0)
            found = dn;
        if(soonest == NULL || dn->dn_Expire < soonest->dn_Expire)
            soonest = dn;
        n++;
    }
    if(found != NULL && lifetime != 0) {
        found->dn_Expire = ndb_ra_end(lifetime);
    } else {
        if(found == NULL && lifetime != 0 && n >= NDB_RA_MAX)
            found = soonest;
        if(found != NULL) {
            Remove((struct Node *)found);
            bsd_free(found, M_NETDB);
            changed = 1;
        }
        if(lifetime != 0 &&
                (dn = bsd_malloc(sizeof(*dn) + len, M_NETDB, M_WAITOK)) != NULL) {
            dn->dn_EntSize = sizeof(*dn) + len - sizeof(struct GenentNode);
            dn->dn_Ent.d_name = (char *)(dn + 1);
            memcpy(dn + 1, name, len - 1);
            dn->dn_Ent.d_name[len - 1] = '\0';
            dn->dn_OwnerIndex = ifp->if_index;
            dn->dn_Expire = ndb_ra_end(lifetime);
            AddTail((struct List *)&DynDB.dyn_Domains, (struct Node *)dn);
            changed = 1;
        }
    }
    ReleaseSemaphore(&DynDB.dyn_Lock);
    if(changed)
        ndb_Serial++;
}

/* Once a second: drop what has reached its end.  Without the lock now, the
 * next second does it. */
void ndb_ra_timer(void)
{
    struct MinNode *node, *next;
    ULONG now = ndb_ra_now();
    int changed = 0;

    if(!AttemptSemaphore(&DynDB.dyn_Lock))
        return;
    ndb_ops_locked();
    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ; node = next) {
        ULONG end = ((struct NameserventNode *)node)->nsn_Expire;

        next = node->mln_Succ;
        if(end != 0 && end != NDB_RA_FOREVER && end <= now) {
            Remove((struct Node *)node);
            bsd_free(node, M_NETDB);
            changed = 1;
        }
    }
    for(node = DynDB.dyn_Domains.mlh_Head; node->mln_Succ; node = next) {
        ULONG end = ((struct DomainentNode *)node)->dn_Expire;

        next = node->mln_Succ;
        if(end != 0 && end != NDB_RA_FOREVER && end <= now) {
            Remove((struct Node *)node);
            bsd_free(node, M_NETDB);
            changed = 1;
        }
    }
    ReleaseSemaphore(&DynDB.dyn_Lock);
    if(changed)
        ndb_Serial++;
}

/*
 * The module that hears Router Advertisements stops: what they gave - every
 * server and domain with an end - goes at once, as when its time is up.
 * (Nobody would call ndb_ra_timer() any more, and the entries would stay
 * for good.) 0, or EBUSY when an application holds the dynamic database
 * (from ClearDynNameServ() until EndDynNameServ()): the stack's task must
 * not wait for it, and the caller refuses its stop instead - nothing was
 * changed.
 */
int ndb_ra_flush(void)
{
    struct MinNode *node, *next;
    int changed = 0;

    if(!AttemptSemaphore(&DynDB.dyn_Lock))
        return EBUSY;
    ndb_ops_locked();
    for(node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ; node = next) {
        next = node->mln_Succ;
        if(((struct NameserventNode *)node)->nsn_Expire != 0) {
            Remove((struct Node *)node);
            bsd_free(node, M_NETDB);
            changed = 1;
        }
    }
    for(node = DynDB.dyn_Domains.mlh_Head; node->mln_Succ; node = next) {
        next = node->mln_Succ;
        if(((struct DomainentNode *)node)->dn_Expire != 0) {
            Remove((struct Node *)node);
            bsd_free(node, M_NETDB);
            changed = 1;
        }
    }
    ReleaseSemaphore(&DynDB.dyn_Lock);
    if(changed)
        ndb_Serial++;
    return 0;
}

/*
 * The access control table of a database the reader has filled
 * (kern/accesscontrol.h). Here, not in the reader: the header's function
 * is an inline one that names the core's allocator, which a module
 * library cannot link.
 */
void
netdb_access_setup(struct NetDataBase *ndb)
{
    setup_accesscontroltable(ndb);
}

/*
 * Initialize the Network Data Base: the locks and lists are the core's,
 * made once; db/netdb is read by the configuration reader
 * (netconfig_netdb_init(), which the stack's task has loaded).
 */
LONG
init_netdb(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_netdb.c) init_netdb()\n"));
#endif

    InitSemaphore(&ndb_Lock);
    InitSemaphore(&DynDB.dyn_Lock);
    NewList((struct List *)&DynDB.dyn_NameServers);
    NewList((struct List *)&DynDB.dyn_Domains);
    NewList((struct List *)&ndb_ops);
    ndb_Serial = 0;

    return netconfig_netdb_init();
}

void netdb_deinit(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_netdb.c) netdb_deinit()\n"));
#endif
    netdb_defer_forget(NULL);
}

/*
 * Flush all DYNAMIC database entries (DHCP/interface-supplied nameservers and
 * domains).  Used by the resolver service on an in-place reload: the sources
 * (interfaces, DHCP) repopulate it as they come back up.  Static entries live
 * in the NDB and are rebuilt separately by netdb_reload().
 */
void dyndb_flush(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_netdb.c) dyndb_flush()\n"));
#endif

    if(!ndb_lock()) {
        if(ndb_queue(NDB_OP_FLUSH, 0, NULL, NULL))
            return;
        ObtainSemaphore(&DynDB.dyn_Lock);
    }
    ndb_ops_locked();
    ndb_flush_locked();
    ReleaseSemaphore(&DynDB.dyn_Lock);
    ndb_Serial++;
}
