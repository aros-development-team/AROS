/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - network diagnostics: NET STATUS / INTERFACES / ROUTES / STATS.

    Sources, all public bsdsocket.library / miami.library entry points:
      interfaces  IoctlSocket(SIOCGIFCONF + SIOCGIF*) exactly as C:ifconfig does,
                  plus QueryInterfaceTagList() for the per-interface counters
      routes      GetRouteInfo()/FreeRouteInfo() (the stack's routing table dump)
      statistics  MiamiSysCtl() over net.inet.{ip,tcp,udp}.*_STATS, which hands
                  back copies of the stack's struct ipstat/tcpstat/udpstat
      status      SocketBaseTags(SBTC_RELEASESTRPTR), gethostname(),
                  GetDefaultDomainName(), ObtainDomainNameServerList()
    NET SOCKETS is deliberately not implemented: the stack has no supported
    socket enumeration (GetNetworkStatistics() is a stub; C:netstat reads the
    stack's memory through libkvm, which this agent refuses to do).
*/

#include "arda.h"

/* Enable the Roadshow-style extensions of bsdsocket.library's headers. */
#define BSDSOCKET_ROADSHOW_ROUTING
#define BSDSOCKET_ROADSHOW_IFCFG
#define BSDSOCKET_ROADSHOW_DNS

#include <proto/exec.h>
#include <proto/bsdsocket.h>
/* proto/miami.h is deliberately not included: its generated macros clash with
   bsdsocket's QueryInterfaceTagList() and its MiamiSysCtl() macro is malformed
   (the _WB helper takes one parameter).  MiamiSysCtl is the only miami.library
   call used, so it is invoked through its LVO directly. */
#include <aros/libcall.h>

/* defines/bsdsocket.h has no inline stub for GetDefaultDomainName(); call LVO 117 directly. */
#define GetDefaultDomainName(buf, size) \
    AROS_LC2(LONG, GetDefaultDomainName, \
             AROS_LCA(STRPTR, (buf), A0), \
             AROS_LCA(LONG, (size), D0), \
             struct Library *, SocketBase, 117, BSDSocket)

#define MiamiSysCtl(n, nl, op, olp, np, nlen, l) \
    AROS_LC7(int, MiamiSysCtl, \
             AROS_LCA(LONG *, (n), A0), \
             AROS_LCA(ULONG, (nl), D0), \
             AROS_LCA(void *, (op), A1), \
             AROS_LCA(LONG *, (olp), A2), \
             AROS_LCA(void *, (np), A3), \
             AROS_LCA(LONG, (nlen), D1), \
             AROS_LCA(int, (l), D2), \
             struct Library *, MiamiBase, 5, Miami)

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <arpa/inet.h>
#include <libraries/bsdsocket.h>
#include <bsdsocket/socketbasetags.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

extern struct Library *SocketBase;
struct Library *MiamiBase = NULL;

#define MAX_IFACES 32

/* ---- helpers ---------------------------------------------------------- */

static void fmt_in4(ULONG s_addr, char *out, ULONG cap)
{
    const char *s = Inet_NtoA(s_addr);
    arda_safe_str(out, cap, s ? s : "?");
}

static void fmt_in6(const UBYTE *b, char *out, ULONG cap)
{
    snprintf(out, cap, "%x:%x:%x:%x:%x:%x:%x:%x",
             (b[0] << 8) | b[1], (b[2] << 8) | b[3], (b[4] << 8) | b[5], (b[6] << 8) | b[7],
             (b[8] << 8) | b[9], (b[10] << 8) | b[11], (b[12] << 8) | b[13], (b[14] << 8) | b[15]);
}

static void fmt_hw(const UBYTE *b, int len, char *out, ULONG cap)
{
    ULONG pos = 0;
    int i;

    out[0] = '\0';
    for (i = 0; i < len && pos + 4 < cap; i++)
        pos += snprintf(out + pos, cap - pos, "%s%02x", i ? ":" : "", b[i]);
}

/* Format any sockaddr for a record field; short netmask sockaddrs are handled. */
static void fmt_sockaddr(const struct sockaddr *sa, char *out, ULONG cap)
{
    if (!sa || sa->sa_len == 0)
    {
        strcpy(out, "-");
        return;
    }
    switch (sa->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        memcpy(&sin, sa, sa->sa_len < sizeof(sin) ? sa->sa_len : sizeof(sin));
        fmt_in4(sin.sin_addr.s_addr, out, cap);
        break;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 sin6;
        memset(&sin6, 0, sizeof(sin6));
        memcpy(&sin6, sa, sa->sa_len < sizeof(sin6) ? sa->sa_len : sizeof(sin6));
        fmt_in6(sin6.sin6_addr.s6_addr, out, cap);
        break;
    }
    case AF_LINK:
    {
        const struct sockaddr_dl *sdl = (const struct sockaddr_dl *)sa;
        if (sdl->sdl_alen)
            fmt_hw((const UBYTE *)LLADDR(sdl), sdl->sdl_alen, out, cap);
        else
            snprintf(out, cap, "link#%u", sdl->sdl_index);
        break;
    }
    case AF_UNSPEC:
        /* netmask of a default route etc. */
        strcpy(out, "0.0.0.0");
        break;
    default:
        snprintf(out, cap, "af%u", sa->sa_family);
        break;
    }
}

/* Netmasks come back as short, often family-less sockaddrs: interpret them
   with the family of the address they belong to. */
static void fmt_mask(const struct sockaddr *sa, int family, char *out, ULONG cap)
{
    UBYTE raw[sizeof(struct sockaddr_in6)];
    ULONG n;

    memset(raw, 0, sizeof(raw));
    if (sa && sa->sa_len)
    {
        n = sa->sa_len < sizeof(raw) ? sa->sa_len : sizeof(raw);
        memcpy(raw, sa, n);
    }
    if (family == AF_INET6)
        fmt_in6(((struct sockaddr_in6 *)raw)->sin6_addr.s6_addr, out, cap);
    else
        fmt_in4(((struct sockaddr_in *)raw)->sin_addr.s_addr, out, cap);
}

struct FlagName { ULONG bit; const char *name; };

static void fmt_flags(ULONG flags, const struct FlagName *t, char *out, ULONG cap)
{
    ULONG pos = 0;

    out[0] = '\0';
    for (; t->name; t++)
    {
        if ((flags & t->bit) && pos + strlen(t->name) + 2 < cap)
            pos += snprintf(out + pos, cap - pos, "%s%s", pos ? "," : "", t->name);
    }
    if (!pos)
        strcpy(out, "-");
}

static const struct FlagName if_flags[] =
{
    { IFF_UP, "UP" }, { IFF_BROADCAST, "BROADCAST" }, { IFF_DEBUG, "DEBUG" },
    { IFF_LOOPBACK, "LOOPBACK" }, { IFF_POINTOPOINT, "POINTOPOINT" },
    { IFF_DRV_RUNNING, "RUNNING" }, { IFF_NOARP, "NOARP" }, { IFF_PROMISC, "PROMISC" },
    { IFF_ALLMULTI, "ALLMULTI" }, { IFF_DRV_OACTIVE, "OACTIVE" }, { IFF_SIMPLEX, "SIMPLEX" },
    { IFF_MULTICAST, "MULTICAST" }, { 0, NULL }
};

static const struct FlagName rt_flags[] =
{
    { RTF_UP, "U" }, { RTF_GATEWAY, "G" }, { RTF_HOST, "H" }, { RTF_REJECT, "R" },
    { RTF_DYNAMIC, "D" }, { RTF_MODIFIED, "M" }, { RTF_CLONING, "C" }, { RTF_XRESOLVE, "X" },
    { RTF_LLINFO, "L" }, { RTF_STATIC, "S" }, { RTF_BLACKHOLE, "B" }, { 0, NULL }
};

/* Collect the unique interface names (and link-level addresses) via SIOCGIFCONF. */
struct IfEntry { char name[IFNAMSIZ]; char hwaddr[64]; };

static int collect_ifaces(int s, struct IfEntry *out, int max)
{
    static char inbuf[8192];
    struct ifconf ifc;
    int i, n = 0;

    ifc.ifc_len = sizeof(inbuf);
    ifc.ifc_buf = inbuf;
    if (IoctlSocket(s, SIOCGIFCONF, (char *)&ifc) < 0)
        return -1;

    for (i = 0; i < ifc.ifc_len; )
    {
        struct ifreq *ifr = (struct ifreq *)((char *)ifc.ifc_req + i);
        int k;

        i += sizeof(ifr->ifr_name) +
             (ifr->ifr_addr.sa_len > sizeof(struct sockaddr) ? ifr->ifr_addr.sa_len
                                                              : sizeof(struct sockaddr));
        for (k = 0; k < n; k++)
            if (strncmp(out[k].name, ifr->ifr_name, IFNAMSIZ) == 0)
                break;
        if (k == n)
        {
            if (n >= max)
                continue;
            memset(&out[n], 0, sizeof(out[n]));
            strncpy(out[n].name, ifr->ifr_name, IFNAMSIZ - 1);
            strcpy(out[n].hwaddr, "-");
            n++;
        }
        if (ifr->ifr_addr.sa_family == AF_LINK)
        {
            const struct sockaddr_dl *sdl = (const struct sockaddr_dl *)&ifr->ifr_addr;
            if (sdl->sdl_alen)
                fmt_hw((const UBYTE *)LLADDR(sdl), sdl->sdl_alen, out[k].hwaddr, sizeof(out[k].hwaddr));
        }
    }
    return n;
}

/* ---- NET INTERFACES --------------------------------------------------- */

static void cmd_net_interfaces(struct Arda *a)
{
    static struct IfEntry ifs[MAX_IFACES];
    struct ArdaResp *r = &a->resp;
    int s, s6, n, i;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        arda_resp_err(r, 500, "socket() failed");
        return;
    }
    s6 = socket(AF_INET6, SOCK_DGRAM, 0);            /* may fail: IPv6 optional */

    n = collect_ifaces(s, ifs, MAX_IFACES);
    if (n < 0)
    {
        CloseSocket(s);
        if (s6 >= 0) CloseSocket(s6);
        arda_resp_err(r, 500, "SIOCGIFCONF failed");
        return;
    }

    arda_resp_begin(r);
    arda_resp_line(r, "COUNT=%d", n);
    for (i = 0; i < n; i++)
    {
        struct ifreq ifr;
        char flagnames[160], addr[48], mask[48], brd[48], addr6[48], name[IFNAMSIZ];
        ULONG flags = 0, mtu = 0, metric = 0;
        ULONG rxpkt = 0, txpkt = 0, noproto = 0;
        LONG state = -1;
        SBQUAD_T rxbytes = { 0, 0 }, txbytes = { 0, 0 };

        strcpy(addr, "-"); strcpy(mask, "-"); strcpy(brd, "-"); strcpy(addr6, "-");
        arda_safe_str(name, sizeof(name), ifs[i].name);

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifs[i].name, IFNAMSIZ - 1);
        if (IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0)
            flags = (UWORD)ifr.ifr_flags;
        if (IoctlSocket(s, SIOCGIFMTU, (char *)&ifr) == 0)
            mtu = ifr.ifr_mtu;
        if (IoctlSocket(s, SIOCGIFMETRIC, (char *)&ifr) == 0)
            metric = ifr.ifr_metric;
        if (IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0)
            fmt_sockaddr(&ifr.ifr_addr, addr, sizeof(addr));
        if (IoctlSocket(s, SIOCGIFNETMASK, (char *)&ifr) == 0)
            fmt_mask(&ifr.ifr_addr, AF_INET, mask, sizeof(mask));
        if ((flags & IFF_BROADCAST) && IoctlSocket(s, SIOCGIFBRDADDR, (char *)&ifr) == 0)
            fmt_sockaddr(&ifr.ifr_broadaddr, brd, sizeof(brd));
        else if ((flags & IFF_POINTOPOINT) && IoctlSocket(s, SIOCGIFDSTADDR, (char *)&ifr) == 0)
            fmt_sockaddr(&ifr.ifr_dstaddr, brd, sizeof(brd));

        if (s6 >= 0)
        {
            struct in6_ifreq r6;
            memset(&r6, 0, sizeof(r6));
            strncpy(r6.ifr_name, ifs[i].name, IFNAMSIZ - 1);
            if (IoctlSocket(s6, SIOCGIFADDR_IN6, (char *)&r6) == 0)
                fmt_in6(r6.ifr_addr.sin6_addr.s6_addr, addr6, sizeof(addr6));
        }

        {
            struct TagItem tags[] =
            {
                { IFQ_PacketsReceived, (IPTR)&rxpkt   },
                { IFQ_PacketsSent,     (IPTR)&txpkt   },
                { IFQ_UnknownTypes,    (IPTR)&noproto },
                { IFQ_GetBytesIn,      (IPTR)&rxbytes },
                { IFQ_GetBytesOut,     (IPTR)&txbytes },
                { IFQ_State,           (IPTR)&state   },
                { TAG_DONE,            0              }
            };
            QueryInterfaceTagList(ifs[i].name, tags);
        }

        fmt_flags(flags, if_flags, flagnames, sizeof(flagnames));
        arda_resp_line(r, "IFACE NAME=%s FLAGS=0x%04lx FLAGNAMES=%s MTU=%lu METRIC=%lu ADDR=%s NETMASK=%s"
                          " BROADCAST=%s ADDR6=%s HWADDR=%s STATE=%s RXPKT=%lu TXPKT=%lu RXBYTES=%lu"
                          " TXBYTES=%lu NOPROTO=%lu",
                       name, (unsigned long)flags, flagnames, (unsigned long)mtu, (unsigned long)metric,
                       addr, mask, brd, addr6, ifs[i].hwaddr,
                       state == SM_Up ? "up" : state == SM_Down ? "down" : "unknown",
                       (unsigned long)rxpkt, (unsigned long)txpkt, (unsigned long)rxbytes.sbq_Low,
                       (unsigned long)txbytes.sbq_Low, (unsigned long)noproto);
    }
    arda_resp_end(r);

    CloseSocket(s);
    if (s6 >= 0)
        CloseSocket(s6);
}

