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
 * in6_cksum_sse2.c - SSE2-optimised IPv6 upper-layer Internet checksum.
 *
 * This is the x86_64 counterpart of the portable in6_cksum.c, using
 * the same SSE2 techniques as the IPv4 in_cksum_sse2.c:
 *
 * 1. SSE2 inner loop  (cksum_sse2_block6)
 *    Processes 64 bytes per iteration with four independent 128-bit
 *    accumulators.  Each 128-bit vector's 8 × u16 words are zero-extended
 *    to u32 via _mm_unpacklo/hi_epi16 before accumulation, eliminating
 *    any risk of 16-bit overflow.
 *
 * 2. 64-bit scalar accumulator
 *    All partial sums are accumulated in a uint64_t, which can absorb
 *    2^32 additions of 16-bit values without overflow.  No intermediate
 *    REDUCE is needed across the mbuf chain.
 *
 * 3. Branchless 64→16 fold  (cksum_fold64_v6)
 *    Three-step carry fold: 64→33→17→16 bits using pure arithmetic.
 *
 * 4. Software prefetch
 *    __builtin_prefetch 128 bytes ahead of the SSE2 read position.
 *
 * 5. memcpy for strict-aliasing-safe scalar word reads.
 *
 * The IPv6 pseudo-header (RFC 2460 / 8200 §8.1) is computed by summing
 * the source and destination addresses as native-endian u16 words
 * (the ones-complement checksum is byte-order invariant when all words
 * are treated consistently), plus the upper-layer length and next-header
 * fields.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>

#include <emmintrin.h>   /* SSE2 intrinsics — requires -msse2 */
#include <stdint.h>
#include <string.h>      /* memcpy */

/* ------------------------------------------------------------------ */
/* Helper: branchless fold of a 64-bit accumulator to 16-bit.         */
/* ------------------------------------------------------------------ */
static inline uint16_t
cksum_fold64_v6(uint64_t sum)
{
    /* 64 → 32 */
    sum = (sum >> 32) + (sum & 0xFFFFFFFFULL);
    /* 32 → 16 (two folds to absorb carry) */
    sum = (sum >> 16) + (sum & 0xFFFFULL);
    sum = (sum >> 16) + (sum & 0xFFFFULL);
    return (uint16_t)sum;
}

