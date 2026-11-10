/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for TCP_INFO (Linux's option, FreeBSD's struct tcp_info):
    what getsockopt() gives of a connection - its state, the options it
    negotiated, its maximum segment size, windows and sequence numbers,
    and its round-trip estimate and retransmission timeout, in
    microseconds. A short buffer gets the start of the structure; a
    listening socket reports LISTEN. Over the loopback. Usage:
    cunit-net-tcpinfo [IFNAME]  (default eth0; the SYN backoff test needs
    an interface, the others the loopback)
*/

#include "socktest.h"

#include <net/if.h>

#include <proto/dos.h>
#include <proto/miami.h>
#include <sys/sysctl.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <net/route.h>

struct Library *SocketBase = NULL;
static const char *ifname = "eth0";
struct Library *MiamiBase = NULL;

/* Read net.inet.tcp.<leaf> into *val, setting it to *nval first if nval. */
static int tcpctl(int leaf, LONG *val, LONG *nval)
{
    LONG name[4] = { CTL_NET, PF_INET, IPPROTO_TCP, 0 };
    LONG len = sizeof(*val);

    name[3] = leaf;
    return MiamiSysCtl(name, 4, val, &len, nval, nval ? sizeof(*nval) : 0, 0);
}

static int info(int s, struct tcp_info *ti, socklen_t *len)
{
    socklen_t l = sizeof(*ti);

    memset(ti, 0xa5, sizeof(*ti));
    if (len == NULL)
        len = &l;
    return getsockopt(s, IPPROTO_TCP, TCP_INFO, ti, len);
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* An established connection: its state, segment size, windows, and a
 * retransmission timeout between the RFC 6298 bounds the stack keeps. */
static void established(int family)
{
    struct tcp_info ti;
    socklen_t len = sizeof(ti);
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(family, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, &len), 0);
    CU_ASSERT_EQUAL(len, sizeof(ti));
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_ESTABLISHED);
    CU_ASSERT(ti.tcpi_snd_mss > 0 && ti.tcpi_snd_mss < 65536);
    CU_ASSERT(ti.tcpi_snd_cwnd >= ti.tcpi_snd_mss);
    CU_ASSERT(ti.tcpi_rcv_space > 0);
    CU_ASSERT(ti.tcpi_snd_wnd > 0);
    CU_ASSERT_EQUAL(ti.tcpi_snd_una, ti.tcpi_snd_nxt);
    CU_ASSERT(ti.tcpi_rto >= 200000 && ti.tcpi_rto <= 64000000);
    printf("\n    rto %lu us, rtt %lu us, rttvar %lu us, mss %lu, options 0x%x\n",
           (unsigned long)ti.tcpi_rto, (unsigned long)ti.tcpi_rtt, (unsigned long)ti.tcpi_rttvar,
           (unsigned long)ti.tcpi_snd_mss, ti.tcpi_options);
    CloseSocket(c);
    CloseSocket(s);
}

void testTCPINFO_ESTABLISHED4(void) { established(AF_INET); }
void testTCPINFO_ESTABLISHED6(void) { established(AF_INET6); }

/* A new connection starts with RFC 6928's initial window, min(10 * MSS,
 * max(2 * MSS, 14600)), even to a destination on a local network (here
 * the loopback): slow start for every connection (RFC 5681 section 3.1). */
