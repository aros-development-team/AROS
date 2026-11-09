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
#include <proto/utility.h>
#include <proto/alib.h>
#include <stddef.h>

#include "intuition_intern.h"
#include "gadgets.h"
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

/**************************************************************************************************/

#define HSPACING 3
#define VSPACING 3
/* Ralph Schmidt
 * heuristics for smaller arrows used in apps
 * like filer
 */
#define HSPACING_MIDDLE 2
#define VSPACING_MIDDLE 2
#define HSPACING_SMALL 1
#define VSPACING_SMALL 1

/* Distance between the edges of an arrow image and the arrow drawn into it */
void getarrowspacing(LONG width, LONG height, UWORD *hspacing, UWORD *vspacing)
{
    *hspacing = HSPACING;
    *vspacing = VSPACING;

    if (width <= 12)
        *hspacing = HSPACING_MIDDLE;

    if (width <= 10)
        *hspacing = HSPACING_SMALL;

    if (height <= 12)
        *vspacing = VSPACING_MIDDLE;

    if (height <= 10)
        *vspacing = VSPACING_SMALL;
}

/**************************************************************************************************/

/* The filled arrow of LEFTIMAGE/UPIMAGE/RIGHTIMAGE/DOWNIMAGE, drawn into the
 * box left/top/width/height. LEFT/RIGHT and UP/DOWN are mirror images. */
void drawarrowimage(struct RastPort *rp, ULONG which, UWORD pen,
                    LONG left, LONG top, LONG width, LONG height,
                    struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    BOOL horiz = (which == LEFTIMAGE) || (which == RIGHTIMAGE);
    BOOL flip = (which == RIGHTIMAGE) || (which == DOWNIMAGE);
    LONG right = left + width - 1;
    LONG bottom = top + height - 1;
    LONG len;
    WORD i, c;

    /* c lines are drawn across the arrow, len is its length */
    if (horiz)
    {
        c = (height + 1) / 2;
        len = width;
    }
    else
    {
        c = (width + 1) / 2;
        len = height;
    }

    SetAPen(rp, pen);

    for(i = 0; i < c; i++)
    {
        LONG tip = (c - i - 1) * len / c;       /* inset on the tip side  */
        LONG base = i * len / c / 2;            /* inset on the base side */

        if (flip)
        {
            LONG t = tip; tip = base; base = t;
        }

        if (horiz)
        {
            RectFill(rp, left + tip, top + i, right - base, top + i);
            RectFill(rp, left + tip, bottom - i, right - base, bottom - i);
        }
        else
        {
            RectFill(rp, left + i, top + tip, left + i, bottom - base);
            RectFill(rp, right - i, top + tip, right - i, bottom - base);
        }
    }
}

/**************************************************************************************************/

/* The two overlapping windows of a depth image, drawn into the box
 * left/top/width/height. In the image of a screen depth gadget the front
 * window is never filled and the back one always uses the shine pen. */
void drawdepthwindows(struct RastPort *rp, UWORD *pens, ULONG state, BOOL screenimage,
                      LONG left, LONG top, LONG width, LONG height,
                      struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    LONG   right, bottom;
    WORD   h_spacing;
    WORD   v_spacing;

    h_spacing = width / 6;
    v_spacing = height / 6;

    left += h_spacing;
    top  += v_spacing;

    width  -= h_spacing * 2;
    height -= v_spacing * 2;

    right  = left + width  - 1;
    bottom = top  + height - 1;

    /* Render top left window  */

    SetAPen(rp, pens[SHADOWPEN]);
    drawrect(rp, left, top, right - (width / 3 ), bottom - (height / 3), IntuitionBase);

    /* Fill top left window (inside of the frame above) */

    if (!screenimage && (state != IDS_INACTIVENORMAL))
    {
        SetAPen(rp, pens[BACKGROUNDPEN]);
        RectFill(rp, left + 1, top + 1,
             right - (width / 3) - 1, bottom - (height / 3) - 1);
    }

    /* Render bottom right window  */
    SetAPen(rp, pens[SHADOWPEN]);
    drawrect(rp, left + (width / 3), top + (height / 3),
             right, bottom, IntuitionBase);

    /* Fill bottom right window (inside of the frame above) */
    SetAPen(rp, pens[(!screenimage && (state == IDS_INACTIVENORMAL)) ? BACKGROUNDPEN : SHINEPEN]);
    RectFill(rp, left + (width / 3) + 1, top + (height / 3) + 1,
             right - 1, bottom - 1);

    if (state == IDS_SELECTED)
    {
        /* Re-Render top left window  */

        SetAPen(rp, pens[SHADOWPEN]);
        drawrect(rp, left, top,
             right - (width / 3 ), bottom - (height / 3), IntuitionBase);
    }
}

/**************************************************************************************************/

/*
 * The window, screen and menu decoration classes have the same instance data
 * and the same two attributes under their own names, so their OM_NEW and
 * OM_GET methods only differ in the attribute IDs.
 */
_Static_assert(sizeof(struct scrdecor_data) == sizeof(struct windecor_data) &&
               offsetof(struct scrdecor_data, userbuffersize) ==
               offsetof(struct windecor_data, userbuffersize),
               "scrdecor_data does not match windecor_data");
_Static_assert(sizeof(struct menudecor_data) == sizeof(struct windecor_data) &&
               offsetof(struct menudecor_data, userbuffersize) ==
               offsetof(struct windecor_data, userbuffersize),
               "menudecor_data does not match windecor_data");

IPTR decor_om_new(Class *cl, Object *obj, struct opSet *msg, Tag userbuffertag)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct windecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        data->userbuffersize = (ULONG) GetTagData(userbuffertag, 0, msg->ops_AttrList);
    }

    return (IPTR)obj;
}

IPTR decor_om_get(Class *cl, Object *obj, struct opGet *msg, Tag userbuffertag, Tag truecoloronlytag)
{
    struct windecor_data *data = INST_DATA(cl, obj);

    if (msg->opg_AttrID == userbuffertag)
    {
        *msg->opg_Storage = (IPTR) data->userbuffersize;
    }
    else if (msg->opg_AttrID == truecoloronlytag)
    {
        *msg->opg_Storage = FALSE;
    }
    else
    {
        return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    return 1;
}
