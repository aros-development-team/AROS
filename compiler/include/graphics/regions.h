#ifndef GRAPHICS_REGIONS_H
#define GRAPHICS_REGIONS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics region definitions.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif

struct Region
{
    struct Rectangle         bounds;
    struct RegionRectangle * RegionRectangle;
};

struct RegionRectangle
{
    struct RegionRectangle * Next;
    struct RegionRectangle * Prev;
    struct Rectangle         bounds;
};

#endif /* GRAPHICS_REGIONS_H */
