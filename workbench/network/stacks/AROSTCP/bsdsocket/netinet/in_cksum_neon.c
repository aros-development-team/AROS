/*
 * Copyright (C) 2026 The AROS Dev Team
 */

/*
 * Copyright (C) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)in_cksum.c  7.3 (Berkeley) 6/28/90
 */

/*
 * ARM / AArch64 NEON-optimised Internet checksum (RFC 1071).
 *
 * This is the ARM counterpart of the x86_64 in_cksum_sse2.c.  It exports the
 * identical public entry point -- int in_cksum(struct mbuf *m, int len) -- and
 * produces bit-identical results to both the portable in_cksum.c and the SSE2
 * variant for every input: multi-mbuf chains, odd-length segments, odd start
 * addresses and trailing odd bytes.
 *
 * The mbuf-walk, the inter-mbuf odd-byte bridging and the final 64->16 fold
 * are the same logic as the SSE2 file (that logic is already correct and is
 * deliberately left unchanged); only the bulk-summation inner loop is
 * re-expressed with NEON intrinsics instead of SSE2 intrinsics.
 *
 * -- NEON inner loop (cksum_neon_block) ------------------------------------
 *
 * A 16-byte NEON vector is loaded with vld1q_u8 and reinterpreted as eight
 * u16 lanes.  vpaddlq_u16 (pairwise long add) sums adjacent lane pairs and
 * widens the result to four u32 lanes -- effectively "unpack + horizontal
 * add" in a single instruction.  Those u32 lanes are added into a u32x4
 * accumulator with vaddq_u32.  Because accumulation happens in 32-bit lanes,
 * no 16-bit overflow is possible: even a maximal 65535-byte IP datagram
 * cannot drive any lane close to the 2^32 limit.
 *
 * Four independent accumulators (acc0-acc3) are kept in parallel to hide the
 * add latency and give the out-of-order engine four independent dependency
 * chains, mirroring the four SSE2 accumulators.  They are pairwise-widened to
 * u64 and reduced to a single 64-bit scalar after the vector section.
 *
 * -- Byte-order note -------------------------------------------------------
 *
 * ARM AROS is little-endian.  Reinterpreting a byte vector as u16 lanes
 * therefore treats byte[N] as the LOW byte and byte[N+1] as the HIGH byte of
 * each word -- exactly what the original code did through its u_short pointer
 * reads and what the SSE2 path does with _mm_unpacklo/hi_epi16.  No explicit
 * byte swap is needed inside cksum_neon_block.
 *
 * The inter-mbuf odd-byte bridge and the final odd-byte pad replicate the
 * SSE2 file's convention verbatim:
 *   - bridge:  word = saved_byte | (new_byte << 8)   (saved = low byte)
 *   - pad:     contribution = (uint16_t)odd_byte      (high byte implicitly 0)
 */

#include <conf.h>

#if defined(__aarch64__) || defined(__arm__)

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <netinet/in_cksum_protos.h>

#include <arm_neon.h>    /* NEON intrinsics -- requires -mfpu=neon on ARMv7 */
#include <stdint.h>
#include <string.h>      /* memcpy */

/* ------------------------------------------------------------------ */
/* Helper: branchless fold of a 64-bit accumulator to a 16-bit        */
/*         ones-complement partial sum.  Identical to the SSE2 file.  */
/* ------------------------------------------------------------------ */
static inline uint16_t
cksum_fold64(uint64_t sum)
{
    /* 64 -> 32 */
    sum = (sum >> 32) + (sum & 0xFFFFFFFFULL);
    /* 32 -> 16 (two folds to absorb any carry from the previous step) */
    sum = (sum >> 16) + (sum & 0xFFFFULL);
    sum = (sum >> 16) + (sum & 0xFFFFULL);
    return (uint16_t)sum;
}

/* ------------------------------------------------------------------ */
/* Helper: reduce a u32x4 accumulator to a 64-bit scalar.             */
/*                                                                    */
/* vpaddlq_u32 pairwise-adds the four u32 lanes into two u64 lanes    */
/* (no overflow: each u64 lane holds the sum of two u32 values), and  */
/* the two lanes are then summed as plain scalars.  vgetq_lane_u64 is */
/* available on both ARMv7 NEON and AArch64, so this avoids the        */
/* AArch64-only across-vector reductions (e.g. vaddvq_u32).            */
/* ------------------------------------------------------------------ */
static inline uint64_t
cksum_hsum_u32x4(uint32x4_t v)
{
    uint64x2_t w = vpaddlq_u32(v);
    return vgetq_lane_u64(w, 0) + vgetq_lane_u64(w, 1);
}

/* ------------------------------------------------------------------ */
/* Helper: NEON bulk checksum over an even-length byte range.         */
/* ------------------------------------------------------------------ */

/*
 * cksum_neon_block()
 *
 * Accumulates the ones-complement 16-bit word sum of 'count' bytes starting
 * at 'ptr' and returns it as a 64-bit partial sum.
 *
 * Preconditions:
 *   - count >= 0 and even (caller handles any odd trailing byte)
 *   - ptr need not be aligned (vld1q_u8 permits unaligned access)
 *
 * Structure mirrors cksum_sse2_block:
 *   - 64-bytes/iteration NEON main loop with four u32x4 accumulators
 *   - 16-bytes/iteration NEON tail
 *   - horizontal reduce of the four accumulators to one 64-bit scalar
 *   - 8-byte and 2-byte scalar tails (memcpy loads to avoid aliasing UB)
 */
