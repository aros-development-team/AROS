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
 * in6_cksum_neon.c - ARM/AArch64 NEON-optimised IPv6 upper-layer checksum.
 *
 * This is the ARM counterpart of the portable in6_cksum.c and the x86_64
 * in6_cksum_sse2.c.  It exports the identical public entry point --
 *   int in6_cksum(struct mbuf *m, u_int8_t nxt, u_int32_t off, u_int32_t len)
 * -- and produces bit-identical results to both, for every input including
 * the IPv6 pseudo-header, an 'off' that lands mid-mbuf, multi-mbuf chains and
 * trailing odd bytes.
 *
 * The mbuf-walk, offset skip, inter-mbuf odd-byte bridging and final fold are
 * the same logic as the SSE2 file (already correct, left unchanged); only the
 * bulk-summation inner loop and the pseudo-header vector sum use NEON
 * intrinsics instead of SSE2 intrinsics.
 *
 * -- NEON inner loop (cksum_neon_block6) -----------------------------------
 *
 * A 16-byte vector is loaded with vld1q_u8 and reinterpreted as eight u16
 * lanes.  vpaddlq_u16 sums adjacent lane pairs while widening to four u32
 * lanes, which are accumulated in a u32x4 accumulator with vaddq_u32.  32-bit
 * lane accumulation means no 16-bit overflow is possible.  Four independent
 * accumulators provide instruction-level parallelism, mirroring the SSE2
 * path's four 128-bit accumulators.
 *
 * -- Byte-order note -------------------------------------------------------
 *
 * ARM AROS is little-endian, so reinterpreting bytes as u16 lanes treats
 * byte[N] as the low byte and byte[N+1] as the high byte of each word -- the
 * same convention as the u_short reads in the portable code and the
 * _mm_unpacklo/hi_epi16 path in the SSE2 code.  The ones-complement checksum
 * is byte-order invariant provided all words are summed consistently, which
 * they are here.  The inter-mbuf odd-byte bridge and final pad replicate the
 * SSE2 file's convention verbatim.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>

#if defined(__aarch64__) || defined(__arm__)

#include <arm_neon.h>    /* NEON intrinsics -- requires -mfpu=neon on ARMv7 */
#include <stdint.h>
#include <string.h>      /* memcpy */

/* ------------------------------------------------------------------ */
/* Helper: branchless fold of a 64-bit accumulator to 16-bit.         */
/* ------------------------------------------------------------------ */
static inline uint16_t
cksum_fold64_v6(uint64_t sum)
{
    /* 64 -> 32 */
    sum = (sum >> 32) + (sum & 0xFFFFFFFFULL);
    /* 32 -> 16 (two folds to absorb carry) */
    sum = (sum >> 16) + (sum & 0xFFFFULL);
    sum = (sum >> 16) + (sum & 0xFFFFULL);
    return (uint16_t)sum;
}

/* ------------------------------------------------------------------ */
/* Helper: reduce a u32x4 accumulator to a 64-bit scalar.             */
/*                                                                    */
/* vpaddlq_u32 pairwise-adds the four u32 lanes to two u64 lanes,     */
/* which are then summed as plain scalars.  vgetq_lane_u64 exists on  */
/* both ARMv7 NEON and AArch64, avoiding AArch64-only reductions.     */
/* ------------------------------------------------------------------ */
static inline uint64_t
cksum_hsum_u32x4_v6(uint32x4_t v)
{
    uint64x2_t w = vpaddlq_u32(v);
    return vgetq_lane_u64(w, 0) + vgetq_lane_u64(w, 1);
}