/* ---- NET ROUTES ------------------------------------------------------- */

#define RT_ROUNDUP(n) ((n) > 0 ? (1 + (((n) - 1) | (sizeof(long) - 1))) : sizeof(long))

static ULONG route_lines(struct ArdaResp *r, int af, const char *afname)
{
    struct rt_msghdr *buf = GetRouteInfo(af, 0), *rtm;
    char *next;
    ULONG count = 0;

    if (!buf)
        return 0;

    for (next = (char *)buf; ; next += rtm->rtm_msglen)
    {
        struct sockaddr *sa_dst = NULL, *sa_gw = NULL, *sa_mask = NULL;
        char *cp;
        int bit;

        rtm = (struct rt_msghdr *)next;
        if (rtm->rtm_msglen == 0)
            break;
        count++;
        if (!r)
            continue;

        cp = (char *)(rtm + 1);
        for (bit = 1; bit; bit <<= 1)
        {
            if (bit & rtm->rtm_addrs)
            {
                struct sockaddr *sa = (struct sockaddr *)cp;
                if (bit == RTA_DST)          sa_dst  = sa;
                else if (bit == RTA_GATEWAY) sa_gw   = sa;
                else if (bit == RTA_NETMASK) sa_mask = sa;
                cp += RT_ROUNDUP(sa->sa_len);
            }
        }
        {
            char dst[48], gw[48], mask[48], flagnames[48];

            fmt_sockaddr(sa_dst, dst, sizeof(dst));
            fmt_sockaddr(sa_gw, gw, sizeof(gw));
            if (rtm->rtm_flags & RTF_HOST)
                strcpy(mask, "host");
            else
                fmt_mask(sa_mask, sa_dst ? sa_dst->sa_family : af, mask, sizeof(mask));
            fmt_flags(rtm->rtm_flags, rt_flags, flagnames, sizeof(flagnames));
            arda_resp_line(r, "ROUTE AF=%s DST=%s MASK=%s GW=%s FLAGS=0x%lx FLAGNAMES=%s IFINDEX=%u USE=%lu MTU=%lu EXPIRE=%lu",
                           afname, dst, mask, gw, (unsigned long)rtm->rtm_flags, flagnames,
                           rtm->rtm_index, (unsigned long)rtm->rtm_use,
                           (unsigned long)rtm->rtm_rmx.rmx_mtu, (unsigned long)rtm->rtm_rmx.rmx_expire);
        }
    }
    FreeRouteInfo(buf);
    return count;
}

