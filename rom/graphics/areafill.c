/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Area support functions
*/

#include <proto/alib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <string.h>
#include <stdlib.h>

#include "graphics_intern.h"

/*
  The algorithm was taken from:
  Computer Graphics
  A programming approach, 2n edition
  Steven Harrington
  Xerox Corp.

  pages 79-91.


  The algorithm that follows the borderlines has to go hand in hand
  with the algorithm that draws the line, such that no parts are
  filled that aren't supposed to be filled.
 */

void LineInTmpRas(struct RastPort   *rp,
                  struct Rectangle *bounds,
                  UWORD              BytesPerRow,
                  UWORD              xleft,
                  UWORD              xright,
                  UWORD              y,
                  struct GfxBase    *GfxBase);



/* Build an even-odd polygon mask one scanline at a time.  The previous edge
 * state machine allocated a large edge table and repeatedly sorted it while
 * walking the polygon.  Recomputing the few active intersections is smaller
 * and considerably faster for the small polygons used by classic software. */
BOOL areafillpolygon(struct RastPort *rp, struct Rectangle *bounds,
                     UWORD first_idx, UWORD last_idx, ULONG BytesPerRow,
                     struct GfxBase *GfxBase)
{
    struct AreaInfo *ai = rp->AreaInfo;
    UWORD edges = last_idx - first_idx;
    WORD *crossings;
    WORD y;

    /* Filled hands, arrows, diamonds and many other classic UI shapes are
     * convex quadrilaterals.  Drawing their four edges into TmpRas first
     * avoids doing a signed division for every edge on every scanline. */
    if (edges == 4) {
        UWORD *v = &ai->VctrTbl[first_idx * 2];
        LONG winding = 0;
        UWORD vertex;

        for (vertex = 0; vertex < 4; vertex++) {
            UWORD next = (vertex + 1) & 3;
            UWORD after = (vertex + 2) & 3;
            LONG ax = (WORD)v[next * 2] - (WORD)v[vertex * 2];
            LONG ay = (WORD)v[next * 2 + 1] - (WORD)v[vertex * 2 + 1];
            LONG bx = (WORD)v[after * 2] - (WORD)v[next * 2];
            LONG by = (WORD)v[after * 2 + 1] - (WORD)v[next * 2 + 1];
            LONG cross = ax * by - ay * bx;

            if (cross) {
                if (winding && ((cross < 0) != (winding < 0)))
                    break;
                winding = cross;
            }
        }

        if (vertex == 4 && winding) {
            LONG edge_x[4];
            LONG edge_step[4];
            WORD edge_min_y[4];
            WORD edge_max_y[4];

            memset(rp->TmpRas->RasPtr, 0,
                   BytesPerRow * (bounds->MaxY - bounds->MinY + 1));

            for (vertex = 0; vertex < 4; vertex++) {
                UWORD next = (vertex + 1) & 3;
                WORD x1 = v[vertex * 2];
                WORD y1 = v[vertex * 2 + 1];
                WORD x2 = v[next * 2];
                WORD y2 = v[next * 2 + 1];

                if (y1 < y2) {
                    edge_min_y[vertex] = y1;
                    edge_max_y[vertex] = y2;
                    edge_x[vertex] = (LONG)x1 * 65536;
                    edge_step[vertex] =
                        ((LONG)(x2 - x1) * 65536) / (y2 - y1);
                } else if (y2 < y1) {
                    edge_min_y[vertex] = y2;
                    edge_max_y[vertex] = y1;
                    edge_x[vertex] = (LONG)x2 * 65536;
                    edge_step[vertex] =
                        ((LONG)(x1 - x2) * 65536) / (y1 - y2);
                } else {
                    edge_min_y[vertex] = y1;
                    edge_max_y[vertex] = y1;
                    edge_x[vertex] = (LONG)x1 * 65536;
                    edge_step[vertex] = 0;
                }
            }

            for (y = bounds->MinY; y <= bounds->MaxY; y++) {
                LONG left = 0x7fffffff;
                LONG right = -0x7fffffff;

                for (vertex = 0; vertex < 4; vertex++) {
                    if (y >= edge_min_y[vertex] &&
                        y < edge_max_y[vertex]) {
                        if (edge_x[vertex] < left)
                            left = edge_x[vertex];
                        if (edge_x[vertex] > right)
                            right = edge_x[vertex];
                        edge_x[vertex] += edge_step[vertex];
                    }
                }

                if (left <= right)
                    LineInTmpRas(rp, bounds, BytesPerRow,
                                 left >> 16, right >> 16, y, GfxBase);
            }

            areaoutlinepolygonmask(rp, bounds, first_idx, last_idx,
                                   BytesPerRow);
            return TRUE;
        }
    }

    crossings = AllocMem(sizeof(*crossings) * edges, MEMF_ANY);
    if (!crossings)
        return FALSE;