static void initial_window(int family)
{
    struct tcp_info ti;
    unsigned long iw;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(family, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
    iw = ti.tcpi_snd_mss * 2 > 14600 ? ti.tcpi_snd_mss * 2 : 14600;
    if (iw > ti.tcpi_snd_mss * 10)
        iw = ti.tcpi_snd_mss * 10;
    printf("\n    snd_cwnd %lu, mss %lu, initial window %lu\n",
           (unsigned long)ti.tcpi_snd_cwnd, (unsigned long)ti.tcpi_snd_mss, iw);
    CU_ASSERT_EQUAL(ti.tcpi_snd_cwnd, iw);
    CloseSocket(c);
    CloseSocket(s);
}

void testTCPINFO_INITIAL_WINDOW4(void) { initial_window(AF_INET); }
void testTCPINFO_INITIAL_WINDOW6(void) { initial_window(AF_INET6); }

/* How much slow start grows cwnd while 12 segments are sent and
 * acknowledged, with net.inet.tcp.abc_l at abc; the bytes acknowledged in
 * *acked. */
static long ss_growth(LONG abc, long *acked)
{
    struct tcp_info a, b;
    static char buf[12 * 1448];
    LONG old = 0, ohs = 1, off = 0;
    int c, s, got, k;

    *acked = -1;
    /* HyStart++ applies its own limit in the first slow start */
    if (tcpctl(TCPCTL_HYSTART, &ohs, &off) != 0 ||
            tcpctl(TCPCTL_ABC_L, &old, &abc) != 0 || st_tcp_pair(AF_INET, &c, &s) != 0)
        return -1;
    info(c, &a, NULL);
    send(c, buf, 12 * a.tcpi_snd_mss, 0);
    for (got = 0; got < (int)(12 * a.tcpi_snd_mss); got += k)
        if ((k = recv(s, buf, sizeof(buf), 0)) <= 0)
            break;
    Delay(25);                          /* the last ACKs, delayed */
    info(c, &b, NULL);
    *acked = b.tcpi_snd_una - a.tcpi_snd_una;
    CloseSocket(c);
    CloseSocket(s);
    tcpctl(TCPCTL_ABC_L, &abc, &old);
    tcpctl(TCPCTL_HYSTART, &off, &ohs);
    return (long)b.tcpi_snd_cwnd - (long)a.tcpi_snd_cwnd;
}

/* Slow start grows cwnd by the bytes acknowledged, at most abc_l segments
 * an ACK (RFC 3465): never by more than was acknowledged, and with 2 by up
 * to twice what 1 (the default, RFC 5681's min(N, SMSS)) gives. Over the
 * loopback each ACK covers several segments, so the limit is what counts. */
void testTCPINFO_SLOW_START_BYTES(void)
{
    long a1, a2, g1, g2;
    LONG v = -1;

    if (MiamiBase == NULL) {
        CU_PASS("miami.library not available - skipping");
        return;
    }
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 1);
    g2 = ss_growth(2, &a2);
    g1 = ss_growth(1, &a1);
    printf("\n    acked %ld: abc_l 2 grows cwnd %ld, abc_l 1 %ld\n", a2, g2, g1);
    CU_ASSERT(a2 > 0 && a1 == a2);
    CU_ASSERT(g1 > 0 && g1 < g2);
    CU_ASSERT(g2 <= 2 * g1 && g2 <= a2);
}

/* HyStart++ ends slow start only when the RTT grows: over the loopback,
 * where it does not, 200 KB leave ssthresh at its initial value. */
void testTCPINFO_HYSTART_NO_EXIT(void)
{
    struct tcp_info a, b;
    static char buf[16384];
    int c, s, sent, got;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &a, NULL), 0);
    for (sent = got = 0; got < 200 * 1024; ) {
        int k;

        if (sent < 200 * 1024 && (k = send(c, buf, sizeof(buf), 0)) > 0)
            sent += k;
        if ((k = recv(s, buf, sizeof(buf), 0)) <= 0)
            break;
        got += k;
    }
    Delay(10);
    CU_ASSERT_EQUAL(info(c, &b, NULL), 0);
    printf("\n    ssthresh %lu -> %lu, cwnd %lu -> %lu\n", (unsigned long)a.tcpi_snd_ssthresh,
           (unsigned long)b.tcpi_snd_ssthresh, (unsigned long)a.tcpi_snd_cwnd, (unsigned long)b.tcpi_snd_cwnd);
    CU_ASSERT(got >= 200 * 1024);
    CU_ASSERT_EQUAL(b.tcpi_snd_ssthresh, a.tcpi_snd_ssthresh);
    CU_ASSERT(b.tcpi_snd_cwnd > a.tcpi_snd_cwnd);
    CloseSocket(c);
    CloseSocket(s);
}

#define RT_ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/* Set the ssthresh metric of the loopback host route (127.0.0.1 or ::1):
 * RTM_CHANGE with RTV_SSTHRESH; 0 or errno. */
/* RTM_CHANGE of a metric of the loopback route of family: RTV_SSTHRESH or
 * RTV_MTU, to value (0 clears it). 0 or the errno. */
static int loopback_metric(int family, u_long which, u_long value)
{
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_storage ss;
    socklen_t len = st_loopback(family, &ss, 0);
    int s = socket(PF_ROUTE, SOCK_RAW, 0), r = 0;

    if (s < 0)
        return Errno();
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_CHANGE;
    h->rtm_flags = RTF_HOST;
    h->rtm_addrs = RTA_DST;
    h->rtm_seq = 1;
    h->rtm_inits = which;
    if (which == RTV_MTU)
        h->rtm_rmx.rmx_mtu = value;
    else
        h->rtm_rmx.rmx_ssthresh = value;
    memcpy(h + 1, &ss, len);
    ((struct sockaddr *)(h + 1))->sa_len = len;
    h->rtm_msglen = sizeof(*h) + RT_ROUNDUP(len);
    if (send(s, (char *)buf, h->rtm_msglen, 0) < 0)
        r = Errno();
    CloseSocket(s);
    return r;
}

static int loopback_ssthresh(int family, u_long bytes)
{
    return loopback_metric(family, RTV_SSTHRESH, bytes);
}

/* A route's ssthresh (RTV_SSTHRESH) is where a new connection over it
 * starts slow start from (tcp_mss()). */
