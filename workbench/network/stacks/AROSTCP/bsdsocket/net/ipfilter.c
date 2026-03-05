/*
 * net/ipfilter.c - IP Filter (ipf) kernel engine for AROSTCP
 *
 * Implements packet filtering rules evaluated in the pfil hook path.
 * Compatible with Roadshow ipf rule syntax.
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
#include "ipfilter.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include <string.h>
#include <stdio.h>

/* Global filter and NAT state */
struct ipf_state ipf_global;
struct ipnat_state ipnat_global;

void
ipf_init(void)
{
    memset(&ipf_global, 0, sizeof(ipf_global));
    ipf_global.enabled = 0;
    ipf_global.rules_in = NULL;
    ipf_global.rules_out = NULL;

    memset(&ipnat_global, 0, sizeof(ipnat_global));
    ipnat_global.rules = NULL;
    ipnat_global.entries = NULL;

    __log(LOG_INFO, "ipfilter: initialized\n");
}

void
ipf_cleanup(void)
{
    ipf_flush(IPF_FLUSH_ALL);
    ipnat_clear_rules();
    ipnat_flush_entries();
    __log(LOG_INFO, "ipfilter: cleaned up\n");
}

int
ipf_enabled(void)
{
    return ipf_global.enabled;
}

void
ipf_enable(void)
{
    ipf_global.enabled = 1;
    __log(LOG_INFO, "ipfilter: enabled\n");
}

void
ipf_disable(void)
{
    ipf_global.enabled = 0;
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

int
ipf_flush(int flags)
{
    spl_t s;
    int count = 0;
    struct ipf_rule *r;

    s = splnet();
    if(flags & IPF_FLUSH_IN) {
        for(r = ipf_global.rules_in; r != NULL; r = r->next)
            count++;
        ipf_free_list(&ipf_global.rules_in);
    }
    if(flags & IPF_FLUSH_OUT) {
        for(r = ipf_global.rules_out; r != NULL; r = r->next)
            count++;
        ipf_free_list(&ipf_global.rules_out);
    }
    ipf_global.rule_count = 0;
    /* Recount if only partial flush */
    if(flags != IPF_FLUSH_ALL) {
        int c = 0;
        for(r = ipf_global.rules_in; r; r = r->next) c++;
        for(r = ipf_global.rules_out; r; r = r->next) c++;
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
 * Check a packet against the filter rules.
 * Returns: IPF_PASS (0) to pass, IPF_BLOCK (1) to block.
 *
 * m     - packet mbuf
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
    u_int16_t sport = 0, dport = 0;
    char ifname[IPF_MAXIFNAME];

    if(!ipf_global.enabled)
        return IPF_PASS;

    if(m->m_len < sizeof(struct ip))
        return IPF_PASS;

    ip = mtod(m, struct ip *);
    if(ip->ip_v != IPVERSION)
        return IPF_PASS;

    hlen = ip->ip_hl << 2;

    /* Extract port numbers for TCP/UDP */
    if((ip->ip_p == IPPROTO_TCP || ip->ip_p == IPPROTO_UDP) &&
            m->m_len >= hlen + 4) {
        u_int16_t *ports = (u_int16_t *)((caddr_t)ip + hlen);
        sport = ntohs(ports[0]);
        dport = ntohs(ports[1]);
    }

    /* Build interface name */
    snprintf(ifname, sizeof(ifname), "%s%d", ifp->if_name, ifp->if_unit);

    rules = (dir == IPF_IN) ? ipf_global.rules_in : ipf_global.rules_out;

    for(r = rules; r != NULL; r = r->next) {
        /* Interface match */
        if(r->ifname[0] != '\0' &&
                strcmp(r->ifname, ifname) != 0)
            continue;

        /* Protocol match */
        if(r->proto != IPF_PROTO_ANY) {
            if(r->proto == IPF_PROTO_TCPUDP) {
                if(ip->ip_p != IPPROTO_TCP &&
                        ip->ip_p != IPPROTO_UDP)
                    continue;
            } else if(r->proto != ip->ip_p) {
                continue;
            }
        }

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

        /* Source port match */
        if(r->sport_op != IPF_PORT_NONE) {
            if(ip->ip_p != IPPROTO_TCP && ip->ip_p != IPPROTO_UDP)
                continue;
            if(!ipf_match_port(sport, r->sport_op,
                               r->sport_lo, r->sport_hi))
                continue;
        }

        /* Destination port match */
        if(r->dport_op != IPF_PORT_NONE) {
            if(ip->ip_p != IPPROTO_TCP && ip->ip_p != IPPROTO_UDP)
                continue;
            if(!ipf_match_port(dport, r->dport_op,
                               r->dport_lo, r->dport_hi))
                continue;
        }

        /* TCP flags match */
        if(r->tcp_flagmask != 0) {
            struct tcphdr *th;
            u_int8_t flags;
            if(ip->ip_p != IPPROTO_TCP)
                continue;
            if(m->m_len < hlen + (int)sizeof(struct tcphdr))
                continue;
            th = (struct tcphdr *)((caddr_t)ip + hlen);
            flags = th->th_flags;
            if((flags & r->tcp_flagmask) != r->tcp_flags)
                continue;
        }

        /* ICMP type match */
        if(r->icmp_type >= 0) {
            struct icmp *ic;
            if(ip->ip_p != IPPROTO_ICMP)
                continue;
            if(m->m_len < hlen + 2)
                continue;
            ic = (struct icmp *)((caddr_t)ip + hlen);
            if(ic->icmp_type != r->icmp_type)
                continue;
            if(r->icmp_code >= 0 && ic->icmp_code != r->icmp_code)
                continue;
        }

        /* Short packet match */
        if(r->rflags & IPF_SHORT) {
            if((ip->ip_p == IPPROTO_TCP || ip->ip_p == IPPROTO_UDP) &&
                    m->m_len >= hlen + 8)
                continue;	/* not short */
        }

        /* IP options match */
        if(r->rflags & IPF_IPOPTS) {
            if(hlen <= (int)sizeof(struct ip))
                continue;	/* no options */
        }

        /* --- Rule matched --- */
        r->hits++;
        r->bytes += m->m_pkthdr.len;

        if(r->action == IPF_BLOCK) {
            result = IPF_BLOCK;
            ipf_global.blocked++;
        } else if(r->action == IPF_PASS) {
            result = IPF_PASS;
            ipf_global.passed++;
        } else if(r->action == IPF_LOG) {
            ipf_global.logged++;
            {
                u_int32_t sa = ntohl(ip->ip_src.s_addr);
                u_int32_t da = ntohl(ip->ip_dst.s_addr);
                __log(LOG_INFO,
                      "ipfilter: %s %s %s %u.%u.%u.%u -> %u.%u.%u.%u\n",
                      result == IPF_BLOCK ? "block" : "pass",
                      dir == IPF_IN ? "in" : "out",
                      ifname,
                      (sa >> 24) & 0xff, (sa >> 16) & 0xff,
                      (sa >> 8) & 0xff, sa & 0xff,
                      (da >> 24) & 0xff, (da >> 16) & 0xff,
                      (da >> 8) & 0xff, da & 0xff);
            }
        } else if(r->action == IPF_COUNT) {
            /* count only, don't change verdict */
            continue;
        }

        /* Quick: stop processing on first match */
        if(r->rflags & IPF_QUICK)
            return result;
    }

    return result;
}

/* --- NAT functions --- */

void
ipnat_init(void)
{
    /* Already done by ipf_init memset */
}

void
ipnat_cleanup(void)
{
    ipnat_clear_rules();
    ipnat_flush_entries();
}

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
