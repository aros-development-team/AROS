/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: TCP SYN cache (RFC 4987).

    Half-open (SYN received, awaiting the final ACK) connections are kept in a
    compact hash table rather than as real sockets on the listening socket's
    incomplete accept queue.  This keeps a SYN flood from consuming socket and
    accept-queue resources: only when the three-way handshake completes is a
    full socket allocated, bounded by the application's accept backlog.

    When a hash bucket overflows (a sustained flood), the oldest entry is
    evicted and the cache switches to stateless SYN cookies for a short while,
    so legitimate handshakes can still complete without keeping any state.

    The design mirrors FreeBSD's tcp_syncache, adapted to AROSTCP's single
    network task / splnet() model.
*/

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/in_cksum_protos.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>

#include "tcp_compat.h"
#include <netinet/tcp_cc.h>
#include "tcp_syncache.h"

#include <kern/amiga_includes.h>
#include <kern/uipc_socket2_protos.h>

extern int ip_defttl;			/* default TTL for TCP segments */

/* Negotiated/observed options carried for a half-open connection. */
#define SCF_WINSCALE	0x01		/* peer offered window scaling */
#define SCF_TIMESTAMP	0x02		/* peer offered RFC 1323 timestamps */
#define SCF_SACK	0x04		/* peer offered SACK */
#define SCF_ECN		0x08		/* peer requested ECN */

struct syncache {
    LIST_ENTRY(syncache) sc_hash;	/* hash bucket chain */
    struct in_addr  sc_faddr;		/* peer (foreign) address (IPv4) */
    struct in_addr  sc_laddr;		/* local address (IPv4) */
    struct in6_addr sc_faddr6;		/* peer (foreign) address (IPv6) */
    struct in6_addr sc_laddr6;		/* local address (IPv6) */
    u_int16_t       sc_fport;		/* peer (foreign) port */
    u_int16_t       sc_lport;		/* local port */
    tcp_seq         sc_irs;		/* initial received seq (peer ISN) */
    tcp_seq         sc_iss;		/* initial send seq (our ISN) */
    u_int32_t       sc_ts_recent;	/* peer timestamp value to echo */
    u_int16_t       sc_peer_mss;	/* peer's advertised MSS */
    u_int16_t       sc_wnd;		/* our advertised window (16-bit field) */
    u_int8_t        sc_requested_s_scale; /* peer's window scale shift */
    u_int8_t        sc_request_r_scale;	/* our window scale shift */
    u_int8_t        sc_ip_ttl;		/* TTL/hop-limit for our responses */
    u_int8_t        sc_flags;		/* SCF_* */
    u_int8_t        sc_isipv6;		/* connection is IPv6 */
    u_int8_t        sc_rxmits;		/* SYN-ACK (re)transmit count */
    u_long          sc_rxttick;		/* tcp_now tick of next retransmit */
    struct socket  *sc_lso;		/* the listening socket (parent) */
};

/*
 * Family-neutral connection key extracted from an incoming segment.  For IPv4
 * the addresses come from the struct tcpiphdr overlay; for IPv6 they are the
 * separately carried src6/dst6 (the overlay then holds only the TCP header).
 * "f" = foreign/peer (segment source), "l" = local (segment destination).
 */
struct sc_key {
    int             isipv6;
    struct in_addr  faddr, laddr;	/* IPv4 */
    struct in6_addr faddr6, laddr6;	/* IPv6 */
    u_int16_t       fport, lport;
};

static void
syncache_key(struct sc_key *k, struct tcpiphdr *ti, int isipv6,
    struct in6_addr *src6, struct in6_addr *dst6)
{
    bzero((caddr_t)k, sizeof(*k));
    k->isipv6 = isipv6;
    k->fport  = ti->ti_sport;
    k->lport  = ti->ti_dport;
    if(isipv6) {
        k->faddr6 = *src6;
        k->laddr6 = *dst6;
    } else {
        k->faddr = ti->ti_src;
        k->laddr = ti->ti_dst;
    }
}

