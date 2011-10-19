/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some common macros for rectangles and regions.
    Lang: english
*/

#ifndef INTREGIONS_H
#define INTREGIONS_H

#define MinX(rr)   ((rr)->bounds.MinX)
#define MaxX(rr)   ((rr)->bounds.MaxX)
#define MinY(rr)   ((rr)->bounds.MinY)
#define MaxY(rr)   ((rr)->bounds.MaxY)

/* CHECKME: Aren't the following candidates for moving into graphics/gfxmacros.h ? */

#define _DoRectsOverlap(Rect, Rect2)	      \
(                                             \
    (Rect2)->MinY <= (Rect)->MaxY &&          \
    (Rect2)->MaxY >= (Rect)->MinY &&          \
    (Rect2)->MinX <= (Rect)->MaxX &&          \
    (Rect2)->MaxX >= (Rect)->MinX             \
)

#define overlap(a, b) _DoRectsOverlap(&(a), &(b))

#define _TranslateRect(rect, dx, dy) \
do                                   \
{                                    \
    (rect)->MinX += dx;              \
    (rect)->MinY += dy;              \
    (rect)->MaxX += dx;              \
    (rect)->MaxY += dy;              \
} while(0)

#define InitRegion(region)            \
do                                    \
{                                     \
    (region)->bounds.MinX = 0;	      \
    (region)->bounds.MinY = 0;        \
    (region)->bounds.MaxX = 0;        \
    (region)->bounds.MaxY = 0;        \
    (region)->RegionRectangle = NULL; \
} while (0)

#endif /* !INTREGIONS_H */
