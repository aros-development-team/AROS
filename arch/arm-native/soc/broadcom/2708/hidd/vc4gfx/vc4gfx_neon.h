#ifndef _VIDEOCOREGFX_NEON_H
#define _VIDEOCOREGFX_NEON_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    NEON-accelerated framebuffer memory operations.

    Uses NEON load/store multiple (vldm/vstm) for bulk transfers, faster than
    memcpy for writes to uncached FB memory since NEON stores coalesce in the
    ARM write buffer.

    NEON registers are caller-saved in AROS (VFP saved/restored on task
    switch), so using them in HIDD methods is safe.
*/

#include <exec/types.h>

#define NEON_PREFIX ".fpu neon\n\t"

static inline void neon_copyline(UBYTE *dst, const UBYTE *src, ULONG bytes)
{
    /* 64-byte bulk copy using NEON: vldm/vstm with 4 q-regs (d0-d7) */
#if defined(__arm__)
    while (bytes >= 64)
    {
        asm volatile(
            NEON_PREFIX
            "vldm %[s]!, {d0-d7}\n\t"
            "vstm %[d]!, {d0-d7}\n\t"
            : [s] "+r"(src), [d] "+r"(dst)
            :
            : "d0","d1","d2","d3","d4","d5","d6","d7","memory"
        );
        bytes -= 64;
    }
#endif
    /* Tail (and full copy on non-ARM): word-at-a-time */
    while (bytes >= 4)
    {
        *(ULONG *)dst = *(const ULONG *)src;
        dst += 4;
        src += 4;
        bytes -= 4;
    }
    while (bytes--)
        *dst++ = *src++;
}

/* Copy backward (end toward start). Needed when src/dst alias on the same
 * row with dst > src, where a forward copy would clobber unread source bytes.
 */
static inline void neon_copyline_rev(UBYTE *dst, const UBYTE *src, ULONG bytes)
{
    dst += bytes;
    src += bytes;
#if defined(__arm__)
    while (bytes >= 64)
    {
        src -= 64;
        dst -= 64;
        bytes -= 64;
        asm volatile(
            NEON_PREFIX
            "vldm %[s], {d0-d7}\n\t"
            "vstm %[d], {d0-d7}\n\t"
            : : [s] "r"(src), [d] "r"(dst)
            : "d0","d1","d2","d3","d4","d5","d6","d7","memory"
        );
    }
#endif
    while (bytes >= 4)
    {
        src -= 4;
        dst -= 4;
        bytes -= 4;
        *(ULONG *)dst = *(const ULONG *)src;
    }
    while (bytes--)
    {
        src--;
        dst--;
        *dst = *src;
    }
}

/* Opaque colour expansion of a Native32 mask row: where mask[i] != 0
 * write fg, else write bg. width is in pixels. dst must be 4-byte aligned.
 */
static inline void neon_blit_mask32_row_opaque(UBYTE *dst,
    const ULONG *mask, ULONG width, ULONG fg, ULONG bg)
{
#if defined(__arm__)
    ULONG quads = width >> 2;
    if (quads)
    {
        asm volatile(
            NEON_PREFIX
            "vdup.32 q2, %[fg]\n\t"
            "vdup.32 q3, %[bg]\n\t"
            "1:\n\t"
            "vld1.32 {q0}, [%[m]]!\n\t"
            "vceq.i32 q1, q0, #0\n\t"
            "vbsl q1, q3, q2\n\t"
            "vst1.32 {q1}, [%[d]]!\n\t"
            "subs %[n], %[n], #1\n\t"
            "bne 1b\n\t"
            : [m] "+r"(mask), [d] "+r"(dst), [n] "+r"(quads)
            : [fg] "r"(fg), [bg] "r"(bg)
            : "q0","q1","q2","q3","cc","memory"
        );
    }
    width &= 3;
#endif
    /* Tail (and full row on non-ARM): one pixel at a time */
    while (width--)
    {
        *(ULONG *)dst = (*mask++ != 0) ? fg : bg;
        dst += 4;
    }
}

static inline void neon_fillline(UBYTE *dst, ULONG fill_val, ULONG bytes)
{
#if defined(__arm__)
    asm volatile(
        NEON_PREFIX
        "vdup.32 d0, %[val]\n\t"
        "vmov d1, d0\n\t"
        "vmov d2, d0\n\t"
        "vmov d3, d0\n\t"
        "vmov d4, d0\n\t"
        "vmov d5, d0\n\t"
        "vmov d6, d0\n\t"
        "vmov d7, d0\n\t"
        : : [val] "r"(fill_val) : "d0","d1","d2","d3","d4","d5","d6","d7"
    );
    while (bytes >= 64)
    {
        asm volatile(
            NEON_PREFIX
            "vstm %[d]!, {d0-d7}\n\t"
            : [d] "+r"(dst)
            :
            : "memory"
        );
        bytes -= 64;
    }
#endif
    /* Tail (and full fill on non-ARM): word-at-a-time */
    while (bytes >= 4)
    {
        *(ULONG *)dst = fill_val;
        dst += 4;
        bytes -= 4;
    }
}

#endif /* _VIDEOCOREGFX_NEON_H */
