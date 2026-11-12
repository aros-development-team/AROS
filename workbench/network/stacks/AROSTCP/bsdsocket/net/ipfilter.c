/*
 * net/ipfilter.c - IP Filter (ipf) kernel engine for AROSTCP
 *
 * Judges the packets of both families against the filter rules, as hooks
 * on the packet hook points of IPv4 and IPv6 (net/pfil.h). Compatible
 * with Roadshow ipf rule syntax; IPv6 rules are struct ipf_rule6
 * (common/include/net/ipfilter.h). Part of the packet services module
 * (net/packet_module.c).
 *
 * Copyright (C) 2026 The AROS Dev Team
 */

#include <conf.h>

#ifdef ENABLE_PACKET_FILTER

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/synch.h>

#include <kern/amiga_includes.h>

#include <net/if.h>
#include <net/pfil.h>
#include <net/netmodule.h>          /* netioctl_register(): net/netdemand.h */
#include <sys/ioctl.h>
#include "ipfilter.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include <string.h>

#include <net/netcore_map.h>        /* always the last include */

/* Global filter and NAT state; the state requests copy ipf_global as it is */
struct ipf_state ipf_global;
struct ipnat_state ipnat_global;

/*
 * The IPv6 rules: the engine's own, apart from ipf_global, whose layout
 * the state requests copy out to programs written before IPv6 rules
 * existed. rule_count of ipf_global counts the rules of both families.
 */
static struct ipf_rule6 *ipf_rules6_in = NULL;
static struct ipf_rule6 *ipf_rules6_out = NULL;

static int ipf_ioctl(struct socket *so, ULONG cmd, caddr_t data);
static int ipf_hook4(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir);
static int ipf_hook6(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir);

/*
 * The engine's hooks, on the points of IPv4 and IPv6 while the filter is
 * enabled: while it is disabled a packet does not enter the engine at all
 * (the families test one word).
 */
static struct pfil_hook ipf_ph4 = { NULL, ipf_hook4, NULL, PFIL_IN | PFIL_OUT, 0, NULL };
static struct pfil_hook ipf_ph6 = { NULL, ipf_hook6, NULL, PFIL_IN | PFIL_OUT, 0, NULL };
static int ipf_hooked = 0;

/*
 * THE MODULE'S SIDE (net/packet_module.c), in the stack's task, fenced.
 *
 * Start: the state is new, and the filter and NAT requests of
 * IoctlSocket() - group 'r', numbers 60 to 95 of sys/ioctl.h - are handed
 * to ipf_ioctl() from now on (net/netdemand.h).
 */
int
ipf_module_start(struct netmodule *nm)
{
    int error;

    memset(&ipf_global, 0, sizeof(ipf_global));
    memset(&ipnat_global, 0, sizeof(ipnat_global));
    ipf_rules6_in = ipf_rules6_out = NULL;
    ipf_ph4.ph_owner = ipf_ph6.ph_owner = nm;
    ipf_hooked = 0;

    if((error = netioctl_register('r', 60, 95, ipf_ioctl, nm, 0)) != 0) {
        __log(LOG_ERR, "ipfilter: the filter requests could not be taken (error %ld)\n",
              (long)error);
        return (error);
    }
    __log(LOG_INFO, "ipfilter: initialized\n");
    return (0);
}

/*
 * Stop: refused before (ipf_module_busy()) while the filter is enabled or
 * holds a rule, so nothing is hooked and nothing is to be freed - what is
 * left (NAT translation entries, which nothing makes) is freed all the
 * same. The requests are given back: they are served with the fence held,
 * so nobody is inside ipf_ioctl() now.
 */
void
ipf_module_stop(void)
{
    ipf_disable();
    (void)ipf_flush(IPF_FLUSH_ALL);
    (void)ipnat_clear_rules();
    (void)ipnat_flush_entries();
    (void)netioctl_unregister(ipf_ioctl);
    __log(LOG_INFO, "ipfilter: cleaned up\n");
}

int
ipf_module_busy(void)
{
    return (ipf_global.enabled || ipf_global.rule_count != 0 ||
            ipf_global.rules_in != NULL || ipf_global.rules_out != NULL ||
            ipf_rules6_in != NULL || ipf_rules6_out != NULL ||
            ipnat_global.rules != NULL);
}