static void route_ssthresh(int family)
{
    struct tcp_info ti;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(loopback_ssthresh(family, 50000), 0);
    CU_ASSERT_EQUAL(st_tcp_pair(family, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
    printf("\n    route ssthresh 50000: connection's ssthresh %lu\n", (unsigned long)ti.tcpi_snd_ssthresh);
    CU_ASSERT_EQUAL(ti.tcpi_snd_ssthresh, 50000);
    CloseSocket(c);
    CloseSocket(s);
    loopback_ssthresh(family, 0);
}

void testTCPINFO_ROUTE_SSTHRESH4(void) { route_ssthresh(AF_INET); }

/* A connection never uses an MSS below net.inet.tcp.minmss (TCP_MINMSS,
 * 216): over a route with an MTU of 128 - 88 bytes of segment, as a peer
 * offering a tiny MSS would ask - both ends use 216 (less the timestamp
 * option); they used 88 less it. */
void testTCPINFO_MSS_FLOOR(void)
{
    struct tcp_info ci, si;
    LONG floor = 0;
    int c, s;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MINMSS, &floor, NULL), 0);
    CU_ASSERT_EQUAL(floor, TCP_MINMSS);
    floor = TCP_MINMSS;
    CU_ASSERT_EQUAL_FATAL(loopback_metric(AF_INET, RTV_MTU, 128), 0);
    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ci, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &si, NULL), 0);
    printf("\n    route MTU 128, minmss %ld: MSS %lu and %lu\n", (long)floor,
           (unsigned long)ci.tcpi_snd_mss, (unsigned long)si.tcpi_snd_mss);
    CU_ASSERT(ci.tcpi_snd_mss + 12 >= (u_long)floor);
    CU_ASSERT(si.tcpi_snd_mss + 12 >= (u_long)floor);
    CloseSocket(c);
    CloseSocket(s);
    loopback_metric(AF_INET, RTV_MTU, 0);
}
void testTCPINFO_ROUTE_SSTHRESH6(void) { route_ssthresh(AF_INET6); }

/* The pure ACKs the stack has sent (net.inet.tcp.stats), or -1. */
static long tcpstat_field(size_t off)
{
    LONG name[4] = { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS };
    struct tcpstat st;
    LONG len = sizeof(st);

    if (MiamiSysCtl(name, 4, &st, &len, NULL, 0, 0) != 0)
        return -1;
    return (long)*(u_long *)((char *)&st + off);
}

static long sent_acks(void)
{
    LONG name[4] = { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS };
    struct tcpstat st;
    LONG len = sizeof(st);

    if (MiamiSysCtl(name, 4, &st, &len, NULL, 0, 0) != 0)
        return -1;
    return (long)st.tcps_sndacks;
}

/* A receiver acknowledges at least every second full-sized segment (RFC
 * 1122 section 4.2.3.2, RFC 5681 section 4.2): 20 segments over the
 * loopback, read as they come, get at least 9 pure ACKs. */
void testTCPINFO_ACK_EVERY_SECOND(void)
{
    struct tcp_info ti;
    static char buf[20 * 1448];
    long a0, a1;
    int c, s, got, mss;

    if (MiamiBase == NULL) {
        CU_PASS("miami.library not available - skipping");
        return;
    }
    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
    mss = ti.tcpi_snd_mss;
    a0 = sent_acks();
    CU_ASSERT_EQUAL(send(c, buf, 20 * mss, 0), 20 * mss);
    for (got = 0; got < 20 * mss; ) {
        int k = recv(s, buf, sizeof(buf), 0);

        if (k <= 0)
            break;
        got += k;
    }
    Delay(10);
    a1 = sent_acks();
    printf("\n    20 segments: %ld pure ACKs\n", a1 - a0);
    CU_ASSERT(a0 >= 0 && got == 20 * mss);
    CU_ASSERT(a1 - a0 >= 9);
    CloseSocket(c);
    CloseSocket(s);
}

/* cwnd grows only while the connection uses it (RFC 7661; Linux's
 * tcp_is_cwnd_limited(), FreeBSD's CCF_CWND_LIMITED): an application that
 * sends 1 KB every 40 ms never has half the initial window in flight, so
 * its 40 ACKs leave cwnd where it was. */
void testTCPINFO_CWND_APP_LIMITED(void)
{
    struct tcp_info a, b;
    char buf[1000];
    int c, s, i;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &a, NULL), 0);
    for (i = 0; i < 40; i++) {
        CU_ASSERT_EQUAL(send(c, buf, sizeof(buf), 0), sizeof(buf));
        CU_ASSERT_EQUAL(recv(s, buf, sizeof(buf), MSG_WAITALL), sizeof(buf));
        Delay(2);
    }
    Delay(10);
    CU_ASSERT_EQUAL(info(c, &b, NULL), 0);
    printf("\n    40 KB sent 1 KB at a time: cwnd %lu -> %lu\n",
           (unsigned long)a.tcpi_snd_cwnd, (unsigned long)b.tcpi_snd_cwnd);
    CU_ASSERT_EQUAL(b.tcpi_snd_cwnd, a.tcpi_snd_cwnd);
    CloseSocket(c);
    CloseSocket(s);
}