/* Fold a 128-bit IPv6 address to 32 bits for the (v4-shaped) cookie hash. */
static struct in_addr
syncache_v6fold(struct in6_addr *a6)
{
    struct in_addr r;
    u_int32_t *w = (u_int32_t *)a6;

    r.s_addr = w[0] ^ w[1] ^ w[2] ^ w[3];
    return (r);
}

LIST_HEAD(sc_head, syncache);

struct syncache_bucket {
    struct sc_head  sch_bucket;		/* chain of entries */
    int             sch_length;		/* number of entries in this bucket */
};

/* Tunables (compile-time). */
#define SYNCACHE_HASHSIZE	512	/* number of buckets (power of two) */
#define SYNCACHE_BUCKETLIMIT	30	/* max entries per bucket */
#define SYNCACHE_MAXREXMTS	3	/* SYN-ACK retransmits before giving up */
#define SYNCACHE_RXTBASE	6	/* base retransmit interval (slow ticks) */
#define SYNCOOKIE_PAUSE		30	/* cookie-only window after overflow */

static struct syncache_bucket	*tcp_syncache_tbl;
static u_int32_t		 tcp_syncache_hashmask;
static int			 tcp_syncache_count;
static u_long			 tcp_syncache_pause_until;

int tcp_syncookiesonly = 0;		/* force SYN cookies for every SYN */

/*
 * Hash a connection 4-tuple to a bucket index using the keyed ISN hash, so an
 * attacker cannot deliberately overload a single bucket.  IPv6 addresses are
 * folded to 32 bits first.
 */
static u_int32_t
syncache_hashidx(struct sc_key *k)
{
    u_int32_t h, fa, la;

    if(k->isipv6) {
        fa = syncache_v6fold(&k->faddr6).s_addr;
        la = syncache_v6fold(&k->laddr6).s_addr;
    } else {
        fa = k->faddr.s_addr;
        la = k->laddr.s_addr;
    }
    h = tcp_isn_hash(fa, la,
        ((u_int32_t)k->fport << 16) | k->lport, 0, tcp_secret_key);
    return (h & tcp_syncache_hashmask);
}

/*
 * Are we currently answering with cookies only (no cache state)?  This is the
 * case if the admin forced it, or if a recent bucket overflow put the cache
 * into the temporary attack-response pause.
 */
static int
syncache_cookiesonly(void)
{
    if (tcp_syncookiesonly)
        return (1);
    if (tcp_syncookies && tcp_now < tcp_syncache_pause_until)
        return (1);
    return (0);
}

void
syncache_init(void)
{
    u_int32_t i;

    tcp_syncache_tbl = bsd_malloc(
        (u_long)SYNCACHE_HASHSIZE * sizeof(struct syncache_bucket),
        M_PCB, M_WAITOK);
    tcp_syncache_hashmask = SYNCACHE_HASHSIZE - 1;
    for (i = 0; i < SYNCACHE_HASHSIZE; i++) {
        LIST_INIT(&tcp_syncache_tbl[i].sch_bucket);
        tcp_syncache_tbl[i].sch_length = 0;
    }
    tcp_syncache_count = 0;
    tcp_syncache_pause_until = 0;
}

static struct syncache *
syncache_lookup(struct syncache_bucket *sch, struct sc_key *k)
{
    struct syncache *sc;

    for (sc = sch->sch_bucket.lh_first; sc != NULL; sc = sc->sc_hash.le_next) {
        if (sc->sc_isipv6 != (k->isipv6 != 0))
            continue;
        if (sc->sc_fport != k->fport || sc->sc_lport != k->lport)
            continue;
        if (k->isipv6) {
            if (IN6_ARE_ADDR_EQUAL(&sc->sc_faddr6, &k->faddr6) &&
                    IN6_ARE_ADDR_EQUAL(&sc->sc_laddr6, &k->laddr6))
                return (sc);
        } else {
            if (sc->sc_faddr.s_addr == k->faddr.s_addr &&
                    sc->sc_laddr.s_addr == k->laddr.s_addr)
                return (sc);
        }
    }
    return (NULL);
}