static void cmd_net_routes(struct Arda *a)
{
    struct ArdaResp *r = &a->resp;
    ULONG n4, n6;

    arda_resp_begin(r);
    n4 = route_lines(NULL, AF_INET, "inet");
    n6 = route_lines(NULL, AF_INET6, "inet6");
    arda_resp_line(r, "COUNT=%lu", (unsigned long)(n4 + n6));
    arda_resp_line(r, "COUNT_INET=%lu", (unsigned long)n4);
    arda_resp_line(r, "COUNT_INET6=%lu", (unsigned long)n6);
    route_lines(r, AF_INET, "inet");
    route_lines(r, AF_INET6, "inet6");
    arda_resp_end(r);
}

/* ---- NET ARP ---------------------------------------------------------- */

#define MAX_ARP 64

static const struct FlagName arp_flags[] =
{
    { ATF_COM, "COMPLETE" }, { ATF_PERM, "PERMANENT" }, { ATF_PUBL, "PUBLISHED" }, { 0, NULL }
};

static void cmd_net_arp(struct Arda *a)
{
    static struct arpreq table[MAX_ARP];
    struct ArdaResp *r = &a->resp;
    struct arptabreq atr;
    struct sockaddr_in *sin;
    int s, n, i;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        arda_resp_err(r, 500, "socket() failed");
        return;
    }
    memset(&atr, 0, sizeof(atr));
    memset(table, 0, sizeof(table));
    sin = (struct sockaddr_in *)&atr.atr_arpreq.arp_pa;
    sin->sin_len    = sizeof(*sin);
    sin->sin_family = AF_INET;
    atr.atr_size  = MAX_ARP;
    atr.atr_table = table;
    if (IoctlSocket(s, SIOCGARPT, (char *)&atr) < 0)
    {
        CloseSocket(s);
        arda_resp_err(r, 500, "SIOCGARPT failed");
        return;
    }
    CloseSocket(s);

    n = atr.atr_inuse < MAX_ARP ? atr.atr_inuse : MAX_ARP;
    arda_resp_begin(r);
    arda_resp_line(r, "COUNT=%d", n);
    arda_resp_line(r, "INUSE=%ld", (long)atr.atr_inuse);
    for (i = 0; i < n; i++)
    {
        struct arpreq *ar = &table[i];
        char addr[48], hw[64], flagnames[48];
        int hwlen = (int)ar->arp_ha.sa_len - 2;

        sin = (struct sockaddr_in *)&ar->arp_pa;
        fmt_in4(sin->sin_addr.s_addr, addr, sizeof(addr));
        if ((ar->arp_flags & ATF_COM) && hwlen > 0 && hwlen <= MAXADDRARP)
            fmt_hw((const UBYTE *)ar->arp_ha.sa_data, hwlen, hw, sizeof(hw));
        else
            strcpy(hw, "incomplete");
        fmt_flags(ar->arp_flags, arp_flags, flagnames, sizeof(flagnames));
        arda_resp_line(r, "ARP ADDR=%s HWADDR=%s FLAGS=0x%x FLAGNAMES=%s", addr, hw, ar->arp_flags, flagnames);
    }
    arda_resp_end(r);
}