/* Data sent and read moves the sequence numbers by its length on both
 * sides, and all of it is acknowledged. */
void testTCPINFO_SEQUENCE(void)
{
    struct tcp_info a, b, a2, b2;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &a, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &b, NULL), 0);
    CU_ASSERT_EQUAL(st_oneway(c, s, 10000), 0);
    Delay(25);                  /* the last ACK, delayed */
    CU_ASSERT_EQUAL(info(c, &a2, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &b2, NULL), 0);
    CU_ASSERT_EQUAL(a2.tcpi_snd_nxt - a.tcpi_snd_nxt, 10000);
    CU_ASSERT_EQUAL(a2.tcpi_snd_una, a2.tcpi_snd_max);
    CU_ASSERT_EQUAL(b2.tcpi_rcv_nxt - b.tcpi_rcv_nxt, 10000);
    CU_ASSERT_EQUAL(b2.tcpi_rcv_nxt, a2.tcpi_snd_nxt);
    CloseSocket(c);
    CloseSocket(s);
}

/* Over the loopback, after some exchanges, the RTT is measured in
 * milliseconds (well under 100 ms) and the RTO is the floor,
 * net.inet.tcp.rexmit_min (200 ms by default), not whole seconds. */
void testTCPINFO_RTO_FLOOR(void)
{
    struct tcp_info ti;
    char b[100];
    int c, s, i;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET6, &c, &s), 0);
    for (i = 0; i < 20; i++) {
        CU_ASSERT_EQUAL(send(c, b, sizeof(b), 0), sizeof(b));
        CU_ASSERT_EQUAL(recv(s, b, sizeof(b), MSG_WAITALL), sizeof(b));
        CU_ASSERT_EQUAL(send(s, b, sizeof(b), 0), sizeof(b));
        CU_ASSERT_EQUAL(recv(c, b, sizeof(b), MSG_WAITALL), sizeof(b));
    }
    CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
    printf("\n    after 20 exchanges: rto %lu us, rtt %lu us, rttvar %lu us\n",
           (unsigned long)ti.tcpi_rto, (unsigned long)ti.tcpi_rtt, (unsigned long)ti.tcpi_rttvar);
    CU_ASSERT(ti.tcpi_rtt < 100000);
    CU_ASSERT_EQUAL(ti.tcpi_rto, 200000);
    CloseSocket(c);
    CloseSocket(s);
}

/* A SYN nobody answers is resent after 1 s, then 2 s, then 4 s (RFC 6298
 * sections 2.1 and 5.5): the RTO doubles from the initial one, it does not
 * fall to the floor once the first SYN timed out. The SYN goes to a
 * link-local neighbour that does not exist, on IFNAME (cunit-net-tcpinfo
 * [IFNAME], default eth0); without the interface the test is skipped. */
static unsigned index_of(const char *name)
{
    struct ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
    r = IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr);
    CloseSocket(s);
    return r == 0 ? ifr.ifr_index : 0;
}

void testTCPINFO_SYN_BACKOFF(void)
{
    struct tcp_info ti;
    struct sockaddr_in6 a;
    int s, on = 1;
    unsigned idx = index_of(ifname);
    unsigned long rto1, rto2;

    if (idx == 0) {
        CU_PASS("no interface - skipping");
        return;
    }
    s = socket(AF_INET6, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    IoctlSocket(s, FIONBIO, (char *)&on);
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_port = htons(9);
    inet_pton(AF_INET6, "fe80::4d54:dead", &a.sin6_addr);
    a.sin6_scope_id = idx;
    connect(s, (struct sockaddr *)&a, sizeof(a));
    Delay(75);                          /* 1.5 s: one timeout */
    info(s, &ti, NULL);
    rto1 = ti.tcpi_rto;
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_SYN_SENT);
    Delay(125);                         /* 4 s: two */
    info(s, &ti, NULL);
    rto2 = ti.tcpi_rto;
    printf("\n    SYN unanswered: RTO %lu us after one timeout, %lu us after two\n", rto1, rto2);
    CU_ASSERT_EQUAL(rto1, 2000000);
    CU_ASSERT_EQUAL(rto2, 4000000);
    CloseSocket(s);
}

/* A short buffer gets the start of the structure, and its length. */
void testTCPINFO_SHORT(void)
{
    struct tcp_info ti;
    socklen_t len = 8;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, &len), 0);
    CU_ASSERT_EQUAL(len, 8);
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_ESTABLISHED);
    CU_ASSERT_EQUAL(((unsigned char *)&ti)[8], 0xa5);    /* nothing past it */
    CloseSocket(c);
    CloseSocket(s);
}

/* One segment of the client's connection, made by hand and sent through a
 * raw socket over the loopback: SEQ, ACK, LEN bytes of data, no options;
 * the TCP checksum over the IPv4 pseudo-header (127.0.0.1 both ways, the
 * source the raw socket's output takes there). */
