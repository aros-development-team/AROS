/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "prefs.h"
#include "debug.h"
#include "levelmeter_private.h"

/****** Levelmeter.mui/MUIC_Levelmeter ************
*
*   NAME
*       MUIC_Levelmeter
*
*   SUPERCLASS
*       Numeric.mui
*
*   DESCRIPTION
*       Show Numeric Value as a needle on a semicircular scale. ValueToScale
*       maps the current value to zero through 180 degrees, honoring
*       Numeric_Reverse. No periodic sampling, peak hold, decay or automatic
*       source binding is implemented; application code sets Value.
*       Construction prepends FillArea FALSE and initializes a private pen
*       sentinel. Setup calls Numeric and obtains a background pen from the
*       screen ColorMap. Cleanup releases a successfully obtained pen before
*       Numeric Cleanup. Complete this lifecycle before disposal; no local
*       disposer substitutes for Cleanup.
*
*   METHODS
*       AskMinMax adds 53 pixels in width and 46 pixels in height to all
*       superclass size fields and returns TRUE. Full Draw paints frame,
*       background, scale marks and an empty label field. Update Draw erases the
*       old needle with the background pen before drawing the new one. Draw
*       returns FALSE without full/update flags, otherwise TRUE and ignores
*       superclass/drawing results.
*       Setup returns Numeric's result even if ObtainBestPen fails. Draw uses
*       the stored pen without checking the -1 failure sentinel. The class has
*       no mouse handler of its own and retains Numeric's raw-key handler; it is
*       not automatically a read-only object merely because it looks like a
*       meter.
*
*   NOTES
*       The Label attribute has no implementation. Numeric_Format/Stringify are
*       inherited but not used to draw a numeric label. Scale artwork uses a
*       nominal 39 by 20 pixel area and current frame geometry. Use ordered
*       representable Numeric bounds and a live screen/render context. No
*       separate bitmap or native gadget is owned.
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_Label ************
*
*   NAME
*       MUIA_Levelmeter_Label
*
*   FUNCTION
*       Declared label attribute without an implementation in this AROS class.
*
*   INPUTS
*       String pointer; header declares initialize, set and get, but no local
*       handler implements those operations.
*
*   NOTES
*       The constructor does not store the string, no getter returns it, and
*       Draw paints only the empty label background. No string ownership is
*       acquired. Supplying a label does not display text or change sizing;
*       inherited Numeric_Format also does not populate this field.
*
*/

extern struct Library *MUIMasterBase;

#define OUTERFRAME_X 2
#define OUTERFRAME_Y 2
#define OUTERFRAME_W (OUTERFRAME_X * 2)
#define OUTERFRAME_H (OUTERFRAME_Y * 2)

#define INNERFRAME_X 2
#define INNERFRAME_Y 2
#define INNERFRAME_W (INNERFRAME_X * 2)
#define INNERFRAME_H (INNERFRAME_Y * 2)

#define BORDERSIZE_X 3
#define BORDERSIZE_Y 2
#define BORDERSIZE_W (BORDERSIZE_X * 2)
#define BORDERSIZE_H (BORDERSIZE_Y * 2)

#define LEVEL_LABEL_SPACING 2

#define LEVEL_WIDTH 39
#define LEVEL_HEIGHT 20

#define LABEL_HEIGHT 8

#define TOTAL_WIDTH OUTERFRAME_W + BORDERSIZE_W + INNERFRAME_W + LEVEL_WIDTH
#define TOTAL_HEIGHT OUTERFRAME_H + BORDERSIZE_H + INNERFRAME_H * 2 \
+ LEVEL_HEIGHT + LEVEL_LABEL_SPACING + LABEL_HEIGHT

/*
 * Construct Numeric with FillArea FALSE and set background-pen sentinel to
 * -1. Return superclass object or NULL. Pen acquisition is deferred to
 * Setup.
 */
IPTR Levelmeter__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        MUIA_FillArea, FALSE, TAG_MORE, (IPTR) msg->ops_AttrList);

    if (obj)
    {
        struct Levelmeter_DATA *data = INST_DATA(cl, obj);

        data->levelbgpen = -1;
    }

    return (IPTR) obj;
}

/*
 * Call Numeric Setup, then obtain a background pen from screen ColorMap on
 * success. Retain pen for Cleanup and return superclass result even if
 * acquisition fails. Requires live screen and no unmatched prior Setup.
 */
IPTR Levelmeter__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
    //struct RastPort          rp;
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
#if 0
        InitRastPort(&rp);
        SetFont(&rp, _font(obj));

        DeinitRastPort(&rp);