static void
syncache_drop(struct syncache *sc, struct syncache_bucket *sch)
{
    LIST_REMOVE(sc, sc_hash);
    sch->sch_length--;
    tcp_syncache_count--;
    bsd_free((caddr_t)sc, M_PCB);
}

/*
 * Parse the interesting options from an incoming SYN: peer MSS, window scale,
 * timestamps, SACK-permitted and ECN.  Returns the option flags; *mssp,
 * *wscalep and *tsp are filled in as appropriate.
 */
static u_int8_t
syncache_parse(u_char *optp, int optlen, u_int16_t tiflags,
    u_int16_t *mssp, u_int8_t *wscalep, u_int32_t *tsp)
{
    u_int8_t flags = 0;
    u_char *cp = optp;
    int cnt = optlen;

    *mssp = 0;
    *wscalep = 0;
    *tsp = 0;

    while (cnt > 0) {
        if (*cp == TCPOPT_EOL)
            break;
        if (*cp == TCPOPT_NOP) {
            cp++;
            cnt--;
            continue;
        }
        if (cnt < 2 || cp[1] < 2 || cp[1] > cnt)
            break;
        switch (*cp) {
        case TCPOPT_MAXSEG:
            if (cp[1] == TCPOLEN_MAXSEG) {
                u_int16_t mss;
                bcopy(cp + 2, &mss, sizeof(mss));
                NTOHS(mss);
                *mssp = mss;
            }
            break;
        case TCPOPT_WINDOW:
            if (cp[1] == TCPOLEN_WINDOW) {
                flags |= SCF_WINSCALE;
                *wscalep = MIN(cp[2], TCP_MAX_WINSHIFT);
            }
            break;
        case TCPOPT_TIMESTAMP:
            if (cp[1] == TCPOLEN_TIMESTAMP) {
                u_int32_t ts;
                flags |= SCF_TIMESTAMP;
                bcopy(cp + 2, &ts, sizeof(ts));
                NTOHL(ts);
                *tsp = ts;
            }
            break;
        case TCPOPT_SACK_PERMITTED:
            if (cp[1] == TCPOLEN_SACK_PERMITTED)
                flags |= SCF_SACK;
            break;
        default:
            break;
        }
        cnt -= cp[1];
        cp += cp[1];
    }

    /* ECN is requested via ECE+CWR in the SYN. */
    if ((tiflags & (TH_ECE | TH_CWR)) == (TH_ECE | TH_CWR))
        flags |= SCF_ECN;

    return (flags);
}

/*
 * Build and transmit a SYN-ACK for a (cached or cookie) half-open connection.
 * Allocates its own mbuf; the incoming SYN mbuf is left untouched.
 */