static int inject(int r, u_short sport, u_short dport, u_long seq, u_long ack, int len)
{
    u_char pkt[20 + 100], ph[12];
    struct sockaddr_in to;
    u_long sum = 0;
    int i;

    memset(pkt, 0, sizeof(pkt));
    pkt[0] = sport >> 8; pkt[1] = sport;
    pkt[2] = dport >> 8; pkt[3] = dport;
    pkt[4] = seq >> 24; pkt[5] = seq >> 16; pkt[6] = seq >> 8; pkt[7] = seq;
    pkt[8] = ack >> 24; pkt[9] = ack >> 16; pkt[10] = ack >> 8; pkt[11] = ack;
    pkt[12] = 5 << 4;                   /* data offset: no options */
    pkt[13] = 0x10;                     /* ACK */
    pkt[14] = 0xff; pkt[15] = 0xff;     /* window */
    for (i = 0; i < len; i++)
        pkt[20 + i] = 'r';
    ph[0] = 127; ph[1] = 0; ph[2] = 0; ph[3] = 1;
    memcpy(ph + 4, ph, 4);
    ph[8] = 0; ph[9] = IPPROTO_TCP; ph[10] = (20 + len) >> 8; ph[11] = (20 + len) & 0xff;
    for (i = 0; i < 12; i += 2)
        sum += (ph[i] << 8) | ph[i + 1];
    for (i = 0; i < 20 + len; i += 2)
        sum += (pkt[i] << 8) | (i + 1 < 20 + len ? pkt[i + 1] : 0);
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    sum = ~sum & 0xffff;
    pkt[16] = sum >> 8; pkt[17] = sum;
    memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return sendto(r, pkt, 20 + len, 0, (struct sockaddr *)&to, sizeof(to));
}

/* The segment that fills the hole in front of a full reassembly queue is
 * taken: 110 segments of 100 bytes arrive out of order, made by hand with
 * the client's numbers (more than the 100 the queue holds), then the
 * client sends those 11100 bytes for real, the hole first; the receiver
 * reads them all. It used to refuse every segment once the queue was full,
 * the one at rcv_nxt too, and the connection never moved again. The client
 * sends the data itself, all in its initial window, so that nothing is
 * acknowledged before the client has sent it (else each side answers the
 * other's ACKs for ever). The connection is opened without RFC 7323's
 * options, which the hand-made segments do not carry. */
void testTCPINFO_REASS_FULL(void)
{
    struct tcp_info ci, si;
    struct sockaddr_in a;
    socklen_t l = sizeof(a);
    LONG old = 1, off = 0;
    u_short cport, sport;
    static char buf[111 * 100];
    int c, s, r, i, got = 0, on = 1, sent = 0;

    CU_ASSERT_EQUAL_FATAL(tcpctl(TCPCTL_DO_RFC1323, &old, &off), 0);
    i = st_tcp_pair(AF_INET, &c, &s);
    tcpctl(TCPCTL_DO_RFC1323, &off, &old);
    CU_ASSERT_EQUAL_FATAL(i, 0);
    getsockname(c, (struct sockaddr *)&a, &l);
    cport = ntohs(a.sin_port);
    l = sizeof(a);
    getsockname(s, (struct sockaddr *)&a, &l);
    sport = ntohs(a.sin_port);
    CU_ASSERT_EQUAL(info(c, &ci, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &si, NULL), 0);
    CU_ASSERT_EQUAL(si.tcpi_rcv_nxt, ci.tcpi_snd_nxt);
    r = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    CU_ASSERT_FATAL(r >= 0);
    for (i = 1; i <= 110; i++)
        sent += inject(r, cport, sport, si.tcpi_rcv_nxt + i * 100, ci.tcpi_rcv_nxt, 100) == 120;
    Delay(5);
    memset(buf, 'r', sizeof(buf));
    CU_ASSERT_EQUAL(send(c, buf, sizeof(buf), 0), sizeof(buf));
    IoctlSocket(s, FIONBIO, (char *)&on);
    for (i = 0; i < 100 && got < (int)sizeof(buf); i++) {     /* up to 2 s */
        int n;

        while ((n = recv(s, buf, sizeof(buf), 0)) > 0)
            got += n;
        Delay(1);
    }
    printf("\n    110 segments out of order (%d sent), then %d bytes from the client: %d read\n",
           sent, (int)sizeof(buf), got);
    CU_ASSERT_EQUAL(sent, 110);
    CU_ASSERT_EQUAL(got, sizeof(buf));
    CloseSocket(r);
    CloseSocket(c);
    CloseSocket(s);
}

/* Without window scaling (a peer that does not take the option: here RFC
 * 7323 is turned off for the connection) the side that connected offers the
 * 65535 bytes the window field can hold, and goes on doing so: with a
 * receive buffer above 64 KB it had promised the whole buffer on the
 * SYN-ACK, and the field then carried the low 16 bits of that - a few
 * hundred bytes, then zero - so that the peer's data waited for its persist
 * timer (#2173). */