#endif
        data->levelbgpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
            0x4b4b4b4b,
            0x39393939,
            0x93939393,
            OBP_FailIfBad, FALSE, OBP_Precision, PRECISION_GUI, TAG_DONE);
    }

    return retval;
}

/*
 * Release stored background pen when not -1, reset sentinel and return
 * Numeric Cleanup result. Requires same live screen/ColorMap as Setup. No
 * native gadget or image ownership.
 */
IPTR Levelmeter__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);

    if (data->levelbgpen != -1)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->levelbgpen);
        data->levelbgpen = -1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
/*
 * Ask Numeric/Area then add fixed 53 by 46 content dimensions to all
 * corresponding size fields. Return TRUE ignoring superclass result.
 * Requires writable MinMaxInfo; Label does not affect size.
 */
IPTR Levelmeter__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    //struct Levelmeter_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += TOTAL_WIDTH;
    msg->MinMaxInfo->MinHeight += TOTAL_HEIGHT;
    msg->MinMaxInfo->DefWidth += TOTAL_WIDTH;
    msg->MinMaxInfo->DefHeight += TOTAL_HEIGHT;
    msg->MinMaxInfo->MaxWidth += TOTAL_WIDTH;
    msg->MinMaxInfo->MaxHeight += TOTAL_HEIGHT;

    return TRUE;
}

/*
 * Paint semicircular scale marks every 15 degrees, with extra pixels at
 * multiples of 45. Requires valid RastPort, pen and adequate rectangle.
 * Mutates drawing state, allocates nothing and returns no status.
 */
static void DrawScale(struct RastPort *rp, LONG x1, LONG y1, LONG x2,
    LONG y2, LONG pen)
{
    LONG cx = (x1 + x2 + 1) / 2;
    LONG cy = y2 - 1;
    LONG rx = cx - x1 - 1;
    LONG ry = cy - y1 - 1;
    LONG a, b, g;

    SetABPenDrMd(rp, pen, 0, JAM1);

    for (g = 0; g <= 180; g += 15)
    {
        double angle = ((double)g) * 3.14159265358979323846 / 180.0;

        a = cx + (LONG) (cos(angle) * rx);
        b = cy - (LONG) (sin(angle) * ry);

        WritePixel(rp, a, b);

        if ((g % 45) == 0)
        {
            static WORD offtable[][2] = {
                {-1, 0},
                {-1, 1},
                {0, 1},
                {1, 1},
                {1, 0}
            };

            WritePixel(rp, a + offtable[g / 45][0],
                b + offtable[g / 45][1]);
        }

    }

}

/*
 * Draw a line from the meter pivot using supplied pen. Angles outside
 * 0..180 become zero before reversing drawing direction. Requires valid
 * RastPort, pen and adequate rectangle; no resource ownership or result.
 */
static void DrawNeedle(struct RastPort *rp, LONG x1, LONG y1, LONG x2,
    LONG y2, double angle, LONG pen)
{
    LONG cx = (x1 + x2 + 1) / 2;
    LONG cy = y2 - 1;
    LONG rx = cx - x1 - 4;
    LONG ry = cy - y1 - 4;
    LONG a, b;

    SetABPenDrMd(rp, pen, 0, JAM1);
    Move(rp, cx, cy);

    if ((angle < 0.0) | (angle > 180.0))
        angle = 0.0;
    angle = 180.0 - angle;

    a = cx + (LONG) (cos(angle * 3.14159265358979323846 / 180.0) * rx);
    b = cy - (LONG) (sin(angle * 3.14159265358979323846 / 180.0) * ry);

    Draw(rp, a, b);

}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
/*
 * Paint full frame/scale/empty label background or erase old needle, then
 * map Value to 0..180 and draw new needle. Return FALSE without full/update
 * flags, otherwise TRUE. Requires live render state and valid background
 * pen; does not check acquisition failure. Store prevangle; no text
 * formatting or periodic update source.
 */