/* ------------------------------------------------------------------ */
/* Helper: SSE2 bulk checksum over an even-length byte range.         */
/*                                                                    */
/* Identical algorithm to cksum_sse2_block in in_cksum_sse2.c:        */
/*   64 B/iter main loop, 16 B/iter tail, scalar 8/2 B tails,        */
/*   four independent 128-bit accumulators for ILP.                   */
/* ------------------------------------------------------------------ */
static uint64_t
cksum_sse2_block6(const uint8_t * __restrict ptr, int count)
{
    const __m128i zero = _mm_setzero_si128();

    __m128i acc0 = zero;
    __m128i acc1 = zero;
    __m128i acc2 = zero;
    __m128i acc3 = zero;

    /* -- 64-bytes-per-iteration SSE2 main loop -- */
    while (count >= 64) {
        __builtin_prefetch(ptr + 128, 0, 1);

        __m128i a = _mm_loadu_si128((const __m128i *)(ptr +  0));
        __m128i b = _mm_loadu_si128((const __m128i *)(ptr + 16));
        __m128i c = _mm_loadu_si128((const __m128i *)(ptr + 32));
        __m128i d = _mm_loadu_si128((const __m128i *)(ptr + 48));

        acc0 = _mm_add_epi32(acc0, _mm_unpacklo_epi16(a, zero));
        acc0 = _mm_add_epi32(acc0, _mm_unpackhi_epi16(a, zero));
        acc1 = _mm_add_epi32(acc1, _mm_unpacklo_epi16(b, zero));
        acc1 = _mm_add_epi32(acc1, _mm_unpackhi_epi16(b, zero));
        acc2 = _mm_add_epi32(acc2, _mm_unpacklo_epi16(c, zero));
        acc2 = _mm_add_epi32(acc2, _mm_unpackhi_epi16(c, zero));
        acc3 = _mm_add_epi32(acc3, _mm_unpacklo_epi16(d, zero));
        acc3 = _mm_add_epi32(acc3, _mm_unpackhi_epi16(d, zero));

        ptr   += 64;
        count -= 64;
    }

    /* -- 16-bytes-per-iteration SSE2 tail -- */
    while (count >= 16) {
        __m128i a = _mm_loadu_si128((const __m128i *)ptr);
        acc0 = _mm_add_epi32(acc0, _mm_unpacklo_epi16(a, zero));
        acc0 = _mm_add_epi32(acc0, _mm_unpackhi_epi16(a, zero));
        ptr   += 16;
        count -= 16;
    }

    /* -- Horizontal fold: four 128-bit accumulators → one 64-bit scalar -- */
    {
        __m128i acc01 = _mm_add_epi32(acc0, acc1);
        __m128i acc23 = _mm_add_epi32(acc2, acc3);
        __m128i acc   = _mm_add_epi32(acc01, acc23);

        __m128i acc_hi  = _mm_srli_si128(acc, 8);
        __m128i acc_sum = _mm_add_epi32(acc, acc_hi);

        uint32_t lo = (uint32_t)_mm_cvtsi128_si32(acc_sum);
        uint32_t hi = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(acc_sum, 4));

        uint64_t vsum = (uint64_t)lo + (uint64_t)hi;

        /* -- 8-byte scalar tail -- */
        while (count >= 8) {
            uint16_t w0, w1, w2, w3;
            memcpy(&w0, ptr + 0, 2);
            memcpy(&w1, ptr + 2, 2);
            memcpy(&w2, ptr + 4, 2);
            memcpy(&w3, ptr + 6, 2);
            vsum  += (uint64_t)w0 + (uint64_t)w1 +
                     (uint64_t)w2 + (uint64_t)w3;
            ptr   += 8;
            count -= 8;
        }

        /* -- 2-byte scalar tail -- */
        while (count >= 2) {
            uint16_t w;
            memcpy(&w, ptr, 2);
            vsum  += w;
            ptr   += 2;
            count -= 2;
        }

        return vsum;
    }
}

/* ------------------------------------------------------------------ */
/* Helper: sum the IPv6 pseudo-header using SSE2.                     */
/*                                                                    */
/* The pseudo-header consists of:                                     */
/*   - 16-byte source address  (8 × u16)                             */
/*   - 16-byte destination address (8 × u16)                         */
/*   - 32-bit upper-layer packet length (network order)               */
/*   - 8-bit zero + 8-bit zero + 8-bit zero + 8-bit next header      */
/*                                                                    */
/* Addresses are loaded as 128-bit vectors and zero-extended to u32   */
/* for accumulation, then folded to a 64-bit partial sum.             */
/* ------------------------------------------------------------------ */
static inline uint64_t
cksum_pseudo6(const struct ip6_hdr *ip6, uint8_t nxt, uint32_t ulen)
{
    const __m128i zero = _mm_setzero_si128();
    __m128i acc = zero;

    /* source address: 16 bytes at ip6 + 8 */
    __m128i src = _mm_loadu_si128((const __m128i *)&ip6->ip6_src);
    acc = _mm_add_epi32(acc, _mm_unpacklo_epi16(src, zero));
    acc = _mm_add_epi32(acc, _mm_unpackhi_epi16(src, zero));

    /* destination address: 16 bytes at ip6 + 24 */
    __m128i dst = _mm_loadu_si128((const __m128i *)&ip6->ip6_dst);
    acc = _mm_add_epi32(acc, _mm_unpacklo_epi16(dst, zero));
    acc = _mm_add_epi32(acc, _mm_unpackhi_epi16(dst, zero));

    /* horizontal fold: 128-bit → 64-bit scalar */
    __m128i acc_hi  = _mm_srli_si128(acc, 8);
    __m128i acc_sum = _mm_add_epi32(acc, acc_hi);

    uint32_t lo = (uint32_t)_mm_cvtsi128_si32(acc_sum);
    uint32_t hi = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(acc_sum, 4));

    uint64_t sum = (uint64_t)lo + (uint64_t)hi;

    /* upper-layer packet length (32-bit, network byte order → 2 × u16) */
    {
        uint32_t ul = htonl(ulen);
        uint16_t *p = (uint16_t *)&ul;
        sum += p[0];
        sum += p[1];
    }

    /* next header (zero-extended to u16, network byte order) */
    sum += htons((uint16_t)nxt);

    return sum;
}