/*
 * The filter and NAT requests of IoctlSocket(), while the engine runs: the
 * core hands them here with the socket calls fenced, after its generic
 * checks of the request (net/netdemand.h). A number of the range that is
 * not one of them: NETIOCTL_PASS, and the core treats it as the routing
 * ioctl it was before the range was taken.
 */
static int
ipf_ioctl(struct socket *so, ULONG cmd, caddr_t data)
{
    switch(cmd) {
    case SIOCFRENB:
        if(*(u_int *)data)
            ipf_enable();
        else
            ipf_disable();
        return 0;

    case SIOCIPFFL: {
        int flags = *(int *)data;
        int flushed = ipf_flush(flags);
        *(int *)data = flushed;
        return 0;
    }

    case SIOCADAFR:	/* SIOCADDFR */
        return ipf_add_rule((struct ipf_rule *)data);

    case SIOCINAFR:
        return ipf_insert_rule((struct ipf_rule *)data);

    case SIOCRMAFR:	/* SIOCDELFR */
        return ipf_remove_rule((struct ipf_rule *)data);

    case SIOCADAFR6:
        return ipf_add_rule6((struct ipf_rule6 *)data, 0);

    case SIOCINAFR6:
        return ipf_add_rule6((struct ipf_rule6 *)data, 1);

    case SIOCRMAFR6:
        return ipf_remove_rule6((struct ipf_rule6 *)data);

    case SIOCGETFS: {
        struct ipf_state *st = (struct ipf_state *)data;
        memcpy(st, &ipf_global, sizeof(*st));
        return 0;
    }

    case SIOCSETFF:
        /* Set global log flags (future use) */
        return 0;

    case SIOCGETFF:
        *(u_int *)data = 0;	/* log flags, future use */
        return 0;

    case SIOCFRZST: {
        struct ipf_state *st = (struct ipf_state *)data;
        memcpy(st, &ipf_global, sizeof(*st));
        ipf_global.passed = 0;
        ipf_global.blocked = 0;
        ipf_global.logged = 0;
        return 0;
    }

    case SIOCADNAT:
        return ipnat_add_rule((struct ipnat_rule *)data);

    case SIOCRMNAT:
        return ipnat_remove_rule((struct ipnat_rule *)data);

    case SIOCFLNAT: {
        int cleared = ipnat_clear_rules();
        *(int *)data = cleared;
        return 0;
    }

    case SIOCCNATL: {
        int flushed = ipnat_flush_entries();
        *(int *)data = flushed;
        return 0;
    }

    case SIOCGNATS: {
        struct ipnat_state *ns = (struct ipnat_state *)data;
        memcpy(ns, &ipnat_global, sizeof(*ns));
        return 0;
    }

    default:
        return NETIOCTL_PASS;
    }
}

int
ipf_enabled(void)
{
    return ipf_global.enabled;
}

/*
 * Enabled: the hooks go on both families' points (a family that is not
 * running gets them when it starts: net/pfil.h). Called with the socket
 * calls fenced (a request, or the module's stop in the stack's task).
 */
void
ipf_enable(void)
{
    ipf_global.enabled = 1;
    if(!ipf_hooked) {
        (void)pfil_hook_add(MIAMIPFBPT_IP, &ipf_ph4);
        (void)pfil_hook_add(MIAMIPFBPT_IP6, &ipf_ph6);
        ipf_hooked = 1;
    }
    __log(LOG_INFO, "ipfilter: enabled\n");
}

void
ipf_disable(void)
{
    ipf_global.enabled = 0;
    if(ipf_hooked) {
        (void)pfil_hook_remove(MIAMIPFBPT_IP, &ipf_ph4);
        (void)pfil_hook_remove(MIAMIPFBPT_IP6, &ipf_ph6);
        ipf_hooked = 0;
    }
    __log(LOG_INFO, "ipfilter: disabled\n");
}

/*
 * Add a rule to the appropriate list (end of list).
 */