static int
syncache_respond(struct syncache *sc)
{
    struct mbuf *m;
    struct tcpiphdr *ti;
    u_char *optp;
    u_char opt[TCP_MAXOLEN];
    int optlen = 0, tlen, win;

    /* MSS option (we advertise the Ethernet default; no per-SYN route work). */
    opt[0] = TCPOPT_MAXSEG;
    opt[1] = TCPOLEN_MAXSEG;
    {
        u_int16_t mss = htons((u_int16_t)tcp_mssdflt);
        bcopy(&mss, opt + 2, sizeof(mss));
    }
    optlen = TCPOLEN_MAXSEG;

    if (sc->sc_flags & SCF_WINSCALE) {
        *((u_int32_t *)(opt + optlen)) = htonl(
            TCPOPT_NOP << 24 | TCPOPT_WINDOW << 16 |
            TCPOLEN_WINDOW << 8 | sc->sc_request_r_scale);
        optlen += 4;
    }
    if (sc->sc_flags & SCF_SACK) {
        opt[optlen++] = TCPOPT_NOP;
        opt[optlen++] = TCPOPT_NOP;
        opt[optlen++] = TCPOPT_SACK_PERMITTED;
        opt[optlen++] = TCPOLEN_SACK_PERMITTED;
    }
    if (sc->sc_flags & SCF_TIMESTAMP) {
        u_int32_t *lp = (u_int32_t *)(opt + optlen);
        *lp++ = htonl(TCPOPT_TSTAMP_HDR);
        *lp++ = htonl(tcp_now);
        *lp = htonl(sc->sc_ts_recent);
        optlen += TCPOLEN_TSTAMP_APPA;
    }

    m = m_gethdr(M_DONTWAIT, MT_HEADER);
    if (m == NULL)
        return (ENOBUFS);
    if (max_linkhdr + sizeof(struct tcpiphdr) + optlen > MHLEN) {
        (void) m_free(m);
        return (ENOBUFS);
    }
    m->m_data += max_linkhdr;

    tlen = sizeof(struct tcpiphdr) + optlen;
    ti = mtod(m, struct tcpiphdr *);
    bzero((caddr_t)ti, sizeof(struct tcpiphdr));

    ti->ti_pr = IPPROTO_TCP;
    ti->ti_src = sc->sc_laddr;
    ti->ti_dst = sc->sc_faddr;
    ti->ti_sport = sc->sc_lport;
    ti->ti_dport = sc->sc_fport;
    ti->ti_seq = htonl(sc->sc_iss);
    ti->ti_ack = htonl(sc->sc_irs + 1);
    ti->ti_off = (sizeof(struct tcphdr) + optlen) >> 2;
    ti->ti_flags = TH_SYN | TH_ACK;
    if (sc->sc_flags & SCF_ECN)
        ti->ti_flags |= TH_ECE;
    win = sc->sc_wnd;
    ti->ti_win = htons((u_int16_t)win);
    ti->ti_urp = 0;

    if (optlen)
        bcopy(opt, (caddr_t)(ti + 1), optlen);

    ti->ti_len = htons((u_int16_t)(sizeof(struct tcphdr) + optlen));
    m->m_len = tlen;
    m->m_pkthdr.len = tlen;
    m->m_pkthdr.rcvif = (struct ifnet *)0;

    if (sc->sc_isipv6) {
        /*
         * Slide off the IPv4 ipovly overlay so the mbuf starts at the TCP
         * header, then build and send the IPv6 SYN-ACK.
         */
        int t6 = sizeof(struct tcphdr) + optlen;

        m->m_data      += sizeof(struct ipovly);
        m->m_len        = t6;
        m->m_pkthdr.len = t6;
        (void) tcp_v6output(m, &sc->sc_laddr6, &sc->sc_faddr6,
                            sc->sc_ip_ttl, t6, NULL);
        return (0);
    }

    ti->ti_sum = 0;
    ti->ti_sum = in_cksum(m, tlen);
    ((struct ip *)ti)->ip_len = tlen;
    ((struct ip *)ti)->ip_ttl = sc->sc_ip_ttl ? sc->sc_ip_ttl : ip_defttl;

#ifdef ENABLE_MULTICAST
    (void) ip_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0,
                     (struct ip_moptions *)NULL);
#else
    (void) ip_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0);
#endif
    return (0);
}

/*
 * Fill in the common parts of a syncache entry from a received SYN and the
 * listening socket, then compute our advertised window and scale.
 */