void testTCPINFO_WINDOW_NO_SCALE(void)
{
    struct tcp_info si;
    LONG old = 1, off = 0;
    static char buf[16384];
    int c, s, i, on = 1, rcvbuf = 0, sent = 0, got = 0;
    socklen_t l = sizeof(rcvbuf);
    const int total = 256 * 1024;

    CU_ASSERT_EQUAL_FATAL(tcpctl(TCPCTL_DO_RFC1323, &old, &off), 0);
    i = st_tcp_pair(AF_INET, &c, &s);
    tcpctl(TCPCTL_DO_RFC1323, &off, &old);
    CU_ASSERT_EQUAL_FATAL(i, 0);
    getsockopt(c, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &l);
    /* a byte each way, so that the server has the window of an ACK the
     * client sent after its SYN */
    CU_ASSERT_EQUAL(send(c, "x", 1, 0), 1);
    CU_ASSERT_EQUAL(sock_wait(s, 0, 2), 1);
    CU_ASSERT_EQUAL(recv(s, buf, 1, 0), 1);
    CU_ASSERT_EQUAL(info(s, &si, NULL), 0);
    printf("\n    receive buffer %d, no window scaling: the client offers %lu bytes\n",
           rcvbuf, (unsigned long)si.tcpi_snd_wnd);
    CU_ASSERT_FALSE(si.tcpi_options & TCPI_OPT_WSCALE);
    CU_ASSERT_EQUAL(si.tcpi_snd_wnd, rcvbuf > 65535 ? 65535 : rcvbuf);

    /* and data to the client arrives without a pause */
    memset(buf, 'w', sizeof(buf));
    IoctlSocket(s, FIONBIO, (char *)&on);
    IoctlSocket(c, FIONBIO, (char *)&on);
    for (i = 0; i < 100 && got < total; i++) {      /* up to 2 s */
        int n;

        while (sent < total &&
                (n = send(s, buf, total - sent < (int)sizeof(buf) ? total - sent : (int)sizeof(buf), 0)) > 0)
            sent += n;
        while ((n = recv(c, buf, sizeof(buf), 0)) > 0)
            got += n;
        if (got < total)
            Delay(1);
    }
    printf("    %d bytes sent to it, %d read\n", sent, got);
    CU_ASSERT_EQUAL(got, total);
    CloseSocket(c);
    CloseSocket(s);
}

/* Two ends out of step do not answer each other's ACKs for ever: 100
 * bytes made by hand with the client's numbers reach the server, which
 * takes them and acknowledges data the client never sent; the client's
 * ACK of its own state is below the server's window, and so on. Each side
 * answering at once kept the stack busy for good (an ACK war); the ACKs
 * for dropped segments are now at most 5 a second on a connection, and in
 * a second the stack sends a few dozen ACKs at most. */
void testTCPINFO_ACK_WAR(void)
{
    struct tcp_info ci, si;
    struct sockaddr_in a;
    socklen_t l = sizeof(a);
    LONG old = 1, off = 0;
    u_short cport, sport;
    long a0, a1;
    int c, s, r, i;

    CU_ASSERT_EQUAL_FATAL(tcpctl(TCPCTL_DO_RFC1323, &old, &off), 0);
    i = st_tcp_pair(AF_INET, &c, &s);
    tcpctl(TCPCTL_DO_RFC1323, &off, &old);
    CU_ASSERT_EQUAL_FATAL(i, 0);
    getsockname(c, (struct sockaddr *)&a, &l);
    cport = ntohs(a.sin_port);
    l = sizeof(a);
    getsockname(s, (struct sockaddr *)&a, &l);
    sport = ntohs(a.sin_port);
    CU_ASSERT_EQUAL(info(c, &ci, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &si, NULL), 0);
    r = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    CU_ASSERT_FATAL(r >= 0);
    a0 = sent_acks();
    CU_ASSERT_EQUAL(inject(r, cport, sport, si.tcpi_rcv_nxt, ci.tcpi_rcv_nxt, 100), 120);
    Delay(50);
    a1 = sent_acks();
    printf("\n    out of step for 1 s: %ld ACKs sent\n", a1 - a0);
    CU_ASSERT(a1 - a0 > 0 && a1 - a0 < 40);
    CloseSocket(r);
    CloseSocket(c);
    CloseSocket(s);
}

/* One write leaves in full-sized segments: the socket layer hands TCP a
 * cluster at a time, and with TCP_NODELAY on an idle connection each one
 * used to end in a segment of its own, part of a segment (as FreeBSD's
 * PRUS_MORETOCOME, TCP now waits for the rest of the write). 10000 bytes
 * on a connection with nothing in flight: ceil(10000 / MSS) segments. */
