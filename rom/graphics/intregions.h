/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Header file for intregions.c
    Lang: english
*/
#include <graphics/gfxbase.h>

BOOL andrectrect(const struct Rectangle* a, const struct Rectangle* b, struct Rectangle* intersect);
void disposerrects(struct RegionRectangle* rr);
BOOL clearrectrect(struct Rectangle* clearrect, struct Rectangle* rect,
		   struct RegionRectangle** erg);
struct RegionRectangle* copyrrects(struct RegionRectangle* src);
struct Region* copyregion(struct Region* r, struct GfxBase* GfxBase);
BOOL clearregionregion(struct Region* r1, struct Region* r2, struct GfxBase* GfxBase);

#define overlap(a,b) (!((a).MaxX < (b).MinX || \
			(a).MaxY < (b).MinY || \
			(a).MinX > (b).MaxX || \
			(a).MinY > (b).MaxY))

