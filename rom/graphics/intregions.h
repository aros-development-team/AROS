/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Header file for intregions.c
    Lang: english
*/
#include <graphics/gfxbase.h>

BOOL andrectrect(const struct Rectangle* a, const struct Rectangle* b, struct Rectangle* intersect);
BOOL clearrectrect(struct Rectangle* clearrect, struct Rectangle* rect,
		   struct RegionRectangle** erg);
struct RegionRectangle* copyrrects(struct RegionRectangle* src);

#define overlap(a,b) (!((a).MaxX < (b).MinX || \
			(a).MaxY < (b).MinY || \
			(a).MinX > (b).MaxX || \
			(a).MinY > (b).MaxY))