int
ipf_add_rule(struct ipf_rule *template)
{
    struct ipf_rule *rule, **tailp;
    spl_t s;

    if(ipf_global.rule_count >= IPF_MAXRULES)
        return ENOMEM;

    rule = bsd_malloc(sizeof(*rule), NULL, NULL);
    if(rule == NULL)
        return ENOMEM;

    memcpy(rule, template, sizeof(*rule));
    rule->next = NULL;
    rule->hits = 0;
    rule->bytes = 0;

    s = splnet();

    if(rule->dir & IPF_IN) {
        tailp = &ipf_global.rules_in;
        while(*tailp != NULL)
            tailp = &(*tailp)->next;
        *tailp = rule;
    }
    if(rule->dir & IPF_OUT) {
        /* If both IN and OUT, duplicate for output list */
        if(rule->dir & IPF_IN) {
            struct ipf_rule *rule2 = bsd_malloc(sizeof(*rule2), NULL, NULL);
            if(rule2 == NULL) {
                splx(s);
                return ENOMEM;
            }
            memcpy(rule2, rule, sizeof(*rule2));
            rule2->next = NULL;
            tailp = &ipf_global.rules_out;
            while(*tailp != NULL)
                tailp = &(*tailp)->next;
            *tailp = rule2;
        } else {
            tailp = &ipf_global.rules_out;
            while(*tailp != NULL)
                tailp = &(*tailp)->next;
            *tailp = rule;
        }
    }

    ipf_global.rule_count++;
    splx(s);
    return 0;
}

/*
 * Insert a rule at a specific position.
 * Position is stored in template->hits (FreeBSD convention).
 */
int
ipf_insert_rule(struct ipf_rule *template)
{
    struct ipf_rule *rule, **pp;
    spl_t s;
    int pos;

    if(ipf_global.rule_count >= IPF_MAXRULES)
        return ENOMEM;

    rule = bsd_malloc(sizeof(*rule), NULL, NULL);
    if(rule == NULL)
        return ENOMEM;

    memcpy(rule, template, sizeof(*rule));
    pos = (int)rule->hits;
    rule->next = NULL;
    rule->hits = 0;
    rule->bytes = 0;

    s = splnet();

    pp = (rule->dir & IPF_OUT) ? &ipf_global.rules_out
         : &ipf_global.rules_in;
    while(pos > 0 && *pp != NULL) {
        pp = &(*pp)->next;
        pos--;
    }
    rule->next = *pp;
    *pp = rule;

    ipf_global.rule_count++;
    splx(s);
    return 0;
}

/*
 * Remove a matching rule from the list.
 */
int
ipf_remove_rule(struct ipf_rule *template)
{
    struct ipf_rule **pp, *r;
    spl_t s;

    s = splnet();

    pp = (template->dir & IPF_OUT) ? &ipf_global.rules_out
         : &ipf_global.rules_in;
    for(; *pp != NULL; pp = &(*pp)->next) {
        r = *pp;
        if(r->action == template->action &&
                r->dir == template->dir &&
                r->proto == template->proto &&
                r->src_addr.s_addr == template->src_addr.s_addr &&
                r->src_mask.s_addr == template->src_mask.s_addr &&
                r->dst_addr.s_addr == template->dst_addr.s_addr &&
                r->dst_mask.s_addr == template->dst_mask.s_addr) {
            *pp = r->next;
            bsd_free(r, NULL);
            ipf_global.rule_count--;
            splx(s);
            return 0;
        }
    }

    splx(s);
    return ESRCH;
}

/*
 * IPv6 RULES. As the IPv4 ones: appended (or inserted at the position in
 * `hits'), a rule for both directions on both lists, counted once.
 */
static int
ipf_rule6_valid(const struct ipf_rule6 *r)
{
    return (r->size == sizeof(struct ipf_rule6) && r->src_plen <= 128 &&
            r->dst_plen <= 128 && (r->dir & IPF_INOUT) != 0);
}

static void
ipf_put_rule6(struct ipf_rule6 **pp, struct ipf_rule6 *rule, int pos)
{
    while(*pp != NULL && pos != 0) {
        pp = &(*pp)->next;
        if(pos > 0)
            pos--;
    }
    rule->next = *pp;
    *pp = rule;
}

int
ipf_add_rule6(struct ipf_rule6 *template, int insert)
{
    struct ipf_rule6 *rin = NULL, *rout = NULL;
    int pos;
    spl_t s;

    if(!ipf_rule6_valid(template))
        return EINVAL;
    if(ipf_global.rule_count >= IPF_MAXRULES)
        return ENOMEM;
    /* -1: at the end */
    pos = insert ? (int)template->hits : -1;
    if(insert && pos < 0)
        pos = 0;

    if(((template->dir & IPF_IN) &&
            (rin = bsd_malloc(sizeof(*rin), NULL, NULL)) == NULL) ||
            ((template->dir & IPF_OUT) &&
             (rout = bsd_malloc(sizeof(*rout), NULL, NULL)) == NULL)) {
        if(rin != NULL)
            bsd_free(rin, NULL);
        return ENOMEM;
    }

    s = splnet();
    if(rin != NULL) {
        memcpy(rin, template, sizeof(*rin));
        rin->hits = rin->bytes = 0;
        ipf_put_rule6(&ipf_rules6_in, rin, pos);
    }
    if(rout != NULL) {
        memcpy(rout, template, sizeof(*rout));
        rout->hits = rout->bytes = 0;
        ipf_put_rule6(&ipf_rules6_out, rout, pos);
    }
    ipf_global.rule_count++;
    splx(s);
    return 0;
}