void testTCPINFO_WRITE_SEGMENTS(void)
{
    static char buf[10000];
    struct tcp_info ti;
    long p0, p1, want;
    int c, s, on = 1, n, got = 0;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    setsockopt(c, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
    want = (sizeof(buf) + ti.tcpi_snd_mss - 1) / ti.tcpi_snd_mss;
    p0 = tcpstat_field(offsetof(struct tcpstat, tcps_sndpack));
    CU_ASSERT_EQUAL(send(c, buf, sizeof(buf), 0), sizeof(buf));
    while (got < (int)sizeof(buf) && (n = recv(s, buf, sizeof(buf), 0)) > 0)
        got += n;
    p1 = tcpstat_field(offsetof(struct tcpstat, tcps_sndpack));
    printf("\n    10000 bytes, MSS %lu: %ld data segments (least %ld)\n",
           (unsigned long)ti.tcpi_snd_mss, p1 - p0, want);
    CU_ASSERT_EQUAL(p1 - p0, want);
    CloseSocket(c);
    CloseSocket(s);
}

/* The accepted side of a connection has an RTT from the handshake (the
 * timestamp its SYN-ACK carried, echoed in the final ACK), before any
 * data; it used to have none until it sent some. */
void testTCPINFO_PASSIVE_RTT(void)
{
    struct tcp_info ti;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(s, &ti, NULL), 0);
    printf("\n    accepted side: rtt %lu us, rto %lu us\n",
           (unsigned long)ti.tcpi_rtt, (unsigned long)ti.tcpi_rto);
    CU_ASSERT(ti.tcpi_rtt > 0);
    CloseSocket(c);
    CloseSocket(s);
}

static int bufsize(int s, int opt)
{
    int v = 0;
    socklen_t l = sizeof(v);

    getsockopt(s, SOL_SOCKET, opt, &v, &l);
    return v;
}

/* Socket buffer autosizing: the ceilings follow the memory free when the
 * stack started (off below 16 MB, 1/64 of it, at most 1 MB and what the
 * budget gives one buffer) and refuse more than that budget; a default send buffer grows when it is
 * what holds a bulk sender back, and one the application sized stays. */
void testTCPINFO_AUTOSIZE(void)
{
    LONG snd = 0, rcv = 0, big = 64 * 1024 * 1024;
    int c, s, c2, s2, i, v = 96 * 1024, n0, n1, r0, r1;
    unsigned long avail = (unsigned long)AvailMem(MEMF_ANY);

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_SENDBUF_MAX, &snd, NULL), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_RECVBUF_MAX, &rcv, NULL), 0);
    printf("\n    %lu KB free: ceilings %ld, %ld\n", avail / 1024, (long)snd, (long)rcv);
    CU_ASSERT(snd <= 1024 * 1024 && rcv <= 1024 * 1024);
    CU_ASSERT(snd % (16 * 1024) == 0 && rcv % (16 * 1024) == 0);
    CU_ASSERT_NOT_EQUAL(tcpctl(TCPCTL_SENDBUF_MAX, &snd, &big), 0);
    if (snd <= 128 * 1024 || rcv <= 128 * 1024) {
        printf("    autosizing off on this machine: nothing to grow\n");
        return;
    }

    /* over the loopback the buffer holds the sender back only when the
     * sending task outruns the stack, which takes a few writes or none:
     * up to three transfers of 4 MB (one did not grow it in about one
     * run in twelve) */
    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    n0 = n1 = bufsize(c, SO_SNDBUF);
    r0 = bufsize(s, SO_RCVBUF);
    for (i = 0; i < 3 && n1 == n0; i++) {
        CU_ASSERT_EQUAL(st_oneway(c, s, 4 * 1024 * 1024), 0);
        n1 = bufsize(c, SO_SNDBUF);
    }
    r1 = bufsize(s, SO_RCVBUF);
    printf("    %d x 4 MB over the loopback: send buffer %d -> %d, receive buffer %d -> %d\n",
           i, n0, n1, r0, r1);
    CU_ASSERT(n1 > n0 && n1 <= snd);
    CU_ASSERT(r1 >= r0 && r1 <= rcv);
    CloseSocket(c);
    CloseSocket(s);

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c2, &s2), 0);
    CU_ASSERT_EQUAL(setsockopt(c2, SOL_SOCKET, SO_SNDBUF, &v, sizeof(v)), 0);
    n0 = bufsize(c2, SO_SNDBUF);
    CU_ASSERT_EQUAL(st_oneway(c2, s2, 4 * 1024 * 1024), 0);
    n1 = bufsize(c2, SO_SNDBUF);
    printf("    with SO_SNDBUF %d: %d -> %d\n", v, n0, n1);
    CU_ASSERT_EQUAL(n1, n0);
    CloseSocket(c2);
    CloseSocket(s2);
}

/* With the ceilings at 0 nothing grows, and a new connection asks for the
 * window scale its buffer alone needs, not the ceiling's. */
