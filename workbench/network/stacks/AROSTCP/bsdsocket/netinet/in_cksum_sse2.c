/*
 * Copyright (C) 2026 The AROS Dev Team
 */

/*
 * Copyright (c) 1988 Regents of the University of California.
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
 * x86_64 SSE2-optimised Internet checksum (RFC 1071).
 *
 * -- Optimisations over the original portable version ----------------------
 *
 * 1. SSE2 inner loop  (cksum_sse2_block)
 *    Processes 64 bytes per iteration using four _mm_loadu_si128 loads.
 *    Each 128-bit vector holds 8 × u16 words.  Words are zero-extended to
 *    u32 with _mm_unpacklo/hi_epi16 before accumulation so that no
 *    intermediate 16-bit overflow is possible, regardless of block size.
 *    Four 128-bit accumulators (acc0-acc3) are kept in parallel to hide the
 *    1-cycle SSE2 add latency and give the out-of-order engine four
 *    independent dependency chains.  They are folded to a single 64-bit
 *    scalar after the SSE2 section.
 *
 * 2. 64-bit scalar accumulator
 *    'sum' is promoted from int (32-bit) to uint64_t.  A 64-bit register
 *    can absorb up to 2^32 additions of 16-bit values before overflowing,
 *    so the REDUCE macro (which was a branch + memory store/reload through
 *    a union) is entirely eliminated from the inner loops; only the final
 *    fold remains.
 *
 * 3. No alignment fixup for the main loops
 *    On x86_64 unaligned 128-bit loads are handled in hardware with zero
 *    penalty when the access does not cross a cache-line boundary, and at
 *    most one extra cycle when it does.  The original code forced alignment
 *    with a byte-by-byte peel loop that introduced an extra branch and a
 *    REDUCE call per mbuf.  That peel is retained only for the mandatory
 *    inter-mbuf odd-byte bridging; the SSE2 and scalar inner loops use
 *    _mm_loadu_si128 / plain pointer arithmetic unconditionally.
 *
 * 4. Branchless 64-to-16 fold  (cksum_fold64)
 *    Replaces the ADDCARRY / REDUCE union pair with pure arithmetic:
 *    fold 64?32?16 bits by adding the two halves at each step.  No
 *    branches, no memory traffic, no union aliasing.
 *
 * 5. Software prefetch
 *    A __builtin_prefetch hint is issued 128 bytes ahead of the current
 *    SSE2 position so that the next two cache lines are being fetched while
 *    the current one is being processed.
 *
 * 6. __builtin_expect on rare paths
 *    Zero-length mbuf skip, odd-byte carry, and the error printf are marked
 *    unlikely so the compiler places them off the hot path and avoids
 *    polluting the branch predictor.
 *
 * 7. memcpy for strict-aliasing-safe scalar word reads
 *    The original code cast char* to u_short* throughout, which is
 *    undefined behaviour under -fstrict-aliasing.  Scalar remainder reads
 *    use a memcpy into a local u_short; the compiler reduces this to a
 *    single MOV instruction.
 *
 * -- Byte-order note -------------------------------------------------------
 *
 * The RFC 1071 checksum is defined as the ones-complement sum of all 16-bit
 * words in the data, with any trailing odd byte zero-padded on the right
 * (i.e. placed in the HIGH byte of a u16 on a big-endian machine, or the
 * LOW byte on a little-endian machine).
 *
 * The original code reads data through u_short pointers, which on x86
 * (little-endian) treats byte[N] as the low byte and byte[N+1] as the high
 * byte of each word.  Our SSE2 _mm_loadu_si128 followed by
 * _mm_unpacklo/hi_epi16 does exactly the same thing: it treats each
 * pair of consecutive bytes as a little-endian u16.  No explicit byte-swap
 * is needed inside cksum_sse2_block.
 *
 * The inter-mbuf odd-byte bridge must replicate the same convention:
 *   - the saved trailing byte of mbuf[N] is the LOW byte  of the word
 *   - the first byte of mbuf[N+1]        is the HIGH byte of the word
 *   => bridging word value = saved_byte | (new_byte << 8)
 *
 * The final odd-byte pad (RFC 1071 §3.b) places the saved byte as the low
 * byte and pads with zero on the high side:
 *   => contribution = (uint16_t)odd_byte          (NOT odd_byte << 8)
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <netinet/in_cksum_protos.h>

#include <emmintrin.h>   /* SSE2 intrinsics – requires -msse2                */
#include <stdint.h>
#include <string.h>      /* memcpy                                            */

