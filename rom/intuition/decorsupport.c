/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.

    Desc: Drawing helpers shared by the decoration and sysimage classes
*/

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>

#include <proto/graphics.h>

#include "intuition_intern.h"
#include "decorsupport.h"

/**************************************************************************************************/

VOID getintframesizes(struct IntuitionBase *IntuitionBase, WORD *xsize, WORD *ysize)
{
    switch (FRAME_SIZE(IntuitionBase))
    {
        case FRAMESIZE_THICK:
            *xsize = 2;
            *ysize = 2;
            break;
        case FRAMESIZE_MEDRES:
            *xsize = 2;
            *ysize = 1;
            break;
        default:    /* FRAMESIZE_THIN */
            *xsize = 1;
            *ysize = 1;
            break;
    }
}

/**************************************************************************************************/

/* Background pen of a system image in the given state */
UWORD getbgpen(ULONG state, UWORD *pens)
{
    UWORD bg;
    switch (state)
    {
    case IDS_NORMAL:
    case IDS_SELECTED:
            bg = pens[FILLPEN];
            break;

    default:
            bg = pens[BACKGROUNDPEN];
            break;
    }

    return bg;
}

/**************************************************************************************************/

/* Draw the 3D frame of a system image, frameh x framev pixels thick */
void drawimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                    WORD left, WORD top, WORD width, WORD height,
                    UWORD frameh, UWORD framev, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    WORD right = left + width - 1;
    WORD bottom = top + height - 1;
    BOOL leftedgegodown = FALSE;
    BOOL topedgegoright = FALSE;

    switch(which)
    {
#if 0
    case CLOSEIMAGE:
            /* draw separator line at the right side */
            SetAPen(rp, pens[SHINEPEN]);
            RectFill(rp, right, top, right, bottom - 1);
            SetAPen(rp, pens[SHADOWPEN]);
            WritePixel(rp, right, bottom);

            right--;
            break;

    case ZOOMIMAGE:
    case DEPTHIMAGE:
    case SDEPTHIMAGE:
            /* draw separator line at the left side */
            SetAPen(rp, pens[SHINEPEN]);
            WritePixel(rp, left, top);
            SetAPen(rp, pens[SHADOWPEN]);
            RectFill(rp, left, top + 1, left, bottom);

            left++;
            break;
#endif

    case UPIMAGE:
    case DOWNIMAGE:
            leftedgegodown = TRUE;
            break;

    case LEFTIMAGE:
    case RIGHTIMAGE:
            topedgegoright = TRUE;
            break;
    }

    if (left == 0) leftedgegodown = TRUE;
    if (top == 0) topedgegoright = TRUE;

    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHADOWPEN : SHINEPEN]);

    /* left edge */
    RectFill(rp, left,
             top,
             left + (frameh - 1),
             bottom - (leftedgegodown ? 0 : framev));

    /* top edge */
    RectFill(rp, left + 1,
             top,
             right - (topedgegoright ? 0 : frameh),
             top + (framev - 1));

    SetAPen(rp, pens[((state == IDS_SELECTED) || (state == IDS_INACTIVESELECTED)) ? SHINEPEN : SHADOWPEN]);

    /* right edge */
    RectFill(rp, right - (frameh - 1),
             top + (topedgegoright ? frameh : 0),
             right,
             bottom);

    /* bottom edge */
    RectFill(rp, left + (leftedgegodown ? framev : 0),
             bottom - (framev - 1),
             right - 1,
             bottom);
}

/**************************************************************************************************/

void draw_thick_line(Class *cl, struct RastPort *rport,
                     LONG x1, LONG y1, LONG x2, LONG y2,
                     UWORD thickness)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    Move(rport, x1, y1);
    Draw(rport, x2, y2);
    /* Georg Steger */
    Move(rport, x1 + 1, y1);
    Draw(rport, x2 + 1, y2);
}

/**************************************************************************************************/

/* The same, in the frame size of the user's preferences */
void renderimageframe(struct RastPort *rp, ULONG which, ULONG state, UWORD *pens,
                      WORD left, WORD top, WORD width, WORD height,
                      struct IntuitionBase *IntuitionBase)
{
    UWORD frameh;
    UWORD framev;

    getintframesizes(IntuitionBase, &frameh, &framev);
    drawimageframe(rp, which, state, pens, left, top, width, height, frameh, framev, IntuitionBase);
}