    memset(rp->TmpRas->RasPtr, 0,
           BytesPerRow * (bounds->MaxY - bounds->MinY + 1));

    for (y = bounds->MinY; y <= bounds->MaxY; y++) {
        UWORD count = 0;
        UWORD edge;

        for (edge = first_idx; edge < last_idx; edge++) {
            WORD x1 = ai->VctrTbl[edge * 2];
            WORD y1 = ai->VctrTbl[edge * 2 + 1];
            WORD x2 = ai->VctrTbl[(edge + 1) * 2];
            WORD y2 = ai->VctrTbl[(edge + 1) * 2 + 1];

            if (y1 == y2 || y < (y1 < y2 ? y1 : y2) ||
                y >= (y1 > y2 ? y1 : y2))
                continue;

            crossings[count++] = x1 +
                ((LONG)(y - y1) * (x2 - x1)) / (y2 - y1);
        }

        for (edge = 1; edge < count; edge++) {
            WORD value = crossings[edge];
            WORD pos = edge;
            while (pos && crossings[pos - 1] > value) {
                crossings[pos] = crossings[pos - 1];
                pos--;
            }
            crossings[pos] = value;
        }

        for (edge = 0; edge + 1 < count; edge += 2)
            LineInTmpRas(rp, bounds, BytesPerRow,
                         crossings[edge], crossings[edge + 1], y, GfxBase);
    }

    FreeMem(crossings, sizeof(*crossings) * edges);
    areaoutlinepolygonmask(rp, bounds, first_idx, last_idx, BytesPerRow);
    return TRUE;
}

/* Add the polygon boundary to the mask built by areafillpolygon().  Keeping
 * both the interior and its edge in TmpRas lets AreaEnd apply a normal filled
 * polygon with one BltPattern instead of rendering every edge separately. */
void areaoutlinepolygonmask(struct RastPort *rp, struct Rectangle *bounds,
                            UWORD first_idx, UWORD last_idx,
                            ULONG BytesPerRow)
{
    UWORD *v = rp->AreaInfo->VctrTbl;
    UWORD idx;

    for (idx = first_idx; idx < last_idx; idx++) {
        WORD x = v[idx * 2];
        WORD y = v[idx * 2 + 1];
        WORD x2 = v[(idx + 1) * 2];
        WORD y2 = v[(idx + 1) * 2 + 1];
        WORD dx = x2 >= x ? x2 - x : x - x2;
        WORD dy = y2 >= y ? y2 - y : y - y2;
        WORD sx = x < x2 ? 1 : -1;
        WORD sy = y < y2 ? 1 : -1;
        WORD error = dx - dy;

        for (;;) {
            ULONG rx = (UWORD)(x - bounds->MinX);
            ULONG ry = (UWORD)(y - bounds->MinY);
            UBYTE *byte = (UBYTE *)rp->TmpRas->RasPtr +
                          ry * BytesPerRow + (rx >> 3);
            *byte |= 0x80 >> (rx & 7);

            if (x == x2 && y == y2)
                break;
            {
                WORD twice = error << 1;
                if (twice > -dy) {
                    error -= dy;
                    x += sx;
                }
                if (twice < dx) {
                    error += dx;
                    y += sy;
                }
            }
        }
    }
}

void areafillellipse(struct RastPort   *rp,
                     struct Rectangle *bounds,
                     UWORD             *CurVctr,
                     ULONG              BytesPerRow,
                     struct GfxBase    *GfxBase)
{
    /* the ellipse drawing algorithm is taken from DrawEllipse() */
    LONG x = CurVctr[2], y = 0;   /* ellipse points */

    /* intermediate terms to speed up loop */
    LONG t1 = CurVctr[2] * CurVctr[2], t2 = t1 << 1, t3 = t2 << 1;
    LONG t4 = CurVctr[3] * CurVctr[3], t5 = t4 << 1, t6 = t5 << 1;
    LONG t7 = CurVctr[2] * t5, t8 = t7 << 1, t9 = 0;
    LONG d1 = t2 - t7 + (t4 >> 1);  /* error terms */
    LONG d2 = (t1 >> 1) - t8 + t5;

    memset(rp->TmpRas->RasPtr,
           0x00,
           BytesPerRow * (bounds->MaxY - bounds->MinY + 1));
    while(d2 < 0 && y < CurVctr[3]) {
        /* draw 2 lines using symmetry */
        if(x >= 0) {
            LineInTmpRas(rp,
                         bounds,
                         BytesPerRow,
                         CurVctr[0] - x,
                         CurVctr[0] + x,
                         CurVctr[1] - y,
                         GfxBase);

            LineInTmpRas(rp,
                         bounds,
                         BytesPerRow,
                         CurVctr[0] - x,
                         CurVctr[0] + x,
                         CurVctr[1] + y,
                         GfxBase);
        }

        y++;            /* always move up here */
        t9 = t9 + t3;
        if(d1 < 0) {    /* move straight up */
            d1 = d1 + t9 + t2;
            d2 = d2 + t9;
        } else {
            x--;
            t8 = t8 - t6;
            d1 = d1 + t9 + t2 - t8;
            d2 = d2 + t9 + t5 - t8;
        }
    }