static uint64_t
cksum_neon_block(const uint8_t *__restrict ptr, int count)
{
    /*
     * Four independent u32x4 accumulators.  Keeping four separate chains
     * lets the core issue multiple adds per cycle even under add latency,
     * exactly as the four 128-bit SSE2 accumulators do.
     */
    uint32x4_t acc0 = vdupq_n_u32(0);
    uint32x4_t acc1 = vdupq_n_u32(0);
    uint32x4_t acc2 = vdupq_n_u32(0);
    uint32x4_t acc3 = vdupq_n_u32(0);

    /* -- 64-bytes-per-iteration NEON main loop -- */
    while(count >= 64) {
        /*
         * Prefetch 128 bytes ahead so the next two 64-byte blocks are being
         * fetched while the current one is processed.  On a warm cache the
         * hint is a cheap no-op.
         */
        __builtin_prefetch(ptr + 128, 0 /* read */, 1 /* L2 locality */);

        /* Load four 16-byte vectors, each holding 8 u16 words in memory order. */
        uint16x8_t a = vreinterpretq_u16_u8(vld1q_u8(ptr +  0));
        uint16x8_t b = vreinterpretq_u16_u8(vld1q_u8(ptr + 16));
        uint16x8_t c = vreinterpretq_u16_u8(vld1q_u8(ptr + 32));
        uint16x8_t d = vreinterpretq_u16_u8(vld1q_u8(ptr + 48));

        /*
         * vpaddlq_u16 sums each adjacent u16 pair and widens to u32, giving 4
         * u32 lanes per vector with no possibility of 16-bit overflow.  The
         * u32 lanes are then accumulated with vaddq_u32.
         */
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

        uint64_t vsum = cksum_hsum_u32x4(acc);

        /* -- 8-byte scalar tail (4 u16 per iteration) -- */
        while(count >= 8) {
            /*
             * Use memcpy for the u16 loads.  Under -fstrict-aliasing a direct
             * cast of uint8_t* to uint16_t* is UB; memcpy into a local is the
             * standards-compliant equivalent and compiles to a single load.
             */
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
/* Public entry point                                                  */
/* ------------------------------------------------------------------ */

int
in_cksum(struct mbuf *m, int len)
{
    /*
     * 64-bit running sum.  A 64-bit accumulator can absorb up to 2^32
     * additions of 16-bit values before overflowing, so no intermediate
     * REDUCE call is required across the mbuf chain; the single final fold
     * is sufficient.
     */
    uint64_t sum = 0;

    /*
     * Inter-mbuf odd-byte state.
     *
     * When an mbuf ends on an odd byte boundary its trailing byte is saved
     * here.  On the next mbuf that byte is paired with the first incoming
     * byte to form a 16-bit word before the main loops run.
     *
     * Byte-order convention (little-endian):
     *   saved_byte  -> low  byte of the bridging u16
     *   new_byte    -> high byte of the bridging u16
     *   word value  = (uint16_t)(saved_byte | (new_byte << 8))
     *
     * For the final odd byte (RFC 1071 3.b zero-pad on the right):
     *   saved_byte -> low byte, high byte = 0
     *   contribution = (uint16_t)saved_byte   (NOT saved_byte << 8)
     */
    uint8_t odd_byte       = 0;
    int     odd_byte_valid = 0;

    for(; __builtin_expect(m != NULL && len > 0, 1); m = m->m_next) {

        if(__builtin_expect(m->m_len == 0, 0))
            continue;

        const uint8_t *ptr = mtod(m, const uint8_t *);
        int mlen = m->m_len;
        if(mlen > len)
            mlen = len;
        len -= mlen;

        /*
         * Bridge the odd byte from the previous mbuf with the first byte of
         * this mbuf.  The saved byte is the low byte of the bridging word;
         * the new byte becomes the high byte.
         */
        if(__builtin_expect(odd_byte_valid, 0)) {
            sum += (uint16_t)(odd_byte | ((uint16_t)(*ptr) << 8));
            ptr++;
            mlen--;
            odd_byte_valid = 0;
        }

        if(__builtin_expect(mlen <= 0, 0))
            continue;

        /*
         * NEON vld1q_u8 handles unaligned loads, so no forced-alignment peel
         * is needed before the vector loops.  Pass the largest even-length
         * prefix to the NEON/scalar helper.
         */
        int even_len = mlen & ~1;

        if(even_len > 0)
            sum += cksum_neon_block(ptr, even_len);

        /*
         * If mlen is odd, save the trailing byte.  It will be bridged with
         * the first byte of the next mbuf (or zero-padded if this is the last
         * mbuf with data).
         */
        if(mlen & 1) {
            odd_byte       = ptr[even_len];
            odd_byte_valid = 1;
        }
    }

    /*
     * A leftover len means the checksum span exceeded the mbuf data, i.e. a
     * truncated or malformed packet.  Those missing bytes simply do not
     * contribute to the sum; do not log the condition, as it is remotely
     * triggerable and could be abused to flood the console.
     */

    /*
     * Final odd-byte handling (RFC 1071 3.b): the saved byte occupies the low
     * byte position; the high byte is implicitly zero.
     */
    if(__builtin_expect(odd_byte_valid, 0))
        sum += (uint16_t)odd_byte;

    /*
     * Fold the 64-bit accumulator to 16 bits and return the ones-complement
     * (bitwise NOT).  The cast chain returns a non-negative int whose lower
     * 16 bits are the checksum, matching the original prototype.
     */
    return (int)(uint16_t)~cksum_fold64(sum);
}

#endif /* defined(__aarch64__) || defined(__arm__) */
