#ifndef GRAPHICS_REGIONS_H
#define GRAPHICS_REGIONS_H

/*
    (C) 1997 AROS - The Amiga Research OS
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
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

struct Region
{
    struct Rectangle         bounds;
    struct RegionRectangle * RegionRectangle;
#if 1
    struct MinList         * RectPoolList;
#endif
};

struct RegionRectangle
{
    struct RegionRectangle * Next;
    struct RegionRectangle * Prev;
    struct Rectangle         bounds;
};

#endif /* GRAPHICS_REGIONS_H */
