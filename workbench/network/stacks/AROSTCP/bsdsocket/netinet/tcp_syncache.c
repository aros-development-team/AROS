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
#include <netinet/in_pcb.h>
#include <netinet/inetfamily.h>

#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <stddef.h>		/* offsetof */
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include "tcp_compat.h"
#include <netinet/tcp_cc.h>
#include "tcp_syncache.h"

#include <kern/amiga_includes.h>
#include <kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

extern int tcp_do_pmtud;

/* Negotiated/observed options carried for a half-open connection. */
#define SCF_WINSCALE	0x01		/* peer offered window scaling */
#define SCF_TIMESTAMP	0x02		/* peer offered RFC 1323 timestamps */
#define SCF_SACK	0x04		/* peer offered SACK */
#define SCF_ECN		0x08		/* peer requested ECN */

struct syncache {
    LIST_ENTRY(syncache) sc_hash;	/* hash bucket chain */
    const struct inetfamily *sc_inf;	/* the family the SYN came in by */
    union inp_addr  sc_fa;		/* peer (foreign) address, of that family; */
    union inp_addr  sc_la;		/* local address (both zero behind it) */
    u_int16_t       sc_fport;		/* peer (foreign) port */
    u_int16_t       sc_lport;		/* local port */
    tcp_seq         sc_irs;		/* initial received seq (peer ISN) */
    tcp_seq         sc_iss;		/* initial send seq (our ISN) */
    u_int32_t       sc_ts_recent;	/* peer timestamp value to echo */
    u_int32_t       sc_tsoff;		/* our TSval offset (RFC 7323 5.4) */
    u_int16_t       sc_peer_mss;	/* peer's advertised MSS */
    u_int16_t       sc_wnd;		/* our advertised window (16-bit field) */
    u_int8_t        sc_requested_s_scale; /* peer's window scale shift */
    u_int8_t        sc_request_r_scale;	/* our window scale shift */
    u_int8_t        sc_flags;		/* SCF_* */
    u_int8_t        sc_rxmits;		/* SYN-ACK (re)transmit count */
    u_long          sc_rxttick;		/* tcp_now tick of next retransmit */
    struct socket  *sc_lso;		/* the listening socket (parent) */
};

/*
 * The connection key of an incoming segment: the family it came in by, its
 * addresses in that family's form (zero behind the family's address length,
 * so that keys compare as bytes) and its ports.
 * "f" = foreign/peer (segment source), "l" = local (segment destination).
 */
struct sc_key {
    const struct inetfamily *inf;
    union inp_addr  fa, la;
    u_int16_t       fport, lport;
};

static void
syncache_key(struct sc_key *k, const struct inetfamily *inf,
    const union inp_addr *src, const union inp_addr *dst, const tcp_ahdr_t *th)
{
    /* src and dst have the family's address length set and no more: the
     * key is zero behind it */
    bzero((caddr_t)k, sizeof(*k));
    k->inf   = inf;
    inpcb_addrcopy(&k->fa, src, inf->inf_addrlen);
    inpcb_addrcopy(&k->la, dst, inf->inf_addrlen);
    k->fport = th->th_sport;
    k->lport = th->th_dport;
}

/* a key's address length and addresses, for tcp_conn_hash() */
#define SCK_HASHADDRS(k) (k)->inf->inf_addrlen, \
        (const void *)&(k)->la, (const void *)&(k)->fa

LIST_HEAD(sc_head, syncache);

struct syncache_bucket {
    struct sc_head  sch_bucket;		/* chain of entries */
    int             sch_length;		/* number of entries in this bucket */
};

/*
 * Tunables (compile-time).
 *
 * The table is made when the first socket listens (syncache_listen()), not
 * when TCP starts: a machine that only connects out never pays for it. Its
 * buckets are a power of two, one for each SYNCACHE_HASHPER bytes of free
 * memory at that moment, between SYNCACHE_HASHMIN and SYNCACHE_HASHMAX: the
 * whole 512 (4 KB on a 32-bit machine, 8 KB on a 64-bit one) from 8 MB
 * free, 32 (256 bytes on a 32-bit machine) under 1 MB free. Fewer buckets hold fewer half-open
 * connections (SYNCACHE_BUCKETLIMIT each); beyond that the cookies answer.
 */