void testTCPINFO_AUTOSIZE_OFF(void)
{
    struct tcp_info ti;
    LONG snd = 0, rcv = 0, zero = 0;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(tcpctl(TCPCTL_SENDBUF_MAX, &snd, &zero), 0);
    CU_ASSERT_EQUAL_FATAL(tcpctl(TCPCTL_RECVBUF_MAX, &rcv, &zero), 0);
    if (st_tcp_pair(AF_INET, &c, &s) == 0) {
        int n0 = bufsize(c, SO_SNDBUF), r0 = bufsize(s, SO_RCVBUF), n1, r1, scale;

        CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
        CU_ASSERT_EQUAL(st_oneway(c, s, 4 * 1024 * 1024), 0);
        n1 = bufsize(c, SO_SNDBUF);
        r1 = bufsize(s, SO_RCVBUF);
        printf("\n    ceilings 0, 4 MB over the loopback: send buffer %d -> %d, receive buffer %d -> %d, window scale %u\n",
               n0, n1, r0, r1, (unsigned)ti.tcpi_rcv_wscale);
        CU_ASSERT_EQUAL(n1, n0);
        CU_ASSERT_EQUAL(r1, r0);
        for (scale = 0; scale < 14 && (65535L << scale) < bufsize(c, SO_RCVBUF); scale++)
            ;
        CU_ASSERT_EQUAL(ti.tcpi_rcv_wscale, scale);
        CloseSocket(c);
        CloseSocket(s);
    } else
        CU_FAIL("no connection");
    /* the ceilings the stack set take back */
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_SENDBUF_MAX, &zero, &snd), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_RECVBUF_MAX, &zero, &rcv), 0);
}

/* A listening socket reports LISTEN. */
void testTCPINFO_LISTEN(void)
{
    struct tcp_info ti;
    unsigned short port;
    int l = st_bound(AF_INET, SOCK_STREAM, &port);

    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(listen(l, 1), 0);
    CU_ASSERT_EQUAL(info(l, &ti, NULL), 0);
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_LISTEN);
    CloseSocket(l);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping TCPINFO tests\n");
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("TCPINFO_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an established connection (IPv4)", testTCPINFO_ESTABLISHED4) ||
        !CU_add_test(p, "an established connection (IPv6)", testTCPINFO_ESTABLISHED6) ||
        !CU_add_test(p, "the initial window, to a local destination (IPv4)", testTCPINFO_INITIAL_WINDOW4) ||
        !CU_add_test(p, "the initial window, to a local destination (IPv6)", testTCPINFO_INITIAL_WINDOW6) ||
        !CU_add_test(p, "slow start counts the bytes acknowledged", testTCPINFO_SLOW_START_BYTES) ||
        !CU_add_test(p, "HyStart++ does not end slow start without a delay", testTCPINFO_HYSTART_NO_EXIT) ||
        !CU_add_test(p, "a route's ssthresh reaches a new connection (IPv4)", testTCPINFO_ROUTE_SSTHRESH4) ||
        !CU_add_test(p, "a route's ssthresh reaches a new connection (IPv6)", testTCPINFO_ROUTE_SSTHRESH6) ||
        !CU_add_test(p, "the MSS never goes below net.inet.tcp.minmss", testTCPINFO_MSS_FLOOR) ||
        !CU_add_test(p, "a receiver ACKs at least every second segment", testTCPINFO_ACK_EVERY_SECOND) ||
        !CU_add_test(p, "an application-limited sender does not grow cwnd", testTCPINFO_CWND_APP_LIMITED) ||
        !CU_add_test(p, "data moves the sequence numbers", testTCPINFO_SEQUENCE) ||
        !CU_add_test(p, "over the loopback the RTO is the floor", testTCPINFO_RTO_FLOOR) ||
        !CU_add_test(p, "an unanswered SYN backs off from 1 s", testTCPINFO_SYN_BACKOFF) ||
        !CU_add_test(p, "a short buffer gets the start", testTCPINFO_SHORT) ||
        !CU_add_test(p, "a full reassembly queue takes the segment that fills its hole", testTCPINFO_REASS_FULL) ||
        !CU_add_test(p, "without window scaling the window offered is 65535, not its low bits", testTCPINFO_WINDOW_NO_SCALE) ||
        !CU_add_test(p, "the accepted side has an RTT from the handshake", testTCPINFO_PASSIVE_RTT) ||
        !CU_add_test(p, "socket buffers grow unless the application sized them", testTCPINFO_AUTOSIZE) ||
        !CU_add_test(p, "with the ceilings at 0 no buffer grows", testTCPINFO_AUTOSIZE_OFF) ||
        !CU_add_test(p, "two ends out of step do not answer each other for ever", testTCPINFO_ACK_WAR) ||
        !CU_add_test(p, "one write leaves in full-sized segments", testTCPINFO_WRITE_SEGMENTS) ||
        !CU_add_test(p, "a listening socket reports LISTEN", testTCPINFO_LISTEN))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-TCPINFO");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (MiamiBase)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