static void
syncache_fill(struct syncache *sc, struct socket *lso, struct sc_key *k,
    struct tcpiphdr *ti, u_int16_t peer_mss, u_int8_t flags, u_int8_t s_scale,
    u_int32_t ts)
{
    long win;
    u_int8_t rscale = 0;

    sc->sc_isipv6 = (k->isipv6 != 0);
    if (k->isipv6) {
        sc->sc_faddr6 = k->faddr6;
        sc->sc_laddr6 = k->laddr6;
    } else {
        sc->sc_faddr = k->faddr;
        sc->sc_laddr = k->laddr;
    }
    sc->sc_fport = k->fport;
    sc->sc_lport = k->lport;
    sc->sc_irs = ti->ti_seq;
    sc->sc_peer_mss = peer_mss;
    sc->sc_flags = flags;
    sc->sc_requested_s_scale = s_scale;
    sc->sc_ts_recent = ts;
    sc->sc_ip_ttl = ip_defttl;
    sc->sc_lso = lso;
    sc->sc_rxmits = 0;

    /* Our receive window scale, derived from the listen socket buffer. */
    if (flags & SCF_WINSCALE) {
        while (rscale < TCP_MAX_WINSHIFT &&
                (TCP_MAXWIN << rscale) < (long)lso->so_rcv.sb_hiwat)
            rscale++;
    }
    sc->sc_request_r_scale = rscale;

    win = sbspace(&lso->so_rcv);
    if (win < 0)
        win = 0;
    win >>= rscale;
    if (win > TCP_MAXWIN)
        win = TCP_MAXWIN;
    sc->sc_wnd = (u_int16_t)win;
}

void
syncache_add(struct socket *lso, struct tcpiphdr *ti,
    u_char *optp, int optlen, struct mbuf *m,
    int isipv6, struct in6_addr *src6, struct in6_addr *dst6)
{
    struct syncache_bucket *sch;
    struct syncache *sc;
    struct sc_key key;
    struct in_addr fa, la;
    u_int16_t peer_mss = 0;
    u_int8_t scflags, s_scale = 0;
    u_int32_t ts = 0;
    int s;

    /* RFC 1122 4.2.3.10: never answer a broadcast/multicast SYN. */
    if (m->m_flags & (M_BCAST | M_MCAST))
        return;
    if (isipv6) {
        if (IN6_IS_ADDR_MULTICAST(dst6))
            return;
    } else if (IN_MULTICAST(ntohl(ti->ti_dst.s_addr)))
        return;

    syncache_key(&key, ti, isipv6, src6, dst6);
    /* v4 / folded-v6 addresses used for the keyed ISN and cookie hashes. */
    if (isipv6) {
        fa = syncache_v6fold(&key.faddr6);
        la = syncache_v6fold(&key.laddr6);
    } else {
        fa = key.faddr;
        la = key.laddr;
    }

    scflags = syncache_parse(optp, optlen, ti->ti_flags,
        &peer_mss, &s_scale, &ts);
    if (peer_mss == 0)
        peer_mss = tcp_mssdflt;

    /* Honour the locally enabled feature set, as the legacy path does. */
    if (!tcp_do_rfc1323)
        scflags &= ~(SCF_WINSCALE | SCF_TIMESTAMP);
    if (!tcp_do_sack)
        scflags &= ~SCF_SACK;
    if (!tcp_do_ecn)
        scflags &= ~SCF_ECN;

    s = splnet();
    sch = &tcp_syncache_tbl[syncache_hashidx(&key)];

    /* Retransmitted SYN for an existing half-open: just resend the SYN-ACK. */
    sc = syncache_lookup(sch, &key);
    if (sc != NULL) {
        tcpstat.tcps_sc_dupsyn++;
        (void) syncache_respond(sc);
        splx(s);
        return;
    }

    /*
     * If a bucket is full, evict the oldest entry and arm the cookie-only
     * pause so we stop committing memory to a flood.
     */
    if (!syncache_cookiesonly() &&
            sch->sch_length >= SYNCACHE_BUCKETLIMIT) {
        struct syncache *old = sch->sch_bucket.lh_first;
        struct syncache *nxt;
        while (old != NULL && (nxt = old->sc_hash.le_next) != NULL)
            old = nxt;
        if (old != NULL)
            syncache_drop(old, sch);
        tcp_syncache_pause_until = tcp_now + SYNCOOKIE_PAUSE;
        tcpstat.tcps_sc_bucketoverflow++;
    }

    if (syncache_cookiesonly()) {
        /* Stateless: encode the connection in the SYN-ACK ISN. */
        struct syncache scs;
        bzero((caddr_t)&scs, sizeof(scs));
        syncache_fill(&scs, lso, &key, ti, peer_mss, 0, 0, 0);
        scs.sc_iss = tcp_syncookie_generate(la, fa,
            key.lport, key.fport, peer_mss);
        tcpstat.tcps_sc_sendcookie++;
        (void) syncache_respond(&scs);
        splx(s);
        return;
    }

    sc = (struct syncache *)bsd_malloc(sizeof(*sc), M_PCB, M_NOWAIT);
    if (sc == NULL) {
        tcpstat.tcps_sc_zonefail++;
        if (tcp_syncookies) {
            struct syncache scs;
            bzero((caddr_t)&scs, sizeof(scs));
            syncache_fill(&scs, lso, &key, ti, peer_mss, 0, 0, 0);
            scs.sc_iss = tcp_syncookie_generate(la, fa,
                key.lport, key.fport, peer_mss);
            tcpstat.tcps_sc_sendcookie++;
            (void) syncache_respond(&scs);
        }
        splx(s);
        return;
    }

    bzero((caddr_t)sc, sizeof(*sc));
    syncache_fill(sc, lso, &key, ti, peer_mss, scflags, s_scale, ts);
    sc->sc_iss = (tcp_seq)tcp_iss + tcp_isn_hash(la.s_addr,
        (u_int32_t)key.lport, fa.s_addr,
        (u_int32_t)key.fport, tcp_secret_key);
    sc->sc_rxttick = tcp_now + SYNCACHE_RXTBASE;

    LIST_INSERT_HEAD(&sch->sch_bucket, sc, sc_hash);
    sch->sch_length++;
    tcp_syncache_count++;
    tcpstat.tcps_sc_added++;

    (void) syncache_respond(sc);
    splx(s);
}

