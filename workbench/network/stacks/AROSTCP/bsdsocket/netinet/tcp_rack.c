/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * RACK, Recent ACKnowledgment (RFC 8985 section 6): loss detection by time.
 *
 * A segment is lost when one sent after it has been delivered (acknowledged
 * or SACKed) and more than RACK.rtt + reo_wnd has passed since it was sent:
 * not because DupThresh segments above it were SACKed (RFC 6675's IsLost()),
 * which takes a reordering of three segments for a loss and cannot tell
 * that a retransmission was lost too. reo_wnd, the reordering the
 * connection tolerates, is min_RTT / 4 (at most SRTT), times a multiplier
 * that each D-SACK grows; 0 before any reordering has been seen, once in
 * recovery or with DupThresh segments SACKed - DupThresh's own timing.
 *
 * The send buffer is a byte stream, so the send times live in a map of its
 * own: t_xmap, ranges of the sequence space with the time each was last
 * sent and whether that was a retransmission (tcp_rack_sent(), from
 * tcp_output()), split at the cumulative ACK and the scoreboard's edges as
 * ACKs come (a window often leaves in one millisecond, one range), merged
 * from the oldest end when full. tcp_rack_ack() marks what was delivered,
 * takes RACK.xmit_ts and RACK.rtt from the most recently sent of it - a
 * retransmission only if the timestamp echoed is its own (Karn) - and sets
 * t_rack_lost: the holes below it are lost. RFC 6675 NextSeg()/SetPipe()
 * use it in place of DupThresh (tcp_sack_lostbound()), and loss recovery
 * starts when it is above snd_una, on any ACK. A retransmission found lost
 * is sent again (snd_highrxt moves back to it).
 *
 * Not done: the reordering timer (section 6.2.6) - a loss is found on the
 * next ACK, or by the tail loss probe or the RTO when none comes.
 * net.inet.tcp.rack turns RACK off: DupThresh again.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <sys/malloc.h>
#include <kern/amiga_includes.h>

int tcp_do_rack = 1;

#define RACK_MULT_MAX   16

/*
 * The map's length follows the memory the machine has, the same for every
 * CPU: one entry (12 bytes) per RACK_MAP_PER bytes free when the stack
 * starts, RACK_MAP_MIN to RACK_MAP_MAX (16 with 4 MB free or less, 64 from
 * 16 MB). A full map merges its two oldest entries, so a short one still
 * works, with coarser send times for the oldest data in flight.
 */
#define RACK_MAP_MIN    16
#define RACK_MAP_MAX    64
#define RACK_MAP_PER    (256 * 1024)

int tcp_rack_maplen = RACK_MAP_MIN;

void
tcp_rack_init(void)
{
    u_long n = (u_long)AvailMem(MEMF_ANY) / RACK_MAP_PER;

    tcp_rack_maplen = n < RACK_MAP_MIN ? RACK_MAP_MIN : n > RACK_MAP_MAX ? RACK_MAP_MAX : n;
}

/*
 * A connection that negotiated SACK gets its map; one that did not, or
 * whose map could not be allocated, runs without RACK (DupThresh, as with
 * net.inet.tcp.rack off), and so does one set up while it was off.
 */
void
tcp_rack_attach(struct tcpcb *tp)
{
    if(!tcp_do_rack || tp->t_xmap)
        return;
    tp->t_xmap = bsd_malloc(tcp_rack_maplen * sizeof(struct tcp_xrec), M_PCB, M_NOWAIT);
    tp->t_xmap_len = tp->t_xmap ? tcp_rack_maplen : 0;
    tp->t_xmap_n = 0;
}

void
tcp_rack_detach(struct tcpcb *tp)
{
    if(tp->t_xmap)
        bsd_free(tp->t_xmap, M_PCB);
    tp->t_xmap = NULL;
    tp->t_xmap_len = tp->t_xmap_n = 0;
}

int
tcp_rack_on(const struct tcpcb *tp)
{
    return tcp_do_rack && (tp->t_flags & TF_SACK_PERMIT) && tp->t_xmap;
}

/* where entry i starts */
static tcp_seq
xr_start(const struct tcpcb *tp, int i)
{
    return i ? tp->t_xmap[i - 1].end : tp->t_xmap_start;
}

/* Make room for one entry: merge the two oldest. 0 if there is no pair. */
static int
xr_compact(struct tcpcb *tp)
{
    struct tcp_xrec *a = &tp->t_xmap[0], *b = &tp->t_xmap[1];
    int i;

    if(tp->t_xmap_n < 2)
        return 0;
    b->ts = (int)(a->ts - b->ts) > 0 ? a->ts : b->ts;
    b->flags = ((a->flags | b->flags) & XR_RETX) | (a->flags & b->flags & XR_DELIV);
    for(i = 0; i < tp->t_xmap_n - 1; i++)
        tp->t_xmap[i] = tp->t_xmap[i + 1];
    tp->t_xmap_n--;
    return 1;
}

/* An entry boundary at seq. 0 if there was no room. */
static int
xr_split(struct tcpcb *tp, tcp_seq seq)
{
    int i;

    for(i = 0; i < tp->t_xmap_n; i++) {
        if(SEQ_LEQ(seq, xr_start(tp, i)))
            return 1;
        if(SEQ_LT(seq, tp->t_xmap[i].end))
            break;
    }
    if(i == tp->t_xmap_n)
        return 1;                       /* beyond the map */
    if(tp->t_xmap_n == tp->t_xmap_len) {
        if(!xr_compact(tp))
            return 0;
        return xr_split(tp, seq);       /* the indices moved */
    }
    {
        int j;

        for(j = tp->t_xmap_n; j > i; j--)
            tp->t_xmap[j] = tp->t_xmap[j - 1];
    }
    tp->t_xmap[i].end = seq;
    tp->t_xmap_n++;
    return 1;
}

/* Drop what the cumulative ACK has passed. */
static void
xr_prune(struct tcpcb *tp, tcp_seq ack)
{
    int i = 0;

    while(i < tp->t_xmap_n && SEQ_LEQ(tp->t_xmap[i].end, ack))
        i++;
    if(i) {
        int j;

        for(j = i; j < tp->t_xmap_n; j++)
            tp->t_xmap[j - i] = tp->t_xmap[j];
        tp->t_xmap_n -= i;
    }
    if(SEQ_GT(ack, tp->t_xmap_start))
        tp->t_xmap_start = ack;
}

/* [s, e) of data has just been sent, a retransmission if retx. */
void
tcp_rack_sent(struct tcpcb *tp, tcp_seq s, tcp_seq e, int retx)
{
    u_int32_t now;
    tcp_seq last;

    if(!tcp_rack_on(tp) || SEQ_GEQ(s, e))
        return;
    now = tcp_ms();
    if(tp->t_xmap_n == 0)
        tp->t_xmap_start = s;
    last = tp->t_xmap_n ? tp->t_xmap[tp->t_xmap_n - 1].end : tp->t_xmap_start;
    if(SEQ_LT(s, last)) {
        /* sent again: what lies in the map takes this time */
        tcp_seq to = SEQ_LT(e, last) ? e : last;

        if(SEQ_LT(s, tp->t_xmap_start))
            s = tp->t_xmap_start;
        if(SEQ_LT(s, to) && xr_split(tp, s) && xr_split(tp, to)) {
            int i;

            for(i = 0; i < tp->t_xmap_n; i++) {
                if(SEQ_GEQ(xr_start(tp, i), s) && SEQ_LEQ(tp->t_xmap[i].end, to)) {
                    tp->t_xmap[i].ts = now;
                    tp->t_xmap[i].flags = XR_RETX;
                }
            }
        }
        if(SEQ_LEQ(e, last))
            return;
        s = last;                       /* and new data beyond */
        retx = 0;
    }
    if(tp->t_xmap_n && tp->t_xmap[tp->t_xmap_n - 1].ts == now &&
            tp->t_xmap[tp->t_xmap_n - 1].flags == (retx ? XR_RETX : 0)) {
        tp->t_xmap[tp->t_xmap_n - 1].end = e;
        return;
    }
    if(tp->t_xmap_n == tp->t_xmap_len && !xr_compact(tp))
        return;
    if(tp->t_xmap_n == 0)
        tp->t_xmap_start = s;
    tp->t_xmap[tp->t_xmap_n].end = e;
    tp->t_xmap[tp->t_xmap_n].ts = now;
    tp->t_xmap[tp->t_xmap_n].flags = retx ? XR_RETX : 0;
    tp->t_xmap_n++;
}

/* RFC 8985 section 6.2.4 */
static u_int32_t
rack_reo_wnd(const struct tcpcb *tp)
{
    u_int32_t w, srtt = tp->t_srtt >> TCP_RTT_SHIFT;

    if(!tp->t_rack_reord && ((tp->t_flagsext & TF_IN_FASTRECOV) || tp->t_rxtshift ||
            tp->snd_sacked >= (u_long)tcprexmtthresh * tp->t_maxseg))
        return 0;
    w = tp->t_rack_minrtt / 4 * (tp->t_rack_mult ? tp->t_rack_mult : 1);
    if(srtt && w > srtt)
        w = srtt;
    return w;
}

/* Is [s, e) wholly within a scoreboard range? */
static int
xr_sacked(const struct tcpcb *tp, tcp_seq s, tcp_seq e)
{
    int i;

    for(i = 0; i < tp->t_num_sack_blks; i++)
        if(SEQ_GEQ(s, tp->t_sack_blks[i].start) && SEQ_LEQ(e, tp->t_sack_blks[i].end))
            return 1;
    return 0;
}

/*
 * An ACK, its SACK blocks already in the scoreboard (tcp_sack_update()),
 * before snd_una moves to ack.
 */
void
tcp_rack_ack(struct tcpcb *tp, const struct tcpopt *to, tcp_seq ack)
{
    u_int32_t now, echo = 0;
    tcp_seq lost, fack = tp->t_rack_end;
    int i;

    if(!tcp_rack_on(tp))
        return;
    if(SEQ_LT(ack, tp->snd_una))
        ack = tp->snd_una;
    now = tcp_ms();
    if((to->to_flag & TOF_TS) && to->to_tsecr)
        echo = to->to_tsecr - tp->t_ts_offset;

    /* a D-SACK (RFC 2883): a segment came twice - reordering, or a
     * needless retransmission; widen the window (section 6.2.4) */
    if(tcp_dsack_block(to, ack, NULL)) {
        tp->t_rack_reord = 1;
        if(tp->t_rack_mult < RACK_MULT_MAX)
            tp->t_rack_mult = (tp->t_rack_mult ? tp->t_rack_mult : 1) + 1;
    }

    /* exact edges: what this ACK delivered is whole entries */
    (void)xr_split(tp, ack);
    for(i = 0; i < tp->t_num_sack_blks; i++) {
        (void)xr_split(tp, tp->t_sack_blks[i].start);
        (void)xr_split(tp, tp->t_sack_blks[i].end);
    }

    for(i = 0; i < tp->t_xmap_n; i++) {
        struct tcp_xrec *x = &tp->t_xmap[i];
        tcp_seq s = xr_start(tp, i);

        if(x->flags & XR_DELIV)
            continue;
        if(!SEQ_LEQ(x->end, ack) && !xr_sacked(tp, s, x->end))
            continue;
        x->flags |= XR_DELIV;
        if((x->flags & XR_RETX) && (!echo || (int)(echo - x->ts) < 0))
            continue;                   /* which copy arrived is unknown */
        if(!(x->flags & XR_RETX) && tp->t_rack_ts && SEQ_LT(x->end, fack))
            tp->t_rack_reord = 1;       /* delivered after one sent later */
        if((int)(x->ts - tp->t_rack_ts) > 0 || tp->t_rack_ts == 0 ||
                (x->ts == tp->t_rack_ts && SEQ_GT(x->end, tp->t_rack_end))) {
            tp->t_rack_ts = x->ts ? x->ts : 1;
            tp->t_rack_end = x->end;
            tp->t_rack_rtt = now - x->ts;
            if(tp->t_rack_minrtt == 0 || tp->t_rack_rtt < tp->t_rack_minrtt)
                tp->t_rack_minrtt = tp->t_rack_rtt ? tp->t_rack_rtt : 1;
        }
    }
    xr_prune(tp, ack);

    /* section 6.2.5: lost if sent before RACK.xmit_ts and not delivered
     * within RACK.rtt + reo_wnd of being sent */
    lost = ack;
    if(tp->t_rack_ts) {
        u_int32_t reo = rack_reo_wnd(tp);

        for(i = 0; i < tp->t_xmap_n; i++) {
            struct tcp_xrec *x = &tp->t_xmap[i];

            if(x->flags & XR_DELIV)
                continue;
            if(!((int)(x->ts - tp->t_rack_ts) < 0 ||
                    (x->ts == tp->t_rack_ts && SEQ_LT(x->end, tp->t_rack_end))))
                continue;
            /* past RACK.rtt + reo_wnd, by more than the 1 ms the clock
             * can be off in each (RFC 8985 compares in its own clock's
             * units; tcp_ms() truncates both the RTT and the elapsed time) */
            if((int)(now - (x->ts + tp->t_rack_rtt + reo)) <= 0)
                continue;
            if(SEQ_GT(x->end, lost))
                lost = x->end;
            if((x->flags & XR_RETX) && SEQ_GT(tp->snd_highrxt, xr_start(tp, i)))
                tp->snd_highrxt = xr_start(tp, i);  /* lost again: resend */
        }
    }
    tp->t_rack_lost = lost;
}

/* After an RTO: nothing is known lost by RACK until the next ACK. */
void
tcp_rack_reset(struct tcpcb *tp)
{
    tp->t_rack_lost = tp->snd_una;
}
