/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Header file for intregions.c
    Lang: english
*/
#include <graphics/gfxbase.h>

BOOL clearrectrect(struct Rectangle* clearrect, struct Rectangle* rect,
		   struct RegionRectangle** erg);

struct RegionRectangle* copyrrects(struct RegionRectangle* src);

#define overlap(a,b) (((a).MinY <= (b).MaxY) && \
                      ((a).MinX <= (b).MaxX) && \
                      ((a).MaxX >= (b).MinX) && \
                      ((a).MaxY >= (b).MinY))


#define _AndRectRect(rect1, rect2, intersect)                  \
({                                                             \
    BOOL res;                                                  \
                                                               \
    if (overlap(*(rect1), *(rect2)))                           \
    {                                                          \
	(intersect)->MinX = MAX((rect1)->MinX, (rect2)->MinX); \
 	(intersect)->MinY = MAX((rect1)->MinY, (rect2)->MinY); \
	(intersect)->MaxX = MIN((rect1)->MaxX, (rect2)->MaxX); \
 	(intersect)->MaxY = MIN((rect1)->MaxY, (rect2)->MaxY); \
                                                               \
	res = TRUE;                                            \
    }                                                          \
    else                                                       \
        res = FALSE;                                           \
                                                               \
    res;                                                       \
})