/* ---- NET STATS -------------------------------------------------------- */

static BOOL sysctl_stats(int proto, int id, void *buf, LONG len, LONG *got)
{
    LONG name[4] = { CTL_NET, PF_INET, proto, id };
    LONG oldlen = len;
    int err;

    memset(buf, 0, len);
    err = MiamiSysCtl(name, 4, buf, &oldlen, NULL, 0, 0);
    *got = oldlen;
    return err == 0;
}

#define STAT(prefix, field, name) arda_resp_line(r, prefix "_" name "=%lu", (unsigned long)(field))

static void cmd_net_stats(struct Arda *a, const char *which)
{
    struct ArdaResp *r = &a->resp;
    BOOL want_ip, want_tcp, want_udp;
    LONG got;

    if (!*which || !strcasecmp(which, "ALL"))
        want_ip = want_tcp = want_udp = TRUE;
    else
    {
        want_ip  = !strcasecmp(which, "IP");
        want_tcp = !strcasecmp(which, "TCP");
        want_udp = !strcasecmp(which, "UDP");
        if (!(want_ip || want_tcp || want_udp))
        {
            arda_resp_err(r, 400, "usage: NET STATS [IP|TCP|UDP|ALL]");
            return;
        }
    }

    MiamiBase = OpenLibrary("miami.library", 0);
    if (!MiamiBase)
    {
        arda_resp_err(r, 501, "miami.library not available: the stack's sysctl MIB is needed for statistics");
        return;
    }

    arda_resp_begin(r);
    if (want_ip)
    {
        struct ipstat st;
        if (sysctl_stats(IPPROTO_IP, IPCTL_STATS, &st, sizeof(st), &got) && got == sizeof(st))
        {
            STAT("IP", st.ips_total, "TOTAL");         STAT("IP", st.ips_delivered, "DELIVERED");
            STAT("IP", st.ips_localout, "LOCALOUT");   STAT("IP", st.ips_forward, "FORWARDED");
            STAT("IP", st.ips_cantforward, "CANTFORWARD"); STAT("IP", st.ips_noroute, "NOROUTE");
            STAT("IP", st.ips_badsum, "BADSUM");       STAT("IP", st.ips_tooshort, "TOOSHORT");
            STAT("IP", st.ips_toosmall, "TOOSMALL");   STAT("IP", st.ips_badhlen, "BADHLEN");
            STAT("IP", st.ips_badlen, "BADLEN");       STAT("IP", st.ips_noproto, "NOPROTO");
            STAT("IP", st.ips_fragments, "FRAGMENTS"); STAT("IP", st.ips_fragdropped, "FRAGDROPPED");
            STAT("IP", st.ips_fragtimeout, "FRAGTIMEOUT"); STAT("IP", st.ips_reassembled, "REASSEMBLED");
            STAT("IP", st.ips_fragmented, "FRAGMENTED"); STAT("IP", st.ips_ofragments, "OFRAGMENTS");
            STAT("IP", st.ips_cantfrag, "CANTFRAG");   STAT("IP", st.ips_odropped, "ODROPPED");
            STAT("IP", st.ips_redirectsent, "REDIRECTSENT");
        }
        else
            arda_resp_line(r, "IP_STATS=unavailable (sysctl returned %ld bytes, expected %lu)", (long)got, (unsigned long)sizeof(st));
    }
    if (want_tcp)
    {
        struct tcpstat st;
        if (sysctl_stats(IPPROTO_TCP, TCPCTL_STATS, &st, sizeof(st), &got) && got == sizeof(st))
        {
            STAT("TCP", st.tcps_connattempt, "CONNATTEMPT"); STAT("TCP", st.tcps_accepts, "ACCEPTS");
            STAT("TCP", st.tcps_connects, "CONNECTS");       STAT("TCP", st.tcps_drops, "DROPS");
            STAT("TCP", st.tcps_conndrops, "CONNDROPS");     STAT("TCP", st.tcps_closed, "CLOSED");
            STAT("TCP", st.tcps_sndtotal, "SNDTOTAL");       STAT("TCP", st.tcps_sndpack, "SNDPACK");
            STAT("TCP", st.tcps_sndbyte, "SNDBYTE");         STAT("TCP", st.tcps_sndrexmitpack, "SNDREXMITPACK");
            STAT("TCP", st.tcps_rcvtotal, "RCVTOTAL");       STAT("TCP", st.tcps_rcvpack, "RCVPACK");
            STAT("TCP", st.tcps_rcvbyte, "RCVBYTE");         STAT("TCP", st.tcps_rcvbadsum, "RCVBADSUM");
            STAT("TCP", st.tcps_rcvduppack, "RCVDUPPACK");   STAT("TCP", st.tcps_rexmttimeo, "REXMTTIMEO");
            STAT("TCP", st.tcps_keeptimeo, "KEEPTIMEO");     STAT("TCP", st.tcps_keepdrops, "KEEPDROPS");
            STAT("TCP", st.tcps_persisttimeo, "PERSISTTIMEO");
        }
        else
            arda_resp_line(r, "TCP_STATS=unavailable (sysctl returned %ld bytes, expected %lu)", (long)got, (unsigned long)sizeof(st));
    }
    if (want_udp)
    {
        struct udpstat st;
        if (sysctl_stats(IPPROTO_UDP, UDPCTL_STATS, &st, sizeof(st), &got) && got == sizeof(st))
        {
            STAT("UDP", st.udps_ipackets, "IPACKETS");  STAT("UDP", st.udps_opackets, "OPACKETS");
            STAT("UDP", st.udps_hdrops, "HDROPS");      STAT("UDP", st.udps_badsum, "BADSUM");
            STAT("UDP", st.udps_badlen, "BADLEN");      STAT("UDP", st.udps_noport, "NOPORT");
            STAT("UDP", st.udps_noportbcast, "NOPORTBCAST"); STAT("UDP", st.udps_fullsock, "FULLSOCK");
        }
        else
            arda_resp_line(r, "UDP_STATS=unavailable (sysctl returned %ld bytes, expected %lu)", (long)got, (unsigned long)sizeof(st));
    }
    arda_resp_end(r);

    CloseLibrary(MiamiBase);
    MiamiBase = NULL;
}