/* ------------------------------------------------------------------ */
/* Helper: branchless fold of a 64-bit accumulator to a 16-bit        */
/*         ones-complement partial sum.                                */
/* ------------------------------------------------------------------ */

/*
 * cksum_fold64()
 *
 * Three-step carry fold:
 *   step 1  64 ? 33 bits  add the two 32-bit halves (carry sets bit 32)
 *   step 2  33 ? 17 bits  add high half (the potential carry) back in
 *   step 3  17 ? 16 bits  absorb any final carry
 *
 * All operations are on plain unsigned integers; no branches, no memory
 * stores.  The compiler maps each step to at most two instructions
 * (SHR + ADD on x86_64).
 */
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
/* Helper: SSE2 bulk checksum over an even-length byte range.         */
/* ------------------------------------------------------------------ */

/*
 * cksum_sse2_block()
 *
 * Accumulates the ones-complement 16-bit word sum of 'count' bytes
 * starting at 'ptr' and returns it as a 64-bit partial sum.
 *
 * Preconditions:
 *   - count >= 0 and even (caller handles any odd trailing byte)
 *   - ptr need not be aligned
 *
 * Algorithm:
 *
 *   Main loop (64 bytes / iteration):
 *     Load four 128-bit vectors (a,b,c,d) from ptr+0,+16,+32,+48.
 *     Each vector holds 8 × u16 words in memory order.
 *     Zero-extend each vector's 8 words to 8 × u32 using two
 *     _mm_unpacklo/hi_epi16(v, zero) calls, then _mm_add_epi32 to the
 *     corresponding accumulator pair.  Using u32 lanes means each lane
 *     can absorb up to 65535 additions of 0xFFFF before overflowing;
 *     no real packet is large enough to reach that limit.
 *     Four independent accumulators (acc0-acc3) hide the 1-cycle add
 *     latency on out-of-order x86 cores.
 *
 *   16-byte tail loop:
 *     Processes any remaining complete 16-byte blocks with acc0.
 *
 *   Horizontal fold:
 *     The four 128-bit accumulators are pairwise-added to one, then the
 *     four 32-bit lanes are summed via two _mm_srli_si128 + _mm_add_epi32
 *     steps and two _mm_cvtsi128_si32 scalar extractions.
 *
 *   8-byte scalar tail:
 *     Processes 4 × u16 per iteration using memcpy to avoid aliasing UB.
 *
 *   2-byte scalar tail:
 *     Handles any remaining whole word.
 */