/*
 * The same rule: every field from `size' to `icmp_code' (not `next', not
 * the counters, not the padding before them). A program clears the
 * structure before it fills it, as ipf does.
 */
static int
ipf_same_rule6(const struct ipf_rule6 *a, const struct ipf_rule6 *b)
{
    return (memcmp(&a->size, &b->size,
                   (char *)(&a->icmp_code + 1) - (char *)&a->size) == 0);
}

int
ipf_remove_rule6(struct ipf_rule6 *template)
{
    struct ipf_rule6 **pp, *r;
    int found = 0, list;
    spl_t s;

    if(!ipf_rule6_valid(template))
        return EINVAL;

    s = splnet();
    for(list = 0; list < 2; list++) {
        if(!(template->dir & (list == 0 ? IPF_IN : IPF_OUT)))
            continue;
        for(pp = list == 0 ? &ipf_rules6_in : &ipf_rules6_out; *pp != NULL; pp = &(*pp)->next) {
            r = *pp;
            if(ipf_same_rule6(r, template)) {
                *pp = r->next;
                bsd_free(r, NULL);
                found = 1;
                break;
            }
        }
    }
    if(found && ipf_global.rule_count > 0)
        ipf_global.rule_count--;
    splx(s);
    return (found ? 0 : ESRCH);
}

static void
ipf_free_list(struct ipf_rule **listp)
{
    struct ipf_rule *r, *next;

    for(r = *listp; r != NULL; r = next) {
        next = r->next;
        bsd_free(r, NULL);
    }
    *listp = NULL;
}

/* the same for an IPv6 list; how many there were */
static int
ipf_free_list6(struct ipf_rule6 **listp)
{
    struct ipf_rule6 *r, *next;
    int count = 0;

    for(r = *listp; r != NULL; r = next) {
        next = r->next;
        bsd_free(r, NULL);
        count++;
    }
    *listp = NULL;
    return count;
}

/* the rules of both families, in the lists given by `flags' */
int
ipf_flush(int flags)
{
    spl_t s;
    int count = 0;
    struct ipf_rule *r;
    struct ipf_rule6 *r6;

    s = splnet();
    if(flags & IPF_FLUSH_IN) {
        for(r = ipf_global.rules_in; r != NULL; r = r->next)
            count++;
        ipf_free_list(&ipf_global.rules_in);
        count += ipf_free_list6(&ipf_rules6_in);
    }
    if(flags & IPF_FLUSH_OUT) {
        for(r = ipf_global.rules_out; r != NULL; r = r->next)
            count++;
        ipf_free_list(&ipf_global.rules_out);
        count += ipf_free_list6(&ipf_rules6_out);
    }
    ipf_global.rule_count = 0;
    /* Recount if only partial flush */
    if(flags != IPF_FLUSH_ALL) {
        int c = 0;
        for(r = ipf_global.rules_in; r; r = r->next) c++;
        for(r = ipf_global.rules_out; r; r = r->next) c++;
        for(r6 = ipf_rules6_in; r6; r6 = r6->next) c++;
        for(r6 = ipf_rules6_out; r6; r6 = r6->next) c++;
        ipf_global.rule_count = c;
    }
    splx(s);

    return count;
}

/*
 * Match a port against a port comparison.
 */
static int
ipf_match_port(u_int16_t port, u_int8_t op, u_int16_t lo, u_int16_t hi)
{
    switch(op) {
    case IPF_PORT_NONE:
        return 1;
    case IPF_PORT_EQ:
        return (port == lo);
    case IPF_PORT_NE:
        return (port != lo);
    case IPF_PORT_LT:
        return (port < lo);
    case IPF_PORT_GT:
        return (port > lo);
    case IPF_PORT_LE:
        return (port <= lo);
    case IPF_PORT_GE:
        return (port >= lo);
    case IPF_PORT_RANGE:
        return (port > lo && port < hi);
    case IPF_PORT_RANGEINCL:
        return (port >= lo && port <= hi);
    }
    return 0;
}

