/* Prototypes for IPv4 checksum functions. */

int in_cksum(register struct mbuf * m,
             register int len);

/* Prototype for IPv6 upper-layer checksum.
 *
 * Computes the ones-complement checksum of the IPv6 pseudo-header
 * (src, dst, upper-layer length, next-header) plus upper-layer data.
 *
 *   m   - mbuf chain starting at the IPv6 header
 *   nxt - next-header value (e.g. IPPROTO_ICMPV6, IPPROTO_UDP);
 *         pass 0 to skip the pseudo-header
 *   off - byte offset to the upper-layer header within the mbuf chain
 *   len - number of upper-layer bytes to checksum
 *
 * Returns 0 when verifying a correct checksum.
 */
#if INET6
int in6_cksum(struct mbuf *m, u_int8_t nxt, u_int32_t off, u_int32_t len);
#endif