static uint64_t
cksum_sse2_block(const uint8_t * __restrict ptr, int count)
{
    const __m128i zero = _mm_setzero_si128();

    /*
     * Four independent 128-bit accumulators, each holding 4 × u32 partial
     * sums.  Keeping four separate chains allows the CPU to execute up to
     * four add µ-ops per cycle even when each has 1-cycle latency.
     */
    __m128i acc0 = zero;
    __m128i acc1 = zero;
    __m128i acc2 = zero;
    __m128i acc3 = zero;

    /* -- 64-bytes-per-iteration SSE2 main loop -- */
    while (count >= 64) {
        /*
         * Prefetch 128 bytes ahead.  On a cold cache this brings the next
         * two 64-byte blocks into L1 while we process the current block.
         * On a warm cache the hint is a cheap no-op.
         */
        __builtin_prefetch(ptr + 128, 0 /* read */, 1 /* L2 locality */);

        __m128i a = _mm_loadu_si128((const __m128i *)(ptr +  0));
        __m128i b = _mm_loadu_si128((const __m128i *)(ptr + 16));
        __m128i c = _mm_loadu_si128((const __m128i *)(ptr + 32));
        __m128i d = _mm_loadu_si128((const __m128i *)(ptr + 48));

        /*
         * _mm_unpacklo_epi16(v, zero) interleaves the lower 4 u16 lanes of
         * v with 0, producing 4 × u32 zero-extended values in the result.
         * _mm_unpackhi_epi16(v, zero) does the same for the upper 4 lanes.
         * Accumulating u32 rather than u16 eliminates all risk of 16-bit
         * overflow inside the loop.
         */
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

    /* -- Horizontal fold: four 128-bit accumulators ? one 64-bit scalar -- */
    {
        /* Fold four accumulators to two, then to one */
        __m128i acc01 = _mm_add_epi32(acc0, acc1);
        __m128i acc23 = _mm_add_epi32(acc2, acc3);
        __m128i acc   = _mm_add_epi32(acc01, acc23);

        /*
         * Shift register right by 8 bytes so lanes [2],[3] align with
         * [0],[1], add, leaving two u32 values in the low 64 bits.
         */
        __m128i acc_hi  = _mm_srli_si128(acc, 8);
        __m128i acc_sum = _mm_add_epi32(acc, acc_hi);

        /* Extract the two remaining u32 lanes as scalars and sum them */
        uint32_t lo = (uint32_t)_mm_cvtsi128_si32(acc_sum);
        uint32_t hi = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(acc_sum, 4));

        /* Accumulate into a 64-bit scalar; no overflow possible here */
        uint64_t vsum = (uint64_t)lo + (uint64_t)hi;

        /* -- 8-byte scalar tail (4 × u16 per iteration) -- */
        while (count >= 8) {
            /*
             * Use memcpy to perform the u16 load.  Under -fstrict-aliasing
             * a direct cast of uint8_t* to uint16_t* is UB; memcpy into a
             * local variable is the standards-compliant equivalent and the
             * compiler will reduce it to a single MOV instruction.
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
/* Public entry point                                                  */
/* ------------------------------------------------------------------ */

int
in_cksum(struct mbuf *m, int len)
{
    /*
     * 64-bit running sum.  A 64-bit accumulator can absorb up to 2^32
     * additions of 16-bit values (max 0xFFFF each) before overflowing,
     * so no intermediate REDUCE call is required across the mbuf chain.
     * The single fold at the end is sufficient.
     */
    uint64_t sum = 0;

    /*
     * Inter-mbuf odd-byte state.
     *
     * When an mbuf ends on an odd byte boundary its trailing byte is saved
     * here.  On the next mbuf that byte is paired with the first incoming
     * byte to form a 16-bit word before the main loops run.
     *
     * Byte-order convention (little-endian x86):
     *   saved_byte  ? low  byte of the bridging u16
     *   new_byte    ? high byte of the bridging u16
     *   word value  = (uint16_t)(saved_byte | (new_byte << 8))
     *
     * This exactly matches what the original code did through its s_util
     * union:
     *   s_util.c[0] = saved_byte;   (stored when the odd mbuf ends)
     *   s_util.c[1] = new_byte;     (stored at the start of the next mbuf)
     *   sum += s_util.s;            (little-endian: c[0] is the low byte)
     *
     * For the final odd byte (RFC 1071 §3.b zero-pad on the right):
     *   saved_byte ? low byte, high byte = 0
     *   contribution = (uint16_t)saved_byte   (NOT saved_byte << 8)
     */
    uint8_t odd_byte       = 0;
    int     odd_byte_valid = 0;

    for (; __builtin_expect(m != NULL && len > 0, 1); m = m->m_next) {

        if (__builtin_expect(m->m_len == 0, 0))
            continue;

        const uint8_t *ptr = mtod(m, const uint8_t *);
        int mlen = m->m_len;
        if (mlen > len)
            mlen = len;
        len -= mlen;

        /*
         * Bridge the odd byte from the previous mbuf with the first byte
         * of this mbuf.  The saved byte is the low byte of the bridging
         * word; the new byte becomes the high byte.
         */
        if (__builtin_expect(odd_byte_valid, 0)) {
            sum += (uint16_t)(odd_byte | ((uint16_t)(*ptr) << 8));
            ptr++;
            mlen--;
            odd_byte_valid = 0;
        }

        if (__builtin_expect(mlen <= 0, 0))
            continue;

        /*
         * x86_64 handles unaligned loads without penalty so no forced-
         * alignment peel is needed before the SSE2 loops.  Pass the largest
         * even-length prefix to the SSE2/scalar helper.
         */
        int even_len = mlen & ~1;

        if (even_len > 0)
            sum += cksum_sse2_block(ptr, even_len);

        /*
         * If mlen is odd, save the trailing byte.  It will be bridged with
         * the first byte of the next mbuf (or zero-padded if this is the
         * last mbuf with data).
         */
        if (mlen & 1) {
            odd_byte       = ptr[even_len];
            odd_byte_valid = 1;
        }
    }

    if (__builtin_expect(len != 0, 0))
        printf("cksum: out of data\n");

    /*
     * Final odd-byte handling (RFC 1071 §3.b):
     * The saved byte occupies the low byte position; the high byte is
     * implicitly zero.  The contribution is simply the byte value itself
     * cast to u16.
     */
    if (__builtin_expect(odd_byte_valid, 0))
        sum += (uint16_t)odd_byte;

    /*
     * Fold the 64-bit accumulator to 16 bits and return the ones-complement
     * (bitwise NOT).  The cast chain ensures we return a non-negative int
     * whose lower 16 bits are the checksum, matching the original prototype.
     */
    return (int)(uint16_t)~cksum_fold64(sum);
}