/*
 * WHAT A RULE IS MATCHED AGAINST, of a packet of either family: filled
 * once per packet by the family's part (ipf_check(), ipf_check6()), read
 * by the part that is the same for both (IPF_MATCH_UPPER, ipf_matched()).
 */
#define IPF_L4MAX   20          /* bytes of the upper-layer header looked at */

struct ipf_pkt {
    struct ifnet   *ifp;
    struct mbuf    *m;
    int             dir;
    u_int8_t        fam;        /* 4, 6 */
    u_int8_t        proto;      /* upper-layer protocol */
    u_int8_t        icmp;       /* proto is the family's ICMP */
    u_int8_t        later;      /* a fragment, not the first: no upper-layer header */
    u_int8_t        hdrs;       /* IPv4 options; IPv6 extension headers */
    u_int8_t        l4len;      /* bytes of the upper-layer header the packet has (<= IPF_L4MAX) */
    u_int8_t        ports;      /* sport, dport are good (TCP or UDP) */
    u_int8_t        tcpflags;   /* when TCP and l4len covers them */
    u_int8_t        tflags;     /* tcpflags is good */
    u_int16_t       sport, dport;
    int16_t         itype, icode;   /* -1 when not ICMP of the family */
    const void     *src, *dst;  /* the addresses, in the packet (for the log) */
    char            ifname[IPF_MAXIFNAME];  /* filled when a rule first names one */
};

/*
 * Up to len bytes from off, without a pull-up: where they are - in the
 * first mbuf when they are there (nearly always: nothing is copied),
 * else copied into buf - and in *got how many the packet has.
 */
static const u_int8_t *
ipf_peek(struct mbuf *m, int off, int len, u_int8_t *buf, int *got)
{
    int have = m->m_pkthdr.len - off;

    *got = 0;
    if(have <= 0)
        return buf;
    if(len > have)
        len = have;
    *got = len;
    if(off + len <= m->m_len)
        return mtod(m, const u_int8_t *) + off;
    m_copydata(m, off, len, (caddr_t)buf);
    return buf;
}

/* the upper-layer header's facts, from l4len bytes at h */
static void
ipf_upper(struct ipf_pkt *pk, const u_int8_t *h)
{
    pk->ports = pk->tflags = 0;
    pk->itype = pk->icode = -1;
    if(pk->later)
        return;
    if((pk->proto == IPPROTO_TCP || pk->proto == IPPROTO_UDP) && pk->l4len >= 4) {
        pk->sport = (h[0] << 8) | h[1];
        pk->dport = (h[2] << 8) | h[3];
        pk->ports = 1;
        if(pk->proto == IPPROTO_TCP && pk->l4len >= 14) {
            pk->tcpflags = h[13];
            pk->tflags = 1;
        }
    } else if(pk->icmp && pk->l4len >= 2) {
        pk->itype = h[0];
        pk->icode = h[1];
    }
}

static int
ipf_ifmatch(struct ipf_pkt *pk, const char *name)
{
    int i;

    if(pk->ifname[0] == '\0')
        if_fullname(pk->ifp, pk->ifname, sizeof(pk->ifname));
    for(i = 0; i < IPF_MAXIFNAME; i++) {
        if(name[i] != pk->ifname[i])
            return 0;
        if(name[i] == '\0')
            return 1;
    }
    return 1;
}

/*
 * The part of a rule that is the same in both structures (the fields have
 * the same names): interface, protocol, ports, TCP flags, ICMP type and
 * code, short packets, options. `continue's the rule loop it is used in
 * when the rule does not match. A rule that names something of the
 * upper-layer header does not match a packet without one (a later
 * fragment, a truncated header).
 */