/*
 * Promote a (cached or cookie) half-open connection to a fully established
 * socket.  Returns the new socket, or NULL (and cleans up) on failure.
 */
static struct socket *
syncache_socket(struct syncache *sc, struct socket *lso, struct tcpiphdr *ti)
{
    struct inpcb *inp;
    struct socket *so;
    struct tcpcb *tp;
    struct mbuf *am;
    struct sockaddr_in *sin;
    struct in_addr laddr;
    u_long tiwin;

    so = sonewconn(lso, 0);
    if (so == NULL)
        return (NULL);			/* accept backlog full */

    inp = (struct inpcb *)so->so_pcb;
#if INET6
    if (sc->sc_isipv6) {
        /*
         * Establish the IPv6 four-tuple directly.  IPv6 PCBs are located by a
         * linear scan of the shared tcb list (in6_pcblookup), so unlike the
         * IPv4 path there is no separate hash to rehash and no route to wire
         * up here - tcp_output()/ip6_output() resolve the route lazily.
         */
        inp->inp_laddr6 = sc->sc_laddr6;
        inp->inp_faddr6 = sc->sc_faddr6;
        inp->inp_lport  = sc->sc_lport;
        inp->inp_fport  = sc->sc_fport;
    } else
#endif
    {
        inp->inp_laddr = sc->sc_laddr;
        inp->inp_lport = sc->sc_lport;
        in_pcbrehash(inp);

        /* Wire up the foreign address and route, as the LISTEN path does. */
        am = m_get(M_DONTWAIT, MT_SONAME);
        if (am == NULL) {
            (void) soabort(so);
            return (NULL);
        }
        am->m_len = sizeof(struct sockaddr_in);
        sin = mtod(am, struct sockaddr_in *);
        sin->sin_family = AF_INET;
        sin->sin_len = sizeof(*sin);
        sin->sin_addr = sc->sc_faddr;
        sin->sin_port = sc->sc_fport;
        bzero((caddr_t)sin->sin_zero, sizeof(sin->sin_zero));
        laddr = inp->inp_laddr;
        if (inp->inp_laddr.s_addr == INADDR_ANY)
            inp->inp_laddr = sc->sc_laddr;
        if (in_pcbconnect(inp, am)) {
            inp->inp_laddr = laddr;
            (void) m_free(am);
            (void) soabort(so);
            return (NULL);
        }
        (void) m_free(am);
    }

    tp = intotcpcb(inp);

    /* tcp_output() requires a header template, or it panics on first send. */
    tp->t_template = tcp_template(tp);
    if (tp->t_template == 0) {
        (void) tcp_drop(tp, ENOBUFS);	/* frees the socket */
        return (NULL);
    }

    tp->t_state = TCPS_ESTABLISHED;
    tp->iss = sc->sc_iss;
    tp->irs = sc->sc_irs;
    tcp_sendseqinit(tp);
    tcp_rcvseqinit(tp);
    /* Our SYN-ACK has been acked: advance the send sequence past it. */
    tp->snd_una = sc->sc_iss + 1;
    tp->snd_nxt = sc->sc_iss + 1;
    tp->snd_max = sc->sc_iss + 1;
    tp->snd_wl1 = sc->sc_irs;
    tp->snd_wl2 = sc->sc_iss + 1;

    /* Restore the options negotiated in the SYN exchange. */
    if (sc->sc_flags & SCF_WINSCALE) {
        tp->snd_scale = sc->sc_requested_s_scale;
        tp->rcv_scale = sc->sc_request_r_scale;
        tp->t_flags |= TF_RCVD_SCALE;	/* TF_REQ_SCALE already set */
        tiwin = (u_long)ti->ti_win << sc->sc_requested_s_scale;
    } else {
        tp->t_flags &= ~TF_REQ_SCALE;
        tp->snd_scale = tp->rcv_scale = 0;
        tiwin = ti->ti_win;
    }
    if (sc->sc_flags & SCF_TIMESTAMP) {
        tp->t_flags |= TF_RCVD_TSTMP;
        tp->ts_recent = sc->sc_ts_recent;
        tp->ts_recent_age = tcp_now;
    } else
        tp->t_flags &= ~TF_REQ_TSTMP;
    if (sc->sc_flags & SCF_SACK)
        tp->t_flags |= TF_SACK_PERMIT;
    else
        tp->t_flags &= ~TF_SACK_PERMIT;
    if (sc->sc_flags & SCF_ECN)
        tp->t_flagsext |= TF_ECN_PERMIT;

    tp->snd_wnd = tiwin;
    tp->max_sndwnd = tiwin;

    /* Compute t_maxseg/t_maxopd (and the initial window) from the peer MSS. */
    tcp_mss(tp, sc->sc_peer_mss);

    tp->t_flags |= TF_ACKNOW;
    tp->t_timer[TCPT_KEEP] = tcp_keepidle;

    tcpstat.tcps_accepts++;
    tcpstat.tcps_connects++;
    soisconnected(so);
    return (so);
}