    do {              /* rest of the right quadrant */
        /* draw 2 lines using symmetry */

        x--;         /* always move left here */
        t8 = t8 - t6;
        if(d2 < 0) { /* move up and left */
            if(x >= 0) {
                LineInTmpRas(rp,
                             bounds,
                             BytesPerRow,
                             CurVctr[0] - x,
                             CurVctr[0] + x,
                             CurVctr[1] - y,
                             GfxBase);

                LineInTmpRas(rp,
                             bounds,
                             BytesPerRow,
                             CurVctr[0] - x,
                             CurVctr[0] + x,
                             CurVctr[1] + y,
                             GfxBase);
            } else
                break;

            y ++;
            t9 = t9 + t3;
            d2 = d2 + t9 + t5 - t8;
        } else {    /* move straight left */
            d2 = d2 + t5 - t8;
        }
    } while(x > 0 && y < CurVctr[3]);
}

/*
** Draw a horizontal line into a temporary rastport.
*/

void LineInTmpRas(struct RastPort   *rp,
                  struct Rectangle *bounds,
                  UWORD              BytesPerRow,
                  UWORD              xleft,
                  UWORD              xright,
                  UWORD              y,
                  struct GfxBase    *GfxBase)
{
    ULONG  index;
    UWORD  NumPixels;
    WORD   PixelMask;
    UWORD  PixelMask2;
    UWORD *RasPtr = (WORD *)rp->TmpRas->RasPtr;
    ULONG  shift;

    /* adjust the coordinates */
    xleft  -= bounds->MinX;
    xright -= bounds->MinX;
    y      -= bounds->MinY;

    if(xleft > xright) return;
    /*
      an algorithm that tries to minimize the number of accesses to the
      RasPtr
    */

    /* Fill the first word */
    PixelMask = 0x8000;

    /* determine the number of pixels to set at the beginning */
    NumPixels = xright - xleft + 1;
    if(NumPixels > 16)
        NumPixels = 16;

    /* create enough pixels */
    PixelMask >>= (NumPixels - 1);

    index = (y * (BytesPerRow >> 1)) + (xleft >> 4);
    /* Adjust the pixelmask so we hit the very first pixel  */
    PixelMask2 = PixelMask & 0xffff;
    if(0 != (shift = (xleft & 0x0f))) {
        PixelMask2 >>= shift;
    }

#if (AROS_BIG_ENDIAN == 0)
    /* Endianess conversion*/
    PixelMask2 = PixelMask2 << 8 | PixelMask2 >> 8;
#endif
    RasPtr[index] |= PixelMask2;

    index++;

    xleft = xleft + (16 - shift);

    if((xright - xleft) < 16)
        goto fillright;

    /* fill the middle with 0xffff's */
    while((xleft + 15) < xright) {
        RasPtr[index] = (WORD)0xffff;
        index++;
        xleft += 16;
    }

fillright:
    if(xleft <= xright) {
        PixelMask = 0x8000;
        /* Create enough pixels - one pixel is already there! */
        if(0 != (shift = (xright - xleft + 0))) {
            PixelMask >>= shift;
        }

        PixelMask2 = PixelMask & 0xffff;

#if (AROS_BIG_ENDIAN == 0)
        /* Endianess conversion*/
        PixelMask2 = PixelMask2 << 8 | PixelMask2 >> 8;
#endif

        RasPtr[index] |= PixelMask2;
    }

}


void areaclosepolygon(struct AreaInfo *areainfo)
{
    /* Note: the caller must make sure, that this
       function is only called if areainfo->Count > 0
       and that there is place for one vector
       (areainfo->Count < areainfo->MaxCount) */

    if(areainfo->FlagPtr[-1] == AREAINFOFLAG_DRAW) {
        if((areainfo->VctrPtr[-1] != areainfo->FirstY) ||
                (areainfo->VctrPtr[-2] != areainfo->FirstX)) {
            areainfo->Count++;
            areainfo->VctrPtr[0] = areainfo->FirstX;
            areainfo->VctrPtr[1] = areainfo->FirstY;
            areainfo->FlagPtr[0] = AREAINFOFLAG_CLOSEDRAW;

            areainfo->VctrPtr = &areainfo->VctrPtr[2];
            areainfo->FlagPtr++;
        } else {
            areainfo->FlagPtr[-1] = AREAINFOFLAG_CLOSEDRAW;
        }
    }
}
