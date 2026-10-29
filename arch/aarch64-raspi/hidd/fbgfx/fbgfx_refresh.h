/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Portable bitmap-to-framebuffer refresh helper for the FB Gfx hidd.
          Copies a rectangle of a chunky bitmap into an RGBX (R,G,B,x byte
          order, 4 bytes per pixel) framebuffer. 4 byte sources are copied
          row by row, 1 byte (LUT8) sources are expanded through a 256 entry
          R,G,B palette. Depends only on the C standard headers.
*/

#ifndef FBGFX_REFRESH_H
#define FBGFX_REFRESH_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct FBGfxSurface
{
    uint8_t  *pixels;           /* First byte of the first row */
    size_t    bytes;            /* Bytes available from pixels onwards */
    uint32_t  width;            /* Size in pixels */
    uint32_t  height;
    uint32_t  pitch;            /* Bytes from one row to the next */
    uint8_t   bytes_per_pixel;  /* 1 (LUT8) or 4 (RGBX) */
} FBGfxSurface;

/* ChunkyBM records its allocation length in ULONG, including row padding. */
static inline int fbgfx_buffer_size(uint32_t width, uint32_t height,
                                  uint32_t pitch, uint8_t bytes_per_pixel,
                                  uint32_t *bytes)
{
    if (bytes == NULL || width == 0 || height == 0)
        return 0;
    if (bytes_per_pixel != 1 && bytes_per_pixel != 4)
        return 0;
    if (width > UINT32_MAX / bytes_per_pixel || pitch < width * bytes_per_pixel)
        return 0;
    if (height > UINT32_MAX / pitch)
        return 0;
    *bytes = height * pitch;
    return 1;
}

/*
 * Check that a surface describes memory which can hold all of its rows:
 * the last row ends at (height - 1) * pitch + width * bytes_per_pixel,
 * which must not exceed bytes.
 */
static inline int fbgfx_surface_valid(const FBGfxSurface *s)
{
    size_t row_bytes, needed;

    if (s == NULL || s->pixels == NULL)
        return 0;
    if (s->bytes_per_pixel != 1 && s->bytes_per_pixel != 4)
        return 0;
    if (s->width == 0 || s->height == 0)
        return 0;

    if ((size_t)s->width > SIZE_MAX / s->bytes_per_pixel)
        return 0;
    row_bytes = (size_t)s->width * s->bytes_per_pixel;
    if ((size_t)s->pitch < row_bytes)
        return 0;

    /* pitch is non-zero here, since it is at least row_bytes */
    if ((size_t)(s->height - 1) > SIZE_MAX / s->pitch)
        return 0;
    needed = (size_t)(s->height - 1) * s->pitch;
    if (needed > SIZE_MAX - row_bytes)
        return 0;
    needed += row_bytes;
    if (needed > s->bytes)
        return 0;

    /* The described memory must not wrap around the address space */
    if ((uintptr_t)s->pixels > UINTPTR_MAX - s->bytes)
        return 0;

    return 1;
}

/* Nonzero if the memory ranges of the two surfaces overlap */
static inline int fbgfx_surfaces_overlap(const FBGfxSurface *a, const FBGfxSurface *b)
{
    uintptr_t a_start = (uintptr_t)a->pixels, a_end = a_start + a->bytes;
    uintptr_t b_start = (uintptr_t)b->pixels, b_end = b_start + b->bytes;

    return (a_start < b_end) && (b_start < a_end);
}

/*
 * Copy the source rectangle [x1, x2) x [y1, y2), given in source bitmap
 * coordinates, to the destination. The source origin appears at
 * (xoffset, yoffset) on the destination. The rectangle is clipped against
 * both surfaces.
 *
 * dst must be 4 bytes per pixel. src may be 4 bytes per pixel (copied
 * unchanged) or 1 byte per pixel, in which case palette must point to
 * 256 * 3 bytes of R,G,B entries and the 4th output byte is written as 0.
 *
 * Returns nonzero if any pixel was written, 0 otherwise. Nothing is
 * written when the arguments are invalid or the clipped rectangle is empty.
 * The source is never written and no memory is allocated.
 */
static inline int fbgfx_refresh_rect(const FBGfxSurface *src, FBGfxSurface *dst,
                                     const uint8_t *palette,
                                     int32_t xoffset, int32_t yoffset,
                                     int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int64_t sx1, sy1, sx2, sy2;
    int64_t dx1, dy1, dx2, dy2;
    size_t w, h, row, col;

    if (!fbgfx_surface_valid(src) || !fbgfx_surface_valid(dst))
        return 0;
    if (dst->bytes_per_pixel != 4)
        return 0;
    if (src->bytes_per_pixel == 1 && palette == NULL)
        return 0;
    if (fbgfx_surfaces_overlap(src, dst))
        return 0;

    /* Clip against the source bitmap */
    sx1 = (x1 < 0) ? 0 : x1;
    sy1 = (y1 < 0) ? 0 : y1;
    sx2 = ((int64_t)x2 > (int64_t)src->width)  ? (int64_t)src->width  : x2;
    sy2 = ((int64_t)y2 > (int64_t)src->height) ? (int64_t)src->height : y2;
    if (sx1 >= sx2 || sy1 >= sy2)
        return 0;

    /* Translate to destination coordinates and clip against the destination */
    dx1 = sx1 + xoffset;
    dy1 = sy1 + yoffset;
    dx2 = sx2 + xoffset;
    dy2 = sy2 + yoffset;
    if (dx1 < 0) dx1 = 0;
    if (dy1 < 0) dy1 = 0;
    if (dx2 > (int64_t)dst->width)  dx2 = dst->width;
    if (dy2 > (int64_t)dst->height) dy2 = dst->height;
    if (dx1 >= dx2 || dy1 >= dy2)
        return 0;

    /* Back to source coordinates for the clipped rectangle */
    sx1 = dx1 - xoffset;
    sy1 = dy1 - yoffset;
    w = (size_t)(dx2 - dx1);
    h = (size_t)(dy2 - dy1);

    /*
     * All coordinates are now inside both surfaces, which were validated to
     * hold every pixel of every row, so the offsets below cannot overflow
     * or leave the described memory.
     */
    for (row = 0; row < h; row++)
    {
        const uint8_t *s = src->pixels
                         + ((size_t)sy1 + row) * src->pitch
                         + (size_t)sx1 * src->bytes_per_pixel;
        uint8_t *d = dst->pixels
                   + ((size_t)dy1 + row) * dst->pitch
                   + (size_t)dx1 * 4;

        if (src->bytes_per_pixel == 4)
        {
            memcpy(d, s, w * 4);
        }
        else
        {
            for (col = 0; col < w; col++)
            {
                const uint8_t *entry = palette + (size_t)s[col] * 3;

                d[col * 4 + 0] = entry[0];
                d[col * 4 + 1] = entry[1];
                d[col * 4 + 2] = entry[2];
                d[col * 4 + 3] = 0;
            }
        }
    }

    return 1;
}

#endif /* FBGFX_REFRESH_H */