/* ------------------------------------------------------------------
 * in6_cksum - SSE2-optimised IPv6 upper-layer Internet checksum.
 *
 * Parameters:
 *   m   - mbuf chain; first mbuf must contain the IPv6 header.
 *   nxt - next-header value for pseudo-header (e.g. IPPROTO_ICMPV6).
 *         Pass 0 to skip the pseudo-header (raw data checksum only).
 *   off - byte offset from start of mbuf data to upper-layer header.
 *   len - number of bytes to checksum (upper-layer header + payload).
 *
 * Returns the 16-bit ones-complement checksum.
 * When verifying a received packet, a return value of 0 means correct.
 * ------------------------------------------------------------------ */
int
in6_cksum(struct mbuf *m, u_int8_t nxt, u_int32_t off, u_int32_t len)
{
    uint64_t sum = 0;
    uint8_t  odd_byte = 0;
    int      odd_byte_valid = 0;
    struct mbuf *mp;

    /* ---- IPv6 pseudo-header (RFC 2460 §8.1) ---- */
    if (nxt != 0) {
        struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
        sum = cksum_pseudo6(ip6, nxt, len);
    }

    /* ---- Skip 'off' bytes into the mbuf chain ---- */
    mp = m;
    {
        u_int32_t skip = off;
        int moff = 0;
        while (mp != NULL && skip > 0) {
            if ((u_int32_t)mp->m_len > skip) {
                moff = (int)skip;
                break;
            }
            skip -= mp->m_len;
            mp = mp->m_next;
        }
        if (__builtin_expect(mp == NULL, 0)) {
            printf("in6_cksum: offset %u past end of chain\n",
                (unsigned)off);
            return 0;
        }

        /* If we landed mid-mbuf, handle the partial first mbuf */
        if (moff > 0) {
            const uint8_t *ptr = (const uint8_t *)mp->m_data + moff;
            int mlen = mp->m_len - moff;
            if ((int)len < mlen)
                mlen = (int)len;
            len -= mlen;

            int even_len = mlen & ~1;
            if (even_len > 0)
                sum += cksum_sse2_block6(ptr, even_len);

            if (mlen & 1) {
                odd_byte       = ptr[even_len];
                odd_byte_valid = 1;
            }

            mp = mp->m_next;
        }
    }

    /* ---- Checksum remaining mbufs ---- */
    for (; __builtin_expect(mp != NULL && len > 0, 1); mp = mp->m_next) {

        if (__builtin_expect(mp->m_len == 0, 0))
            continue;

        const uint8_t *ptr = (const uint8_t *)mp->m_data;
        int mlen = mp->m_len;
        if ((int)len < mlen)
            mlen = (int)len;
        len -= mlen;

        /* Bridge odd byte from previous mbuf */
        if (__builtin_expect(odd_byte_valid, 0)) {
            sum += (uint16_t)(odd_byte | ((uint16_t)(*ptr) << 8));
            ptr++;
            mlen--;
            odd_byte_valid = 0;
        }

        if (__builtin_expect(mlen <= 0, 0))
            continue;

        int even_len = mlen & ~1;

        if (even_len > 0)
            sum += cksum_sse2_block6(ptr, even_len);

        if (mlen & 1) {
            odd_byte       = ptr[even_len];
            odd_byte_valid = 1;
        }
    }

    if (__builtin_expect(len != 0, 0))
        printf("in6_cksum: ran out of data (%u bytes left)\n", (unsigned)len);

    /* Final odd-byte: low byte, high byte = 0 */
    if (__builtin_expect(odd_byte_valid, 0))
        sum += (uint16_t)odd_byte;

    /* Fold and complement */
    return (int)(uint16_t)~cksum_fold64_v6(sum);
}

#endif /* INET6 */