#define IPF_MATCH_UPPER(r, pk) \
        if((r)->ifname[0] != '\0' && !ipf_ifmatch((pk), (r)->ifname)) \
            continue; \
        if((r)->proto != IPF_PROTO_ANY) { \
            if((r)->proto == IPF_PROTO_TCPUDP) { \
                if((pk)->proto != IPPROTO_TCP && (pk)->proto != IPPROTO_UDP) \
                    continue; \
            } else if((r)->proto != (pk)->proto) \
                continue; \
        } \
        if((r)->sport_op != IPF_PORT_NONE && (!(pk)->ports || \
                !ipf_match_port((pk)->sport, (r)->sport_op, (r)->sport_lo, (r)->sport_hi))) \
            continue; \
        if((r)->dport_op != IPF_PORT_NONE && (!(pk)->ports || \
                !ipf_match_port((pk)->dport, (r)->dport_op, (r)->dport_lo, (r)->dport_hi))) \
            continue; \
        if((r)->tcp_flagmask != 0 && (!(pk)->tflags || \
                ((pk)->tcpflags & (r)->tcp_flagmask) != (r)->tcp_flags)) \
            continue; \
        if((r)->icmp_type >= 0 && ((pk)->itype != (r)->icmp_type || \
                ((r)->icmp_code >= 0 && (pk)->icode != (r)->icmp_code))) \
            continue; \
        if(((r)->rflags & IPF_SHORT) && \
                ((pk)->proto == IPPROTO_TCP || (pk)->proto == IPPROTO_UDP) && \
                ((pk)->later || (pk)->l4len >= 8)) \
            continue; \
        if(((r)->rflags & IPF_IPOPTS) && !(pk)->hdrs) \
            continue

/*
 * A rule matched: its counters, the engine's, the log line. The verdict
 * so far is *result; 1: the rule list ends here (quick).
 */
static int
ipf_matched(struct ipf_pkt *pk, u_int8_t action, u_int16_t rflags,
            u_int64_t *hits, u_int64_t *bytes, int *result)
{
    (*hits)++;
    *bytes += pk->m->m_pkthdr.len;

    switch(action) {
    case IPF_BLOCK:
        *result = IPF_BLOCK;
        ipf_global.blocked++;
        break;
    case IPF_PASS:
        *result = IPF_PASS;
        ipf_global.passed++;
        break;
    case IPF_LOG: {
        char sa[46], da[46];        /* INET6_ADDRSTRLEN */
        int af = pk->fam == 6 ? AF_INET6 : AF_INET;

        ipf_global.logged++;
        if(pk->ifname[0] == '\0')
            if_fullname(pk->ifp, pk->ifname, sizeof(pk->ifname));
        if(__inet_ntop(af, pk->src, sa, sizeof(sa), NULL) == NULL)
            sa[0] = '\0';
        if(__inet_ntop(af, pk->dst, da, sizeof(da), NULL) == NULL)
            da[0] = '\0';
        __log(LOG_INFO, "ipfilter: %s %s %s %s -> %s\n",
              *result == IPF_BLOCK ? "block" : "pass",
              pk->dir == IPF_IN ? "in" : "out", pk->ifname, sa, da);
        break;
    }
    case IPF_COUNT:
    default:
        /* count only, don't change verdict */
        return 0;
    }

    /* Quick: stop processing on first match */
    return (rflags & IPF_QUICK) != 0;
}

/*
 * Check an IPv4 packet against the filter rules.
 * Returns: IPF_PASS (0) to pass, IPF_BLOCK (1) to block.
 *
 * m     - packet mbuf, the IP header in the first one; ip_len and ip_off
 *         in host order (as at all of IPv4's hook points)
 * ifp   - interface
 * dir   - IPF_IN or IPF_OUT
 */