/* ---- NET STATUS ------------------------------------------------------- */

static void cmd_net_status(struct Arda *a)
{
    static struct IfEntry ifs[MAX_IFACES];
    struct ArdaResp *r = &a->resp;
    STRPTR release = NULL;
    char buf[128];
    struct List *dns;
    struct Library *miami;
    int s, n = -1;

    arda_resp_begin(r);
    arda_resp_line(r, "BSDSOCKET_VERSION=%u.%u", SocketBase->lib_Version, SocketBase->lib_Revision);
    SocketBaseTags(SBTM_GETREF(SBTC_RELEASESTRPTR), (IPTR)&release, TAG_DONE);
    arda_safe_str(buf, sizeof(buf), release);
    arda_resp_line(r, "STACK_RELEASE=%s", buf);

    buf[0] = '\0';
    if (gethostname(buf, sizeof(buf)) == 0)
    {
        char h[128];
        arda_safe_str(h, sizeof(h), buf);
        arda_resp_line(r, "HOSTNAME=%s", h);
    }
    buf[0] = '\0';
    if (GetDefaultDomainName(buf, sizeof(buf)) && buf[0])
    {
        char d[128];
        arda_safe_str(d, sizeof(d), buf);
        arda_resp_line(r, "DOMAIN=%s", d);
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s >= 0)
    {
        n = collect_ifaces(s, ifs, MAX_IFACES);
        CloseSocket(s);
    }
    arda_resp_line(r, "INTERFACES=%d", n);
    arda_resp_line(r, "ROUTES_INET=%lu", (unsigned long)route_lines(NULL, AF_INET, "inet"));
    arda_resp_line(r, "ROUTES_INET6=%lu", (unsigned long)route_lines(NULL, AF_INET6, "inet6"));

    dns = ObtainDomainNameServerList();
    if (dns)
    {
        struct DomainNameServerNode *dn;
        int count = 0;
        for (dn = (struct DomainNameServerNode *)dns->lh_Head;
             dn->dnsn_MinNode.mln_Succ; dn = (struct DomainNameServerNode *)dn->dnsn_MinNode.mln_Succ)
        {
            char addr[64];
            arda_safe_str(addr, sizeof(addr), dn->dnsn_Address);
            arda_resp_line(r, "DNS=%s USECOUNT=%ld", addr, (long)dn->dnsn_UseCount);
            count++;
        }
        arda_resp_line(r, "DNS_COUNT=%d", count);
        ReleaseDomainNameServerList(dns);
    }
    else
        arda_resp_line(r, "DNS_COUNT=unavailable");

    miami = OpenLibrary("miami.library", 0);
    arda_resp_line(r, "STATS=%s", miami ? "available" : "unavailable");
    if (miami)
        CloseLibrary(miami);
    arda_resp_line(r, "SOCKETS=unsupported");
    arda_resp_end(r);
}

