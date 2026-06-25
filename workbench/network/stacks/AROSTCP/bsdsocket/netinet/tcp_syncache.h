/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: TCP SYN cache (RFC 4987) - hold half-open connections in a compact
          hash table instead of consuming listen-socket accept-queue slots,
          with stateless SYN cookies used as an overflow fallback.
*/

#ifndef NETINET_TCP_SYNCACHE_H
#define NETINET_TCP_SYNCACHE_H

struct socket;
struct tcpiphdr;
struct mbuf;
struct in6_addr;

/*
 * Public interface used by the TCP input, timer and init paths.
 *
 *   syncache_init    - allocate the hash table (called from tcp_init()).
 *   syncache_add     - handle an incoming SYN for a listening socket: create
 *                      a half-open entry (or emit a SYN cookie under overflow)
 *                      and send the SYN-ACK.  Does not consume the mbuf.
 *   syncache_expand  - handle the final ACK of a handshake: locate the matching
 *                      entry (or validate a SYN cookie) and create the fully
 *                      established socket.  Returns 1 and stores the new socket
 *                      in *sop on success, 0 otherwise.
 *   syncache_chkrst  - drop a matching half-open entry on a valid RST.
 *   syncache_timer   - retransmit/expire pending entries (called every slow
 *                      timer tick from tcp_slowtimo()).
 *   syncache_purge   - drop all entries belonging to a closing listen socket.
 *
 * The add/expand/chkrst calls are dual-stack: isipv6 selects the family, and
 * for IPv6 src6/dst6 carry the segment's peer (source) and local (destination)
 * addresses (the struct tcpiphdr overlay only holds the TCP header then).
 */
void syncache_init(void);
void syncache_add(struct socket *lso, struct tcpiphdr *ti,
        u_char *optp, int optlen, struct mbuf *m,
        int isipv6, struct in6_addr *src6, struct in6_addr *dst6);
int  syncache_expand(struct socket *lso, struct tcpiphdr *ti,
        u_char *optp, int optlen, struct mbuf *m, struct socket **sop,
        int isipv6, struct in6_addr *src6, struct in6_addr *dst6);
void syncache_chkrst(struct tcpiphdr *ti,
        int isipv6, struct in6_addr *src6, struct in6_addr *dst6);
void syncache_timer(void);
void syncache_purge(struct socket *lso);

extern int tcp_syncookiesonly;	/* force SYN cookies for every SYN */

#endif /* NETINET_TCP_SYNCACHE_H */
