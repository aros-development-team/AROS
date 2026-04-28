/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction led.image - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <images/led.h>
#include <utility/tagitem.h>

#include <string.h>

#include "led_intern.h"

#define LEDBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

/*
 * 7-segment display layout:
 *
 *   --0--
 *  |     |
 *  5     1
 *  |     |
 *   --6--
 *  |     |
 *  4     2
 *  |     |
 *   --3--
 *
 * Segment bitmask for digits 0-9:
 * Bit:  0=top, 1=top-right, 2=bottom-right, 3=bottom,
 *       4=bottom-left, 5=top-left, 6=middle
 */

static const UBYTE led_segments[10] =
{
    0x3F,   /* 0: segments 0,1,2,3,4,5    = 0011 1111 */
    0x06,   /* 1: segments 1,2            = 0000 0110 */
    0x5B,   /* 2: segments 0,1,3,4,6      = 0101 1011 */
    0x4F,   /* 3: segments 0,1,2,3,6      = 0100 1111 */
    0x66,   /* 4: segments 1,2,5,6        = 0110 0110 */
    0x6D,   /* 5: segments 0,2,3,5,6      = 0110 1101 */
    0x7D,   /* 6: segments 0,2,3,4,5,6    = 0111 1101 */
    0x07,   /* 7: segments 0,1,2          = 0000 0111 */
    0x7F,   /* 8: segments 0,1,2,3,4,5,6  = 0111 1111 */
    0x6F,   /* 9: segments 0,1,2,3,5,6    = 0110 1111 */
};

/******************************************************************************/

static void led_set(Class *cl, Object *o, struct opSet *msg)
{
    struct LEDData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LED_Pairs:
                data->ld_Pairs = (UWORD)tag->ti_Data;
                break;
            case LED_Time:
                data->ld_Time = (BOOL)tag->ti_Data;
                break;
            case LED_Values:
                data->ld_Values = (UBYTE *)tag->ti_Data;
                break;
            case LED_EditMode:
                data->ld_EditMode = (BOOL)tag->ti_Data;
                break;
            case LED_Pen:
                data->ld_Pen = (UWORD)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

static void led_draw_segment(struct RastPort *rp,
    WORD x, WORD y, WORD digitW, WORD digitH, UBYTE segments, WORD thick)
{
    WORD halfH = digitH / 2;

    /* Segment 0: top horizontal */
    if (segments & (1 << 0))
        RectFill(rp, x + thick, y, x + digitW - thick - 1, y + thick - 1);

    /* Segment 1: top-right vertical */
    if (segments & (1 << 1))
        RectFill(rp, x + digitW - thick, y + thick, x + digitW - 1, y + halfH - 1);

    /* Segment 2: bottom-right vertical */
    if (segments & (1 << 2))
        RectFill(rp, x + digitW - thick, y + halfH + 1, x + digitW - 1, y + digitH - thick - 1);

    /* Segment 3: bottom horizontal */
    if (segments & (1 << 3))
        RectFill(rp, x + thick, y + digitH - thick, x + digitW - thick - 1, y + digitH - 1);

    /* Segment 4: bottom-left vertical */
    if (segments & (1 << 4))
        RectFill(rp, x, y + halfH + 1, x + thick - 1, y + digitH - thick - 1);

    /* Segment 5: top-left vertical */
    if (segments & (1 << 5))
        RectFill(rp, x, y + thick, x + thick - 1, y + halfH - 1);

    /* Segment 6: middle horizontal */
    if (segments & (1 << 6))
        RectFill(rp, x + thick, y + halfH - thick / 2, x + digitW - thick - 1, y + halfH + thick / 2);
}

static void led_draw_colon(struct RastPort *rp,
    WORD x, WORD y, WORD colonW, WORD digitH, WORD thick)
{
    WORD dotSize = thick;
    WORD cx = x + colonW / 2 - dotSize / 2;
    WORD quarterH = digitH / 4;

    /* Upper dot */
    RectFill(rp, cx, y + quarterH, cx + dotSize - 1, y + quarterH + dotSize - 1);

    /* Lower dot */
    RectFill(rp, cx, y + digitH - quarterH - dotSize, cx + dotSize - 1, y + digitH - quarterH - 1);
}

/******************************************************************************/

IPTR LED__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct LEDData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct LEDData));
        data->ld_Pairs = 1;

        led_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR LED__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR LED__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    led_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR LED__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct LEDData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case LED_Pairs:
            *msg->opg_Storage = data->ld_Pairs;
            return TRUE;

        case LED_Time:
            *msg->opg_Storage = data->ld_Time;
            return TRUE;

        case LED_Values:
            *msg->opg_Storage = (IPTR)data->ld_Values;
            return TRUE;

        case LED_EditMode:
            *msg->opg_Storage = data->ld_EditMode;
            return TRUE;

        case LED_Pen:
            *msg->opg_Storage = data->ld_Pen;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR LED__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct LEDData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    UWORD *pens;
    WORD x, y, w, h;
    UWORD ledPen, bgPen;
    UWORD numDigits;
    UWORD numColons;
    WORD digitW, digitH, colonW, gap, thick;
    WORD curX;
    UWORD i;

    if (!rp || !dri)
        return FALSE;

    pens = dri->dri_Pens;
    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    if (w < 4 || h < 6)
        return TRUE;

    /* Select pens */
    ledPen = data->ld_Pen ? data->ld_Pen : pens[TEXTPEN];
    bgPen = pens[BACKGROUNDPEN];

    /* Clear background */
    SetAPen(rp, bgPen);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    if (!data->ld_Values || data->ld_Pairs == 0)
        return TRUE;

    /* Calculate digit layout */
    numDigits = data->ld_Pairs * 2;
    numColons = data->ld_Time ? (data->ld_Pairs - 1) : 0;
    gap = 2;
    colonW = gap * 3;

    /* Calculate digit dimensions to fit available space */
    {
        WORD totalGap = gap * (numDigits - 1) + colonW * numColons;
        digitW = (w - totalGap) / numDigits;
        if (digitW < 4)
            digitW = 4;
    }
    digitH = h - 2;
    if (digitH < 6)
        digitH = 6;

    thick = digitW > 8 ? 2 : 1;

    SetAPen(rp, ledPen);
    SetDrMd(rp, JAM1);

    curX = x + (w - (digitW * numDigits + gap * (numDigits - 1) + colonW * numColons)) / 2;

    for (i = 0; i < numDigits; i++)
    {
        UBYTE value = data->ld_Values[i];
        UBYTE segs;

        if (value > 9)
            value = 0;

        segs = led_segments[value];

        led_draw_segment(rp, curX, y + 1, digitW, digitH, segs, thick);

        curX += digitW + gap;

        /* Draw colon after every pair (except the last) in time mode */
        if (data->ld_Time && (i & 1) && i < numDigits - 1)
        {
            led_draw_colon(rp, curX, y + 1, colonW, digitH, thick);
            curX += colonW;
        }
    }

    /* Draw disabled ghosting pattern */
    if (msg->imp_State == IDS_DISABLED || msg->imp_State == IDS_INACTIVEDISABLED)
    {
        UWORD ghostPat[] = { 0x2222, 0x8888 };

        SetAPen(rp, pens[BLOCKPEN]);
        SetAfPt(rp, ghostPat, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    /* Draw edit mode cursor indicator */
    if (data->ld_EditMode)
    {
        SetAPen(rp, ledPen);
        Move(rp, x, y + h - 1);
        Draw(rp, x + w - 1, y + h - 1);
    }

    return TRUE;
}