int
syncache_expand(struct socket *lso, struct tcpiphdr *ti,
    u_char *optp, int optlen, struct mbuf *m, struct socket **sop,
    int isipv6, struct in6_addr *src6, struct in6_addr *dst6)
{
    struct syncache_bucket *sch;
    struct syncache *sc;
    struct socket *so;
    struct sc_key key;
    int s;

    syncache_key(&key, ti, isipv6, src6, dst6);

    s = splnet();
    sch = &tcp_syncache_tbl[syncache_hashidx(&key)];
    sc = syncache_lookup(sch, &key);

    if (sc != NULL) {
        /* The ACK must acknowledge our SYN-ACK and be in sequence. */
        if (ti->ti_ack != sc->sc_iss + 1 ||
                ti->ti_seq != sc->sc_irs + 1) {
            splx(s);
            return (0);			/* leave entry; silent drop */
        }
        LIST_REMOVE(sc, sc_hash);
        sch->sch_length--;
        tcp_syncache_count--;
        so = syncache_socket(sc, lso, ti);
        bsd_free((caddr_t)sc, M_PCB);
        if (so == NULL) {
            tcpstat.tcps_sc_aborted++;
            splx(s);
            return (0);
        }
        tcpstat.tcps_sc_completed++;
        *sop = so;
        splx(s);
        return (1);
    }

    /*
     * No cache entry.  The ACK may be completing a stateless cookie
     * handshake (sent while the cache was overflowing).
     */
    if (tcp_syncookies || tcp_syncookiesonly) {
        struct in_addr fa, la;
        u_int16_t mss;

        if (isipv6) {
            fa = syncache_v6fold(&key.faddr6);
            la = syncache_v6fold(&key.laddr6);
        } else {
            fa = key.faddr;
            la = key.laddr;
        }
        mss = tcp_syncookie_validate(la, fa,
            key.lport, key.fport, ti->ti_ack);
        if (mss > 0) {
            struct syncache scs;
            bzero((caddr_t)&scs, sizeof(scs));
            scs.sc_isipv6 = (isipv6 != 0);
            if (isipv6) {
                scs.sc_faddr6 = key.faddr6;
                scs.sc_laddr6 = key.laddr6;
            } else {
                scs.sc_faddr = key.faddr;
                scs.sc_laddr = key.laddr;
            }
            scs.sc_fport = key.fport;
            scs.sc_lport = key.lport;
            scs.sc_irs = ti->ti_seq - 1;
            scs.sc_iss = ti->ti_ack - 1;
            scs.sc_peer_mss = mss;
            scs.sc_ip_ttl = ip_defttl;
            so = syncache_socket(&scs, lso, ti);
            if (so == NULL) {
                splx(s);
                return (0);
            }
            tcpstat.tcps_sc_recvcookie++;
            tcpstat.tcps_sc_completed++;
            *sop = so;
            splx(s);
            return (1);
        }
        tcpstat.tcps_sc_badcookie++;
    }

    splx(s);
    return (0);
}

