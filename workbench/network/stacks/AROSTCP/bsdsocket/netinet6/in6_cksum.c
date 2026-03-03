/*
 * Copyright (C) 2026 The AROS Development Team.  All rights reserved.
 *
 * Based on FreeBSD/KAME in6_cksum.c:
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * in6_cksum.c - Internet checksum for IPv6 upper-layer protocols.
 *
 * Computes the ones-complement checksum of the IPv6 pseudo-header
 * (RFC 2460 / 8200 §8.1) plus the upper-layer payload.  Used by
 * ICMPv6, UDP/IPv6, TCP/IPv6, and any other protocol that requires
 * a pseudo-header checksum over IPv6.
 *
 * The implementation mirrors the portable in_cksum() style:
 * it walks the mbuf chain, handling cross-mbuf odd-byte boundaries,
 * and uses a 32-bit accumulator with periodic folding.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>

/* ------------------------------------------------------------------
 * in6_cksum - compute the Internet checksum for IPv6 upper layers.
 *
 * Parameters:
 *   m   - mbuf chain; first mbuf must contain the IPv6 header.
 *   nxt - next-header value for pseudo-header (e.g. IPPROTO_ICMPV6).
 *         Pass 0 to skip the pseudo-header (raw data checksum only).
 *   off - byte offset from start of mbuf data to the upper-layer header.
 *   len - number of bytes to checksum (upper-layer header + payload).
 *
 * Returns the 16-bit ones-complement checksum.
 * When verifying a received packet, a return value of 0 means the
 * checksum is correct.
 * ------------------------------------------------------------------ */
int
in6_cksum(struct mbuf *m, u_int8_t nxt, u_int32_t off, u_int32_t len)
{
	u_int32_t sum = 0;
	struct mbuf *mp;
	int moff;		/* offset within current mbuf */
	int mlen;		/* usable bytes in current mbuf */
	int dlen;		/* remaining data bytes to checksum */
	u_int16_t *w;
	int byte_swapped = 0;
	union {
		char	c[2];
		u_int16_t s;
	} s_util;
	union {
		u_int16_t s[2];
		u_int32_t l;
	} l_util;

#define ADDCARRY(x) (x > 65535 ? x -= 65535 : x)
#define REDUCE { l_util.l = sum; sum = l_util.s[0] + l_util.s[1]; ADDCARRY(sum); }

	/* ---- IPv6 pseudo-header (RFC 2460 §8.1) ---- */
	if (nxt != 0) {
		struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
		u_int16_t *addr;
		int i;

		/* source address: 16 bytes = 8 words */
		addr = (u_int16_t *)&ip6->ip6_src;
		for (i = 0; i < 8; i++)
			sum += addr[i];

		/* destination address: 16 bytes = 8 words */
		addr = (u_int16_t *)&ip6->ip6_dst;
		for (i = 0; i < 8; i++)
			sum += addr[i];

		/* upper-layer packet length (32-bit, network byte order) */
		{
			u_int32_t ul = htonl(len);
			u_int16_t *p = (u_int16_t *)&ul;
			sum += p[0];
			sum += p[1];
		}

		/* next header (16-bit word with zero high byte) */
		sum += htons(nxt);

		REDUCE;
	}

	/* ---- Skip 'off' bytes into the mbuf chain ---- */
	mp = m;
	moff = 0;
	{
		u_int32_t skip = off;
		while (mp != NULL && skip > 0) {
			if ((u_int32_t)mp->m_len > skip) {
				moff = skip;
				break;
			}
			skip -= mp->m_len;
			mp = mp->m_next;
		}
		if (mp == NULL) {
			printf("in6_cksum: offset %u past end of chain\n",
			    (unsigned)off);
			return 0;
		}
	}

	/* ---- Checksum 'len' bytes of upper-layer data ---- */
	dlen = (int)len;
	byte_swapped = 0;

	while (mp != NULL && dlen > 0) {
		if (mp->m_len <= moff) {
			mp = mp->m_next;
			moff = 0;
			continue;
		}

		w = (u_int16_t *)((u_int8_t *)mp->m_data + moff);
		mlen = mp->m_len - moff;
		moff = 0;

		if (dlen < mlen)
			mlen = dlen;
		dlen -= mlen;

		/*
		 * Handle inter-mbuf odd-byte bridge.
		 */
		if (mlen > 0 && s_util.c[0] != 0 && byte_swapped == 0) {
			/* This is handled below; we get here only if
			 * the previous mbuf ended on an odd byte. */
		}

		/*
		 * Force to even boundary.
		 */
		if ((1 & (long)w) && (mlen > 0)) {
			REDUCE;
			sum <<= 8;
			s_util.c[0] = *(u_char *)w;
			w = (u_int16_t *)((char *)w + 1);
			mlen--;
			byte_swapped = 1;
		}

		/* Unrolled loop: 32 bytes at a time */
		while ((mlen -= 32) >= 0) {
			sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3];
			sum += w[4]; sum += w[5]; sum += w[6]; sum += w[7];
			sum += w[8]; sum += w[9]; sum += w[10]; sum += w[11];
			sum += w[12]; sum += w[13]; sum += w[14]; sum += w[15];
			w += 16;
		}
		mlen += 32;

		/* 8 bytes at a time */
		while ((mlen -= 8) >= 0) {
			sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3];
			w += 4;
		}
		mlen += 8;

		if (mlen == 0 && byte_swapped == 0) {
			mp = mp->m_next;
			continue;
		}
		REDUCE;

		/* 2 bytes at a time */
		while ((mlen -= 2) >= 0) {
			sum += *w++;
		}

		if (byte_swapped) {
			REDUCE;
			sum <<= 8;
			byte_swapped = 0;
			if (mlen == -1) {
				s_util.c[1] = *(char *)w;
				sum += s_util.s;
				mlen = 0;
			} else
				mlen = -1;
		} else if (mlen == -1) {
			s_util.c[0] = *(char *)w;
		}

		mp = mp->m_next;
	}

	if (dlen > 0)
		printf("in6_cksum: ran out of data (%d bytes left)\n", dlen);

	/* Handle trailing odd byte */
	if (mlen == -1) {
		s_util.c[1] = 0;
		sum += s_util.s;
	}

	REDUCE;
	return (~sum & 0xffff);

#undef ADDCARRY
#undef REDUCE
}

#endif /* INET6 */