IPTR Levelmeter__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
    struct RastPort *rp;
    WORD x1, y1, x2, y2;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    x1 = _mleft(obj);
    y1 = _mtop(obj);
    x2 = _mright(obj);
    y2 = _mbottom(obj);

    rp = _rp(obj);

    if (msg->flags & MADF_DRAWOBJECT)
    {
        /* Transparent edges */

        DoMethod(obj, MUIM_DrawParentBackground, x1, y1, 2, 1, x1, y1, 0);
        DoMethod(obj, MUIM_DrawParentBackground, x1, y1 + 1, 1, 1, x1,
            y1 + 1, 0);

        DoMethod(obj, MUIM_DrawParentBackground, x2 - 1, y1, 2, 1, x2 - 1,
            y1, 0);
        DoMethod(obj, MUIM_DrawParentBackground, x2, y1 + 1, 1, 1, x2,
            y1 + 1, 0);

        DoMethod(obj, MUIM_DrawParentBackground, x1, y2, 2, 1, x1, y2, 0);
        DoMethod(obj, MUIM_DrawParentBackground, x1, y2 - 1, 1, 1, x1,
            y2 - 1, 0);

        DoMethod(obj, MUIM_DrawParentBackground, x2 - 1, y2, 2, 1, x2 - 1,
            y2, 0);
        DoMethod(obj, MUIM_DrawParentBackground, x2, y2 - 1, 1, 1, x2,
            y2 - 1, 0);

        /* Outer frame */

        SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], 0, JAM1);
        Move(rp, x1 + 1, y2 - 1);
        Draw(rp, x1, y2 - 2);
        Draw(rp, x1, y1 + 2);
        Draw(rp, x1 + 2, y1);
        Draw(rp, x2 - 2, y1);
        Draw(rp, x2 - 1, y1 + 1);

        SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
        Move(rp, x2, y1 + 2);
        Draw(rp, x2, y2 - 2);
        Draw(rp, x2 - 2, y2);
        Draw(rp, x1 + 2, y2);

        SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
        Move(rp, x1 + 1, y2 - 2);
        Draw(rp, x1 + 1, y1 + 2);
        Draw(rp, x1 + 2, y1 + 1);
        Draw(rp, x2 - 2, y1 + 1);

        SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
        Move(rp, x2 - 1, y1 + 2);
        Draw(rp, x2 - 1, y2 - 2);
        Draw(rp, x2 - 2, y2 - 1);
        Draw(rp, x1 + 2, y2 - 1);

        /* Border */

        x1 += OUTERFRAME_X;
        x2 -= OUTERFRAME_X;
        y1 += OUTERFRAME_X;
        y2 -= OUTERFRAME_Y;

        SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
        RectFill(rp, x1, y1, x2, y1 + BORDERSIZE_Y - 1);
        RectFill(rp, x1, y1 + BORDERSIZE_Y, x1 + BORDERSIZE_X - 1, y2);
        RectFill(rp, x1 + BORDERSIZE_X - 1, y2 - BORDERSIZE_Y + 1, x2, y2);
        RectFill(rp, x2 - BORDERSIZE_X + 1, y1 + BORDERSIZE_Y, x2,
            y2 - BORDERSIZE_Y);

        /* Inner Frame */

        x1 += BORDERSIZE_X;
        x2 -= BORDERSIZE_X;
        y1 += BORDERSIZE_Y;
        y2 = y1 + LEVEL_HEIGHT + INNERFRAME_H - 1;

        Move(rp, x1, y1);
        Draw(rp, x1 + 2, y1);
        Draw(rp, x1, y1 + 2);
        Draw(rp, x1, y1 + 1);

        Move(rp, x2, y1);
        Draw(rp, x2 - 2, y1);
        Draw(rp, x2, y1 + 2);
        Draw(rp, x2, y1 + 1);

        Move(rp, x1, y2);
        Draw(rp, x1 + 2, y2);
        Draw(rp, x1, y2 - 2);
        Draw(rp, x1, y2 - 1);

        Move(rp, x2, y2);
        Draw(rp, x2 - 2, y2);
        Draw(rp, x2, y2 - 2);
        Draw(rp, x2, y2 - 1);

        SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
        Move(rp, x1 + 2, y2 - 1);
        Draw(rp, x1, y2 - 3);
        Draw(rp, x1, y1 + 3);
        Draw(rp, x1 + 3, y1);
        Draw(rp, x2 - 3, y1);
        Draw(rp, x2 - 1, y1 + 2);

        SetAPen(rp, _pens(obj)[MPEN_SHINE]);
        Move(rp, x2, y1 + 3);
        Draw(rp, x2, y2 - 3);
        Draw(rp, x2 - 3, y2);
        Draw(rp, x1 + 3, y2);

        SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
        Move(rp, x1 + 3, y1 + 1);
        Draw(rp, x2 - 3, y1 + 1);
        Move(rp, x1 + 1, y1 + 3);
        Draw(rp, x1 + 1, y2 - 3);
        Move(rp, x1 + 3, y2 - 1);
        Draw(rp, x2 - 3, y2 - 1);
        Move(rp, x2 - 1, y1 + 3), Draw(rp, x2 - 1, y2 - 3);

        /* Levelmeter bg */

        x1 += INNERFRAME_X;
        x2 -= INNERFRAME_X;
        y1 += INNERFRAME_Y;
        y2 -= INNERFRAME_Y;

        SetAPen(rp, data->levelbgpen);
        RectFill(rp, x1 + 1, y1, x2 - 1, y1);
        RectFill(rp, x1, y1 + 1, x2, y2 - 1);
        RectFill(rp, x1 + 1, y2, x2 - 1, y2);

        SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
        WritePixel(rp, x1, y1);
        WritePixel(rp, x2, y1);
        WritePixel(rp, x1, y2);
        WritePixel(rp, x2, y2);

        /* Levelmeter scale */

        DrawScale(rp, x1, y1, x2, y2, _pens(obj)[MPEN_SHINE]);

        /* Level-Label spacing */

        x1 -= INNERFRAME_X;
        x2 += INNERFRAME_X;
        y1 = y2 + INNERFRAME_Y + 1;

        SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
        RectFill(rp, x1, y1, x2, y1 + LEVEL_LABEL_SPACING - 1);

        /* Label Frame */

        y1 += LEVEL_LABEL_SPACING;
        y2 = _mbottom(obj) - OUTERFRAME_Y - BORDERSIZE_Y;

        SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
        Move(rp, x1, y1);
        Draw(rp, x1 + 1, y1);
        Draw(rp, x1, y1 + 1);
        Move(rp, x2, y1);
        Draw(rp, x2 - 1, y1);
        Draw(rp, x2, y1 + 1);
        Move(rp, x1, y2);
        Draw(rp, x1 + 1, y2);
        Draw(rp, x1, y2 - 1);
        Move(rp, x2, y2);
        Draw(rp, x2 - 1, y2);
        Draw(rp, x2, y2 - 1);

        SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
        Move(rp, x1 + 1, y2 - 1);
        Draw(rp, x1, y2 - 2);
        Draw(rp, x1, y1 + 2);
        Draw(rp, x1 + 2, y1);
        Draw(rp, x2 - 2, y1);
        Draw(rp, x2 - 1, y1 + 1);

        SetAPen(rp, _pens(obj)[MPEN_SHINE]);
        Move(rp, x2, y1 + 2);
        Draw(rp, x2, y2 - 2);
        Draw(rp, x2 - 2, y2);
        Draw(rp, x1 + 2, y2);

        SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
        RectFill(rp, x1 + 1, y1 + 2, x1 + 1, y2 - 2);
        RectFill(rp, x2 - 1, y1 + 2, x2 - 1, y2 - 2);
        RectFill(rp, x1 + 2, y1 + 1, x2 - 2, y1 + 1);
        RectFill(rp, x1 + 2, y2 - 1, x2 - 2, y2 - 1);

        /* Label Bg */

        RectFill(rp, x1 + 2, y1 + 2, x2 - 2, y2 - 2);

    }

    x1 = _mleft(obj) + OUTERFRAME_X + BORDERSIZE_X + INNERFRAME_X;
    x2 = _mright(obj) - OUTERFRAME_X - BORDERSIZE_X - INNERFRAME_X;
    y1 = _mtop(obj) + OUTERFRAME_Y + BORDERSIZE_Y + INNERFRAME_Y;
    y2 = y1 + LEVEL_HEIGHT - 1;

    if (msg->flags & MADF_DRAWUPDATE)
    {
        DrawNeedle(rp, x1, y1, x2, y2, data->prevangle, data->levelbgpen);
    }

    data->prevangle =
        (double)DoMethod(obj, MUIM_Numeric_ValueToScale, 0, 180);

    DrawNeedle(rp, x1, y1, x2, y2, data->prevangle, _pens(obj)[MPEN_SHINE]);

    return TRUE;
}

#if ZUNE_BUILTIN_LEVELMETER
/*
 * When built in, route local overrides and delegate other messages to
 * Numeric. Return the selected handler result. Requires ABI-correct message
 * and live object except construction; resource ownership follows the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Levelmeter_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Levelmeter__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Setup:
        return Levelmeter__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Levelmeter__MUIM_Cleanup(cl, obj,
            (struct MUIP_Cleanup *)msg);
    case MUIM_AskMinMax:
        return Levelmeter__MUIM_AskMinMax(cl, obj,
            (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Levelmeter__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Levelmeter_desc =
{
    MUIC_Levelmeter,
    MUIC_Numeric,
    sizeof(struct Levelmeter_DATA),
    (void *) Levelmeter_Dispatcher
};
#endif /* ZUNE_BUILTIN_LEVELMETER */