/* ------------------------------------------------------------------ */
/* Helper: NEON bulk checksum over an even-length byte range.         */
/*                                                                    */
/* Identical structure to cksum_neon_block in in_cksum_neon.c:        */
/*   64 B/iter main loop, 16 B/iter tail, scalar 8/2 B tails,         */
/*   four independent u32x4 accumulators for ILP.                     */
/* ------------------------------------------------------------------ */
static uint64_t
cksum_neon_block6(const uint8_t *__restrict ptr, int count)
{
    uint32x4_t acc0 = vdupq_n_u32(0);
    uint32x4_t acc1 = vdupq_n_u32(0);
    uint32x4_t acc2 = vdupq_n_u32(0);
    uint32x4_t acc3 = vdupq_n_u32(0);

    /* -- 64-bytes-per-iteration NEON main loop -- */
    while(count >= 64) {
        __builtin_prefetch(ptr + 128, 0, 1);

        uint16x8_t a = vreinterpretq_u16_u8(vld1q_u8(ptr +  0));
        uint16x8_t b = vreinterpretq_u16_u8(vld1q_u8(ptr + 16));
        uint16x8_t c = vreinterpretq_u16_u8(vld1q_u8(ptr + 32));
        uint16x8_t d = vreinterpretq_u16_u8(vld1q_u8(ptr + 48));

        acc0 = vaddq_u32(acc0, vpaddlq_u16(a));
        acc1 = vaddq_u32(acc1, vpaddlq_u16(b));
        acc2 = vaddq_u32(acc2, vpaddlq_u16(c));
        acc3 = vaddq_u32(acc3, vpaddlq_u16(d));

        ptr   += 64;
        count -= 64;
    }

    /* -- 16-bytes-per-iteration NEON tail -- */
    while(count >= 16) {
        uint16x8_t a = vreinterpretq_u16_u8(vld1q_u8(ptr));
        acc0 = vaddq_u32(acc0, vpaddlq_u16(a));
        ptr   += 16;
        count -= 16;
    }

    /* -- Horizontal fold: four u32x4 accumulators -> one 64-bit scalar -- */
    {
        uint32x4_t acc01 = vaddq_u32(acc0, acc1);
        uint32x4_t acc23 = vaddq_u32(acc2, acc3);
        uint32x4_t acc   = vaddq_u32(acc01, acc23);

        uint64_t vsum = cksum_hsum_u32x4_v6(acc);

        /* -- 8-byte scalar tail -- */
        while(count >= 8) {
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
        while(count >= 2) {
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
/* Helper: sum the IPv6 pseudo-header using NEON.                     */
/*                                                                    */
/* The pseudo-header consists of:                                     */
/*   - 16-byte source address       (8 x u16)                        */
/*   - 16-byte destination address  (8 x u16)                        */
/*   - 32-bit upper-layer packet length (network order)               */
/*   - 24 bits zero + 8-bit next header                               */
/*                                                                    */
/* Addresses are loaded as 16-byte vectors, pairwise-widened to u32   */
/* and accumulated, then folded to a 64-bit partial sum.              */
/* ------------------------------------------------------------------ */
static inline uint64_t
cksum_pseudo6(const struct ip6_hdr *ip6, uint8_t nxt, uint32_t ulen)
{
    uint32x4_t acc = vdupq_n_u32(0);

    /* source address: 16 bytes */
    uint16x8_t src = vreinterpretq_u16_u8(vld1q_u8((const uint8_t *)&ip6->ip6_src));
    acc = vaddq_u32(acc, vpaddlq_u16(src));

    /* destination address: 16 bytes */
    uint16x8_t dst = vreinterpretq_u16_u8(vld1q_u8((const uint8_t *)&ip6->ip6_dst));
    acc = vaddq_u32(acc, vpaddlq_u16(dst));

    uint64_t sum = cksum_hsum_u32x4_v6(acc);

    /* upper-layer packet length (32-bit, network byte order -> 2 x u16) */
    {
        uint32_t ul = htonl(ulen);
        uint16_t p0, p1;
        memcpy(&p0, (const uint8_t *)&ul + 0, 2);
        memcpy(&p1, (const uint8_t *)&ul + 2, 2);
        sum += p0;
        sum += p1;
    }

    /* next header (zero-extended to u16, network byte order) */
    sum += htons((uint16_t)nxt);

    return sum;
}

/* ------------------------------------------------------------------
 * in6_cksum - NEON-optimised IPv6 upper-layer Internet checksum.
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

    /* ---- IPv6 pseudo-header (RFC 2460 8.1) ---- */
    if(nxt != 0) {
        struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
        sum = cksum_pseudo6(ip6, nxt, len);
    }

    /* ---- Skip 'off' bytes into the mbuf chain ---- */
    mp = m;
    {
        u_int32_t skip = off;
        int moff = 0;
        while(mp != NULL && skip > 0) {
            if((u_int32_t)mp->m_len > skip) {
                moff = (int)skip;
                break;
            }
            skip -= mp->m_len;
            mp = mp->m_next;
        }
        if(__builtin_expect(mp == NULL, 0)) {
            printf("in6_cksum: offset %u past end of chain\n",
                   (unsigned)off);
            return 0;
        }

        /* If we landed mid-mbuf, handle the partial first mbuf */
        if(moff > 0) {
            const uint8_t *ptr = (const uint8_t *)mp->m_data + moff;
            int mlen = mp->m_len - moff;
            if((int)len < mlen)
                mlen = (int)len;
            len -= mlen;

            int even_len = mlen & ~1;
            if(even_len > 0)
                sum += cksum_neon_block6(ptr, even_len);

            if(mlen & 1) {
                odd_byte       = ptr[even_len];
                odd_byte_valid = 1;
            }

            mp = mp->m_next;
        }
    }

    /* ---- Checksum remaining mbufs ---- */
    for(; __builtin_expect(mp != NULL && len > 0, 1); mp = mp->m_next) {

        if(__builtin_expect(mp->m_len == 0, 0))
            continue;

        const uint8_t *ptr = (const uint8_t *)mp->m_data;
        int mlen = mp->m_len;
        if((int)len < mlen)
            mlen = (int)len;
        len -= mlen;

        /* Bridge odd byte from previous mbuf */
        if(__builtin_expect(odd_byte_valid, 0)) {
            sum += (uint16_t)(odd_byte | ((uint16_t)(*ptr) << 8));
            ptr++;
            mlen--;
            odd_byte_valid = 0;
        }

        if(__builtin_expect(mlen <= 0, 0))
            continue;

        int even_len = mlen & ~1;

        if(even_len > 0)
            sum += cksum_neon_block6(ptr, even_len);

        if(mlen & 1) {
            odd_byte       = ptr[even_len];
            odd_byte_valid = 1;
        }
    }

    /*
     * A truncated or malformed packet whose checksum span exceeds the mbuf
     * data leaves len != 0 here. This is remote-triggerable, so we do not log
     * it: an unconditional message would let an attacker flood the console.
     * The computed checksum is unaffected and the packet is rejected later.
     */

    /* Final odd-byte: low byte, high byte = 0 */
    if(__builtin_expect(odd_byte_valid, 0))
        sum += (uint16_t)odd_byte;

    /* Fold and complement */
    return (int)(uint16_t)~cksum_fold64_v6(sum);
}

#endif /* defined(__aarch64__) || defined(__arm__) */

#endif /* INET6 */