int
ipf_check(struct mbuf *m, struct ifnet *ifp, int dir)
{
    struct ipf_rule *r, *rules;
    struct ip *ip;
    int hlen;
    int result = IPF_PASS;	/* default: pass */
    struct ipf_pkt pk;
    u_int8_t l4[IPF_L4MAX];
    const u_int8_t *h;
    int n;

    if(!ipf_global.enabled)
        return IPF_PASS;
    rules = (dir == IPF_IN) ? ipf_global.rules_in : ipf_global.rules_out;
    if(rules == NULL)
        return IPF_PASS;

    if(m->m_len < sizeof(struct ip))
        return IPF_PASS;

    ip = mtod(m, struct ip *);
    if(ip->ip_v != IPVERSION)
        return IPF_PASS;

    hlen = ip->ip_hl << 2;

    pk.ifp = ifp;
    pk.m = m;
    pk.dir = dir;
    pk.fam = 4;
    pk.proto = ip->ip_p;
    pk.icmp = (ip->ip_p == IPPROTO_ICMP);
    pk.later = (ip->ip_off & IP_OFFMASK) != 0;
    pk.hdrs = hlen > (int)sizeof(struct ip);
    pk.l4len = 0;
    h = l4;
    if(!pk.later) {
        h = ipf_peek(m, hlen, IPF_L4MAX, l4, &n);
        pk.l4len = n;
    }
    pk.src = &ip->ip_src;
    pk.dst = &ip->ip_dst;
    pk.ifname[0] = '\0';
    ipf_upper(&pk, h);

    for(r = rules; r != NULL; r = r->next) {
        /* Source address match */
        if(r->src_addr.s_addr != 0 || r->src_mask.s_addr != 0) {
            int match = ((ip->ip_src.s_addr & r->src_mask.s_addr) ==
                         (r->src_addr.s_addr & r->src_mask.s_addr));
            if(r->src_not) match = !match;
            if(!match) continue;
        }

        /* Destination address match */
        if(r->dst_addr.s_addr != 0 || r->dst_mask.s_addr != 0) {
            int match = ((ip->ip_dst.s_addr & r->dst_mask.s_addr) ==
                         (r->dst_addr.s_addr & r->dst_mask.s_addr));
            if(r->dst_not) match = !match;
            if(!match) continue;
        }

        IPF_MATCH_UPPER(r, &pk);

        if(ipf_matched(&pk, r->action, r->rflags, &r->hits, &r->bytes, &result))
            break;
    }

    return result;
}

/* whether the first plen bits of a and b are the same */
static int
ipf_prefix6(const u_int8_t *a, const u_int8_t *b, int plen)
{
    int n = plen >> 3;

    if(n != 0 && memcmp(a, b, n) != 0)
        return 0;
    if(plen & 7) {
        u_int8_t mask = 0xff << (8 - (plen & 7));

        if((a[n] ^ b[n]) & mask)
            return 0;
    }
    return 1;
}

/*
 * Check an IPv6 packet against the IPv6 rules: its extension headers are
 * walked to the upper-layer protocol (hop-by-hop, routing, destination
 * options, fragment, AH), read in place, or with m_copydata() into a few
 * bytes on the stack where they are not in the first mbuf - no pull-up, no
 * allocation. ESP and anything unknown end the
 * walk: that is the protocol. Same answers as ipf_check().
 */
#define IPF6_MAXHDRS    16      /* extension headers walked at most */

