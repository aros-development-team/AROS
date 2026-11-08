/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AllocBitMap() tags private to graphics.library and intuition.library.
*/

#ifndef GRAPHICS_PRIVATE_H
#define GRAPHICS_PRIVATE_H

#include <utility/tagitem.h>

/*
 * ULONG, a CyberGraphX PIXFMT_* up to PIXFMT_RGBA32: the bitmap's format.
 * With BMATags_DisplayID, a displayable bitmap of that mode in this format
 * where its pixels are of the mode's size (OpenScreen()'s SA_PixelFormat).
 */
#define BMATags_CGXPixFmt           (TAG_USER + 0x1000)

#endif /* GRAPHICS_PRIVATE_H */
