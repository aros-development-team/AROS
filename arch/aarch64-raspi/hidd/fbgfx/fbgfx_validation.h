/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Michele Dipace (@kaffeine1)

    Desc: Geometry and palette validation for the linear framebuffer HIDD.
*/

#ifndef FBGFX_VALIDATION_H
#define FBGFX_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

/* ChunkyBM and HWData store buffer sizes in a 32-bit ULONG. */
static inline int fbgfx_buffer_size(uintptr_t width, uintptr_t height,
                                  uintptr_t pitch, uintptr_t bytes_per_pixel,
                                  uint32_t *bytes)
{
    if (!bytes || !width || !height || !bytes_per_pixel || bytes_per_pixel > 4)
        return 0;
    if (width > UINT32_MAX || height > UINT32_MAX || pitch > UINT32_MAX)
        return 0;
    if (width > UINT32_MAX / bytes_per_pixel || pitch < width * bytes_per_pixel)
        return 0;
    if (height > UINT32_MAX / pitch)
        return 0;
    *bytes = (uint32_t)(height * pitch);
    return 1;
}

static inline int fbgfx_buffer_span(uintptr_t start, uint32_t bytes)
{
    return start != 0 && bytes != 0 && start <= UINTPTR_MAX - bytes;
}

/* Use a wide interval so depth 32 and first + count cannot overflow. */
static inline int fbgfx_color_range(int depth, int indexed, uint32_t first,
                                  uint32_t count, const void *colors)
{
    uint64_t limit;

    if (depth <= 0 || depth > 32)
        return 0;
    limit = UINT64_C(1) << depth;
    if (indexed && limit > 256)
        limit = 256;
    return (uint64_t)first + count <= limit && (!count || colors != NULL);
}

#endif /* FBGFX_VALIDATION_H */