int
ipf_check6(struct mbuf *m, struct ifnet *ifp, int dir)
{
    struct ipf_rule6 *r, *rules;
    struct ip6_hdr *ip6;
    int result = IPF_PASS;
    struct ipf_pkt pk;
    u_int8_t l4[IPF_L4MAX];
    const u_int8_t *h;
    int off, nxt, n;

    if(!ipf_global.enabled)
        return IPF_PASS;
    rules = (dir == IPF_IN) ? ipf_rules6_in : ipf_rules6_out;
    if(rules == NULL)
        return IPF_PASS;

    if(m->m_len < sizeof(struct ip6_hdr))
        return IPF_PASS;
    ip6 = mtod(m, struct ip6_hdr *);
    if((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION)
        return IPF_PASS;

    pk.ifp = ifp;
    pk.m = m;
    pk.dir = dir;
    pk.fam = 6;
    pk.later = 0;
    pk.hdrs = 0;
    pk.l4len = 0;
    pk.src = &ip6->ip6_src;
    pk.dst = &ip6->ip6_dst;
    pk.ifname[0] = '\0';

    off = sizeof(struct ip6_hdr);
    nxt = ip6->ip6_nxt;
    for(n = 0; n < IPF6_MAXHDRS; n++) {
        u_int8_t ebuf[4];
        const u_int8_t *eh;
        int got;

        if(nxt != IPPROTO_HOPOPTS && nxt != IPPROTO_ROUTING && nxt != IPPROTO_DSTOPTS &&
                nxt != IPPROTO_FRAGMENT && nxt != IPPROTO_AH)
            break;
        pk.hdrs = 1;
        eh = ipf_peek(m, off, 4, ebuf, &got);
        if(got < 4) {
            off = -1;           /* truncated: no upper-layer header */
            break;
        }
        if(nxt == IPPROTO_FRAGMENT) {
            /* ip6f_nxt, reserved, ip6f_offlg (offset in its upper 13 bits) */
            if(((eh[2] << 8) | eh[3]) & 0xfff8) {
                pk.later = 1;
                nxt = eh[0];
                break;
            }
            off += sizeof(struct ip6_frag);
        } else if(nxt == IPPROTO_AH)
            off += (eh[1] + 2) << 2;
        else
            off += (eh[1] + 1) << 3;
        nxt = eh[0];
    }
    pk.proto = nxt;
    pk.icmp = (nxt == IPPROTO_ICMPV6);
    h = l4;
    if(!pk.later && off > 0 && n < IPF6_MAXHDRS) {
        int got;

        h = ipf_peek(m, off, IPF_L4MAX, l4, &got);
        pk.l4len = got;
    }
    ipf_upper(&pk, h);

    for(r = rules; r != NULL; r = r->next) {
        if(r->src_plen != 0 &&
                ipf_prefix6((const u_int8_t *)&ip6->ip6_src,
                            (const u_int8_t *)&r->src_addr, r->src_plen) == (r->src_not != 0))
            continue;
        if(r->dst_plen != 0 &&
                ipf_prefix6((const u_int8_t *)&ip6->ip6_dst,
                            (const u_int8_t *)&r->dst_addr, r->dst_plen) == (r->dst_not != 0))
            continue;

        IPF_MATCH_UPPER(r, &pk);

        if(ipf_matched(&pk, r->action, r->rflags, &r->hits, &r->bytes, &result))
            break;
    }

    return result;
}

/*
 * The hooks (net/pfil.h): a blocked packet is not freed here - pfil_run()
 * does it - and the send that made it fails with EHOSTUNREACH, as it
 * always did for IPv4.
 */
static int
ipf_hook4(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir)
{
    return (ipf_check(*mp, ifp, dir) == IPF_PASS ? 0 : EHOSTUNREACH);
}

static int
ipf_hook6(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir)
{
    return (ipf_check6(*mp, ifp, dir) == IPF_PASS ? 0 : EHOSTUNREACH);
}

/* --- NAT functions --- */

/*
 * The NAT rules are stored, counted and reported; nothing translates a
 * packet by them yet (no code makes a struct ipnat_entry).
 */
int
ipnat_add_rule(struct ipnat_rule *template)
{
    struct ipnat_rule *rule, **tailp;
    spl_t s;

    rule = bsd_malloc(sizeof(*rule), NULL, NULL);
    if(rule == NULL)
        return ENOMEM;

    memcpy(rule, template, sizeof(*rule));
    rule->next = NULL;
    rule->hits = 0;

    s = splnet();
    tailp = &ipnat_global.rules;
    while(*tailp != NULL)
        tailp = &(*tailp)->next;
    *tailp = rule;
    ipnat_global.rule_count++;
    splx(s);

    return 0;
}

/*
 * Remove a matching NAT rule.
 */
int
ipnat_remove_rule(struct ipnat_rule *template)
{
    struct ipnat_rule **pp, *r;
    spl_t s;

    s = splnet();
    for(pp = &ipnat_global.rules; *pp != NULL; pp = &(*pp)->next) {
        r = *pp;
        if(r->type == template->type &&
                r->match_addr.s_addr == template->match_addr.s_addr &&
                r->match_mask.s_addr == template->match_mask.s_addr &&
                r->nat_addr.s_addr == template->nat_addr.s_addr &&
                strcmp(r->ifname, template->ifname) == 0) {
            *pp = r->next;
            bsd_free(r, NULL);
            ipnat_global.rule_count--;
            splx(s);
            return 0;
        }
    }
    splx(s);
    return ESRCH;
}

int
ipnat_clear_rules(void)
{
    struct ipnat_rule *r, *next;
    spl_t s;
    int count = 0;

    s = splnet();
    for(r = ipnat_global.rules; r != NULL; r = next) {
        next = r->next;
        bsd_free(r, NULL);
        count++;
    }
    ipnat_global.rules = NULL;
    ipnat_global.rule_count = 0;
    splx(s);

    return count;
}

int
ipnat_flush_entries(void)
{
    struct ipnat_entry *e, *next;
    spl_t s;
    int count = 0;

    s = splnet();
    for(e = ipnat_global.entries; e != NULL; e = next) {
        next = e->next;
        bsd_free(e, NULL);
        count++;
    }
    ipnat_global.entries = NULL;
    ipnat_global.entry_count = 0;
    splx(s);

    return count;
}

#endif /* ENABLE_PACKET_FILTER */