#define SYNCACHE_HASHMIN	32	/* fewest buckets (power of two) */
#define SYNCACHE_HASHMAX	512	/* most buckets (power of two) */
#define SYNCACHE_HASHPER	(16 * 1024) /* bytes of free memory per bucket */
#define SYNCACHE_BUCKETLIMIT	30	/* max entries per bucket */
#define SYNCACHE_MAXREXMTS	3	/* SYN-ACK retransmits before giving up */
/* base retransmit interval (slow ticks): the initial RTO, 1 s (RFC 6298
 * section 2.1), doubled for each retransmission, as FreeBSD's */
#define SYNCACHE_RXTBASE	(TCPTV_RTOBASE * PR_SLOWHZ)
#define SYNCOOKIE_PAUSE		30	/* cookie-only window after overflow */

static struct syncache_bucket	*tcp_syncache_tbl;
static u_int32_t		 tcp_syncache_hashmask;
static int			 tcp_syncache_count;
static u_long			 tcp_syncache_pause_until;

int tcp_syncookiesonly = 0;		/* force SYN cookies for every SYN */

/*
 * Hash a connection 4-tuple to a bucket index using the keyed ISN hash, so an
 * attacker cannot deliberately overload a single bucket.
 */
static u_int32_t
syncache_hashidx(struct sc_key *k)
{
    return (tcp_conn_hash(tcp_secret_key, TCPH_SYNCACHE, SCK_HASHADDRS(k),
        k->lport, k->fport, 0) & tcp_syncache_hashmask);
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

/*
 * TCP starts: no table yet - the first listen() makes it. Every entry
 * point below returns at once while there is none.
 */
void
syncache_init(void)
{
    tcp_syncache_tbl = NULL;
    tcp_syncache_hashmask = 0;
    tcp_syncache_count = 0;
    tcp_syncache_pause_until = 0;
}

/*
 * A socket is to listen (PRU_LISTEN): make the table if there is none,
 * sized by the memory free now (see SYNCACHE_HASHPER). If the pool cannot
 * give that much, a half as large table is tried, down to the floor.
 * 0, or ENOBUFS when not even the smallest table could be had - then the
 * socket does not listen. The table stays until TCP stops
 * (syncache_deinit()): listeners come and go, and freeing it with the last
 * one would only make the next listen() allocate it again.
 */
int
syncache_listen(void)
{
    struct syncache_bucket *tbl;
    u_long n, want;
    u_int32_t i;

    if (tcp_syncache_tbl != NULL)
        return (0);

    want = (u_long)AvailMem(MEMF_ANY) / SYNCACHE_HASHPER;
    for (n = SYNCACHE_HASHMIN; n < SYNCACHE_HASHMAX && n * 2 <= want; n <<= 1)
        ;
    while ((tbl = bsd_malloc(n * sizeof(struct syncache_bucket),
                             M_PCB, M_NOWAIT)) == NULL) {
        if (n <= SYNCACHE_HASHMIN)
            return (ENOBUFS);
        n >>= 1;
    }
    for (i = 0; i < n; i++) {
        LIST_INIT(&tbl[i].sch_bucket);
        tbl[i].sch_length = 0;
    }
    tcp_syncache_hashmask = n - 1;
    tcp_syncache_count = 0;
    tcp_syncache_tbl = tbl;
    return (0);
}

static struct syncache *
syncache_lookup(struct syncache_bucket *sch, struct sc_key *k)
{
    struct syncache *sc;

    for (sc = sch->sch_bucket.lh_first; sc != NULL; sc = sc->sc_hash.le_next) {
        if (sc->sc_inf != k->inf)
            continue;
        if (sc->sc_fport != k->fport || sc->sc_lport != k->lport)
            continue;
        if (bcmp((caddr_t)&sc->sc_fa, (caddr_t)&k->fa, sizeof(k->fa)) == 0 &&
                bcmp((caddr_t)&sc->sc_la, (caddr_t)&k->la, sizeof(k->la)) == 0)
            return (sc);
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
    const struct inetfamily *inf = sc->sc_inf;
    struct mbuf *m;
    tcp_ahdr_t *th;
    u_char opt[TCP_MAXOLEN];
    int optlen, tlen, win;

    /* MSS option: what the family's MTU towards the peer leaves behind its
     * header and TCP's (RFC 6691). (IPv4 gives the MTU of its default MSS,
     * whatever the route: inf_mtu.) */
    opt[0] = TCPOPT_MAXSEG;
    opt[1] = TCPOLEN_MAXSEG;
    {
        u_int16_t mss;

        mss = htons((u_int16_t)((*inf->inf_mtu)(&sc->sc_fa, NULL) -
                                inf->inf_hdrlen - sizeof(struct tcphdr)));
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
        *lp++ = htonl(tcp_ms() + sc->sc_tsoff);
        *lp = htonl(sc->sc_ts_recent);
        optlen += TCPOLEN_TSTAMP_APPA;
    }

    m = m_gethdr(M_DONTWAIT, MT_HEADER);
    if (m == NULL)
        return (ENOBUFS);
    if (max_linkhdr + inf->inf_hdrlen + sizeof(struct tcphdr) + optlen > MHLEN) {
        (void) m_free(m);
        return (ENOBUFS);
    }
    /* room in front for the family's header */
    m->m_data += max_linkhdr + inf->inf_hdrlen;

    tlen = sizeof(struct tcphdr) + optlen;
    th = mtod(m, tcp_ahdr_t *);
    bzero((caddr_t)th, sizeof(struct tcphdr));

    th->th_sport = sc->sc_lport;
    th->th_dport = sc->sc_fport;
    th->th_seq = htonl(sc->sc_iss);
    th->th_ack = htonl(sc->sc_irs + 1);
    th->th_off = (sizeof(struct tcphdr) + optlen) >> 2;
    th->th_flags = TH_SYN | TH_ACK;
    if (sc->sc_flags & SCF_ECN)
        th->th_flags |= TH_ECE;
    win = sc->sc_wnd;
    th->th_win = htons((u_int16_t)win);
    th->th_urp = 0;

    if (optlen)
        bcopy(opt, (caddr_t)(th + 1), optlen);

    m->m_len = tlen;
    m->m_pkthdr.len = tlen;
    m->m_pkthdr.rcvif = (struct ifnet *)0;

    /*
     * No socket yet: the family's default hop limit, no route kept. Path
     * MTU discovery (RFC 1191) where the family has a DF bit.
     */
    (void) (*inf->inf_output)(m, (struct inpcb *)NULL, &sc->sc_la, &sc->sc_fa,
                              IPPROTO_TCP, offsetof(struct tcphdr, th_sum), 0, 0,
                              tcp_do_pmtud ? INF_OUT_DF : 0);
    return (0);
}

/*
 * Fill in the common parts of a syncache entry from a received SYN and the
 * listening socket, then compute our advertised window and scale.
 */
static void
syncache_fill(struct syncache *sc, struct socket *lso, struct sc_key *k,
    tcp_ahdr_t *th, u_int16_t peer_mss, u_int8_t flags, u_int8_t s_scale,
    u_int32_t ts)
{
    long win;
    u_int8_t rscale = 0;

    sc->sc_inf = k->inf;
    sc->sc_fa = k->fa;
    sc->sc_la = k->la;
    sc->sc_fport = k->fport;
    sc->sc_lport = k->lport;
    sc->sc_tsoff = tcp_conn_hash(tcp_ts_key, TCPH_TSOFF, SCK_HASHADDRS(k),
        k->lport, k->fport, 0);
    sc->sc_irs = th->th_seq;
    sc->sc_peer_mss = peer_mss;
    sc->sc_flags = flags;
    sc->sc_requested_s_scale = s_scale;
    sc->sc_ts_recent = ts;
    sc->sc_lso = lso;
    sc->sc_rxmits = 0;

    /* Our receive window scale, derived from the listen socket buffer
     * (and what autosizing may grow it to). */
    if (flags & SCF_WINSCALE)
        rscale = tcp_rscale(lso);
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
syncache_add(struct socket *lso, const struct inetfamily *inf,
    const union inp_addr *src, const union inp_addr *dst, tcp_ahdr_t *th,
    u_char *optp, int optlen, struct mbuf *m)
{
    struct syncache_bucket *sch;
    struct syncache *sc;
    struct sc_key key;
    u_int16_t peer_mss = 0;
    u_int8_t scflags, s_scale = 0;
    u_int32_t ts = 0;
    int s;

    /* no table: nothing listens (a listener always made one) */
    if (tcp_syncache_tbl == NULL)
        return;
    /* RFC 1122 4.2.3.10: never answer a broadcast/multicast SYN. */
    if (m->m_flags & (M_BCAST | M_MCAST))
        return;
    if ((*inf->inf_addr_multicast)(dst))
        return;

    syncache_key(&key, inf, src, dst, th);

    scflags = syncache_parse(optp, optlen, th->th_flags,
        &peer_mss, &s_scale, &ts);
    if (peer_mss == 0)                  /* RFC 9293 section 3.7.1 defaults */
        peer_mss = *inf->inf_mssdflt;

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
        syncache_fill(&scs, lso, &key, th, peer_mss, 0, 0, 0);
        scs.sc_iss = tcp_syncookie_generate(SCK_HASHADDRS(&key),
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
            syncache_fill(&scs, lso, &key, th, peer_mss, 0, 0, 0);
            scs.sc_iss = tcp_syncookie_generate(SCK_HASHADDRS(&key),
                key.lport, key.fport, peer_mss);
            tcpstat.tcps_sc_sendcookie++;
            (void) syncache_respond(&scs);
        }
        splx(s);
        return;
    }

    bzero((caddr_t)sc, sizeof(*sc));
    syncache_fill(sc, lso, &key, th, peer_mss, scflags, s_scale, ts);
    sc->sc_iss = (tcp_seq)tcp_iss + tcp_conn_hash(tcp_secret_key, TCPH_ISN,
        SCK_HASHADDRS(&key), key.lport, key.fport, 0);
    sc->sc_rxttick = tcp_now + 1 + SYNCACHE_RXTBASE;   /* a tick more: never early */

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
syncache_socket(struct syncache *sc, struct socket *lso, tcp_ahdr_t *th)
{
    struct inpcb *inp;
    struct socket *so;
    struct tcpcb *tp;
    u_long tiwin;

    so = sonewconn(lso, 0);
    if (so == NULL)
        return (NULL);			/* accept backlog full */

    inp = (struct inpcb *)so->so_pcb;
    /*
     * Give the PCB its addresses and ports. A connection of another
     * family than the socket it was accepted by (an IPv4 connection on an
     * IPv6 socket that listens for both) has a PCB of the connection's
     * family; the socket's family describes it to the application. A peer
     * without an address or a port is none - the PCB would read as not
     * connected - and no second connection may have the same four: one
     * can exist that the segment's look-up passed over (bound to another
     * interface).
     */
    if ((inp->inp_family != sc->sc_inf &&
         in_pcbsetfamily(inp, sc->sc_inf) != 0) ||
        sc->sc_fport == 0 ||
        inpcb_addrany(&sc->sc_fa, sc->sc_inf->inf_addrlen) ||
        inpcb_lookup(&tcbinfo, sc->sc_inf, &sc->sc_fa, sc->sc_fport,
                     &sc->sc_la, sc->sc_lport, (const struct ifnet *)NULL,
                     INPLOOKUP_HASHED) != NULL) {
        (void) soabort(so);
        return (NULL);
    }
    inpcb_setconn(inp, &sc->sc_la, sc->sc_lport, &sc->sc_fa, sc->sc_fport,
                  (struct ifnet *)NULL);

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
        tiwin = (u_long)th->th_win << sc->sc_requested_s_scale;
    } else {
        tp->t_flags &= ~TF_REQ_SCALE;
        tp->snd_scale = tp->rcv_scale = 0;
        tiwin = th->th_win;
    }
    if (sc->sc_flags & SCF_TIMESTAMP) {
        tp->t_flags |= TF_RCVD_TSTMP;
        tp->ts_recent = sc->sc_ts_recent;
        tp->ts_recent_age = tcp_now;
        tp->t_ts_offset = sc->sc_tsoff;
    } else
        tp->t_flags &= ~TF_REQ_TSTMP;
    if (sc->sc_flags & SCF_SACK) {
        tp->t_flags |= TF_SACK_PERMIT;
        tcp_rack_attach(tp);
    } else
        tp->t_flags &= ~TF_SACK_PERMIT;
    if (sc->sc_flags & SCF_ECN)
        tp->t_flagsext |= TF_ECN_PERMIT;

    tp->snd_wnd = tiwin;
    tp->max_sndwnd = tiwin;

    /* Compute t_maxseg/t_maxopd (and the initial window) from the peer MSS. */
    tcp_mss(tp, sc->sc_peer_mss);

    /* a SYN-ACK retransmitted: 3 s until an RTT is measured (RFC 6298 5.7) */
    if (sc->sc_rxmits > 0) {
        tp->t_rxtshift = 1;
        tcp_rto_after_syn(tp);
    }

    /* the listening socket's keepalive values (as FreeBSD's syncache) and
     * user timeout */
    {
        const struct tcpcb *ltp = intotcpcb(sotoinpcb(lso));

        if (ltp != NULL) {
            tp->t_keepinit = ltp->t_keepinit;
            tp->t_keepidle = ltp->t_keepidle;
            tp->t_keepintvl = ltp->t_keepintvl;
            tp->t_keepcnt = ltp->t_keepcnt;
            tp->t_uto_ms = ltp->t_uto_ms;   /* as Linux */
        }
    }

    tp->t_flags |= TF_ACKNOW;
    tp->t_timer[TCPT_KEEP] = TP_KEEPIDLE(tp);

    tcpstat.tcps_accepts++;
    tcpstat.tcps_connects++;
    soisconnected(so);
    return (so);
}

int
syncache_expand(struct socket *lso, const struct inetfamily *inf,
    const union inp_addr *src, const union inp_addr *dst, tcp_ahdr_t *th,
    u_char *optp, int optlen, struct mbuf *m, struct socket **sop)
{
    struct syncache_bucket *sch;
    struct syncache *sc;
    struct socket *so;
    struct sc_key key;
    int s;

    /* no table: nothing listens, the ACK completes nothing */
    if (tcp_syncache_tbl == NULL)
        return (0);
    syncache_key(&key, inf, src, dst, th);

    s = splnet();
    sch = &tcp_syncache_tbl[syncache_hashidx(&key)];
    sc = syncache_lookup(sch, &key);

    if (sc != NULL) {
        /*
         * The ACK must acknowledge our SYN-ACK - else a reset, as RFC 9293
         * section 3.10.7.3 has it for SYN-RECEIVED - and be in sequence,
         * else it is dropped. The entry stays either way, for the right ACK.
         */
        if (th->th_ack != sc->sc_iss + 1) {
            splx(s);
            return (0);
        }
        if (th->th_seq != sc->sc_irs + 1) {
            splx(s);
            return (-1);
        }
        LIST_REMOVE(sc, sc_hash);
        sch->sch_length--;
        tcp_syncache_count--;
        so = syncache_socket(sc, lso, th);
        bsd_free((caddr_t)sc, M_PCB);
        if (so == NULL) {
            tcpstat.tcps_sc_aborted++;
            splx(s);
            return (-1);
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
        u_int16_t mss;

        mss = tcp_syncookie_validate(SCK_HASHADDRS(&key),
            key.lport, key.fport, th->th_ack);
        if (mss > 0) {
            struct syncache scs;
            bzero((caddr_t)&scs, sizeof(scs));
            scs.sc_inf = key.inf;
            scs.sc_fa = key.fa;
            scs.sc_la = key.la;
            scs.sc_fport = key.fport;
            scs.sc_lport = key.lport;
            scs.sc_irs = th->th_seq - 1;
            scs.sc_iss = th->th_ack - 1;
            scs.sc_peer_mss = mss;
            so = syncache_socket(&scs, lso, th);
            if (so == NULL) {
                splx(s);
                return (-1);
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
syncache_chkrst(const struct inetfamily *inf, const union inp_addr *src,
    const union inp_addr *dst, tcp_ahdr_t *th)
{
    struct syncache_bucket *sch;
    struct syncache *sc;
    struct sc_key key;
    int s;

    if (tcp_syncache_tbl == NULL)       /* nothing listens */
        return;
    syncache_key(&key, inf, src, dst, th);

    s = splnet();
    sch = &tcp_syncache_tbl[syncache_hashidx(&key)];
    sc = syncache_lookup(sch, &key);
    /* Only honour a RST whose sequence is the one we expect next. */
    if (sc != NULL && th->th_seq == sc->sc_irs + 1) {
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
                1 + (SYNCACHE_RXTBASE << sc->sc_rxmits);
            tcpstat.tcps_sc_retransmitted++;
            (void) syncache_respond(sc);
        }
    }
    splx(s);
}

/*
 * syncache_flush - the half-open connections that came in by family inf
 * are forgotten; inf NULL: all of them. An entry keeps the pointer to its
 * family (sc_inf) and calls through it when its SYN-ACK is sent again, so
 * none of a family's may be left when the family leaves the transports
 * (inetfamily_unregister()); the peers get no answer and try again.
 */
void
syncache_flush(const struct inetfamily *inf)
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
            if (inf == NULL || sc->sc_inf == inf)
                syncache_drop(sc, sch);
        }
    }
    splx(s);
}

/*
 * syncache_deinit - TCP stops: every entry and the table go back to the
 * pool, and the cache's variables are as the file has them.
 */
void
syncache_deinit(void)
{
    int s = splnet();

    syncache_flush(NULL);
    if (tcp_syncache_tbl != NULL)
        bsd_free((caddr_t)tcp_syncache_tbl, M_PCB);
    tcp_syncache_tbl = NULL;
    tcp_syncache_hashmask = 0;
    tcp_syncache_count = 0;
    tcp_syncache_pause_until = 0;
    tcp_syncookiesonly = 0;
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
