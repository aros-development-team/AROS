/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: TCP SYN cache (RFC 4987) - hold half-open connections in a compact
          hash table instead of consuming listen-socket accept-queue slots,
          with stateless SYN cookies used as an overflow fallback.
*/

#ifndef NETINET_TCP_SYNCACHE_H
#define NETINET_TCP_SYNCACHE_H

struct socket;
struct tcphdr;
struct mbuf;
struct inetfamily;
union inp_addr;

/*
 * Public interface used by the TCP input, timer and init paths.
 *
 *   syncache_init    - TCP starts: no table yet (called from tcp_init()).
 *   syncache_listen  - a socket is to listen: make the table, sized by the
 *                      free memory, if there is none yet (PRU_LISTEN). 0, or
 *                      ENOBUFS and the socket does not listen. Every other
 *                      entry returns at once while there is no table.
 *   syncache_add     - handle an incoming SYN for a listening socket: create
 *                      a half-open entry (or emit a SYN cookie under overflow)
 *                      and send the SYN-ACK.  Does not consume the mbuf.
 *   syncache_expand  - handle the final ACK of a handshake: locate the matching
 *                      entry (or validate a SYN cookie) and create the fully
 *                      established socket.  Returns 1 and stores the new socket
 *                      in *sop on success; 0 when the ACK completes nothing
 *                      (no entry and no valid cookie, or an entry whose SYN-ACK
 *                      it does not acknowledge), which the caller answers with
 *                      a reset (RFC 9293 section 3.10.7.2); -1 when it is to be
 *                      dropped quietly (out of sequence for its entry, or no
 *                      socket could be made for it).
 *   syncache_chkrst  - drop a matching half-open entry on a valid RST.
 *   syncache_timer   - retransmit/expire pending entries (called every slow
 *                      timer tick from tcp_slowtimo()).
 *   syncache_purge   - drop all entries belonging to a closing listen socket.
 *   syncache_flush   - drop all entries of a family that leaves the transports.
 *   syncache_deinit  - TCP stops: free the entries and the table.
 *
 * The add/expand/chkrst calls take the family the segment came in by, its
 * source and destination address in that family's form (zero behind the
 * family's address length) and its TCP header (sequence numbers and window
 * in host byte order, as tcp_input() has them).
 */
void syncache_init(void);
int  syncache_listen(void);
void syncache_add(struct socket *lso, const struct inetfamily *inf,
        const union inp_addr *src, const union inp_addr *dst, struct tcphdr *th,
        u_char *optp, int optlen, struct mbuf *m);
int  syncache_expand(struct socket *lso, const struct inetfamily *inf,
        const union inp_addr *src, const union inp_addr *dst, struct tcphdr *th,
        u_char *optp, int optlen, struct mbuf *m, struct socket **sop);
void syncache_chkrst(const struct inetfamily *inf, const union inp_addr *src,
        const union inp_addr *dst, struct tcphdr *th);
void syncache_timer(void);
void syncache_purge(struct socket *lso);
/* a family leaves the transports: its entries go (inf NULL: every entry) */
void syncache_flush(const struct inetfamily *inf);
/* TCP stops: the entries, the table, the cache's variables */
void syncache_deinit(void);

extern int tcp_syncookiesonly;	/* force SYN cookies for every SYN */

#endif /* NETINET_TCP_SYNCACHE_H */