/* ---- dispatch --------------------------------------------------------- */

void arda_cmd_net(struct Arda *a, char *args)
{
    char *sub = args, *rest;

    while (*sub == ' ' || *sub == '\t')
        sub++;
    for (rest = sub; *rest && *rest != ' ' && *rest != '\t'; rest++)
        ;
    if (*rest)
    {
        *rest++ = '\0';
        while (*rest == ' ' || *rest == '\t')
            rest++;
    }

    if (!strcasecmp(sub, "STATUS"))
        cmd_net_status(a);
    else if (!strcasecmp(sub, "INTERFACES"))
        cmd_net_interfaces(a);
    else if (!strcasecmp(sub, "ROUTES"))
        cmd_net_routes(a);
    else if (!strcasecmp(sub, "ARP"))
        cmd_net_arp(a);
    else if (!strcasecmp(sub, "STATS"))
        cmd_net_stats(a, rest);
    else if (!strcasecmp(sub, "SOCKETS"))
        arda_resp_err(&a->resp, 501, "socket enumeration has no supported API in AROSTCP (netstat reads stack memory via libkvm)");
    else
        arda_resp_err(&a->resp, 400, "usage: NET STATUS | NET INTERFACES | NET ROUTES | NET ARP | NET STATS [IP|TCP|UDP|ALL]");
}