void
syncache_chkrst(struct tcpiphdr *ti,
    int isipv6, struct in6_addr *src6, struct in6_addr *dst6)
{
    struct syncache_bucket *sch;
    struct syncache *sc;
    struct sc_key key;
    int s;

    syncache_key(&key, ti, isipv6, src6, dst6);

    s = splnet();
    sch = &tcp_syncache_tbl[syncache_hashidx(&key)];
    sc = syncache_lookup(sch, &key);
    /* Only honour a RST whose sequence is the one we expect next. */
    if (sc != NULL && ti->ti_seq == sc->sc_irs + 1) {
        tcpstat.tcps_sc_reset++;
        syncache_drop(sc, sch);
    }
    splx(s);
}

void
syncache_timer(void)
{
    struct syncache_bucket *sch;
    struct syncache *sc, *nxt;
    u_int32_t i;
    int s;

    if (tcp_syncache_tbl == NULL || tcp_syncache_count == 0)
        return;

    s = splnet();
    for (i = 0; i <= tcp_syncache_hashmask; i++) {
        sch = &tcp_syncache_tbl[i];
        for (sc = sch->sch_bucket.lh_first; sc != NULL; sc = nxt) {
            nxt = sc->sc_hash.le_next;
            if (tcp_now < sc->sc_rxttick)
                continue;
            if (sc->sc_rxmits >= SYNCACHE_MAXREXMTS) {
                tcpstat.tcps_sc_timeout++;
                syncache_drop(sc, sch);
                continue;
            }
            sc->sc_rxmits++;
            sc->sc_rxttick = tcp_now +
                (SYNCACHE_RXTBASE << sc->sc_rxmits);
            tcpstat.tcps_sc_retransmitted++;
            (void) syncache_respond(sc);
        }
    }
    splx(s);
}

void
syncache_purge(struct socket *lso)
{
    struct syncache_bucket *sch;
    struct syncache *sc, *nxt;
    u_int32_t i;
    int s;

    if (tcp_syncache_tbl == NULL || tcp_syncache_count == 0)
        return;

    s = splnet();
    for (i = 0; i <= tcp_syncache_hashmask; i++) {
        sch = &tcp_syncache_tbl[i];
        for (sc = sch->sch_bucket.lh_first; sc != NULL; sc = nxt) {
            nxt = sc->sc_hash.le_next;
            if (sc->sc_lso == lso)
                syncache_drop(sc, sch);
        }
    }
    splx(s);
}
