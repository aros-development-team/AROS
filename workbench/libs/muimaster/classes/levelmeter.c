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
*       Display the inherited Numeric value as either a legacy needle meter or
*       a segmented meter. MUIV_Levelmeter_Style_Needle is the default and
*       preserves the historical semicircular display.
*
*       MUIV_Levelmeter_Style_Segments maps the inherited Numeric value to an
*       active segment count with MUIM_Numeric_ValueToScale(). Horizontal
*       meters grow from left to right; vertical meters grow from bottom to
*       top. Critical segments occupy the high end of the scale, warning
*       segments occupy the range immediately below them, and the remaining
*       active segments use the normal color.
*
*       The class does not sample an input source or implement peak hold,
*       decay, FFT processing, channel management, or decibel conversion.
*       Applications update MUIA_Numeric_Value.
*
*   METHODS
*       OM_NEW preserves MUIA_FillArea FALSE, initializes the Needle style,
*       16 segments, horizontal orientation, two warning segments and one
*       critical segment, then applies constructor attributes.
*
*       OM_SET applies Levelmeter attributes and passes the tag list to
*       Numeric. Style, segment-count and orientation changes request parent
*       relayout and a full redraw. Zone changes request an update redraw.
*       OM_GET returns the stored Levelmeter attributes.
*
*       MUIM_Setup obtains the legacy background pen and the three segmented
*       display colors after Numeric setup succeeds. MUIM_Cleanup releases
*       each successfully obtained pen before Numeric cleanup.
*
*       MUIM_AskMinMax preserves the historical fixed Needle geometry. The
*       Segments style derives its primary minimum and default dimension from
*       MUIA_Levelmeter_Segments and permits expansion in both dimensions.
*
*       MUIM_Draw delegates to the segmented renderer when Segments is
*       selected. Needle drawing otherwise follows the historical frame,
*       scale and needle path.
*
*   NOTES
*       Segments defaults to 16 segments, producing 13 normal, two warning
*       and one critical segment. The default active colors match the current
*       AHIRecord level display. If a segmented color pen cannot be obtained,
*       the renderer falls back to Zune theme pens. Inactive segments use the
*       Levelmeter background pen when available and MPEN_SHADOW otherwise.
*
*       MUIA_Numeric_Min, MUIA_Numeric_Max and MUIA_Numeric_Reverse remain
*       authoritative for value mapping. The class retains Numeric's raw-key
*       handling; applications that require a passive display must manage
*       focus and input accordingly.
*
*   BUGS
*       MUIA_Levelmeter_Label is declared but not implemented. The class does
*       not store or draw the supplied string.
*
*       In Needle style, MUIM_Setup still returns Numeric's successful setup
*       result when obtaining the legacy background pen fails. The Needle
*       draw path later uses the stored -1 failure sentinel without a fallback.
*
*   SEE ALSO
*       Numeric.mui, MUIA_Numeric_Value, MUIA_Numeric_Min,
*       MUIA_Numeric_Max, MUIA_Numeric_Reverse, MUIA_Levelmeter_Style,
*       MUIA_Levelmeter_Segments, MUIA_Levelmeter_Horiz
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_Label ************
*
*   NAME
*       MUIA_Levelmeter_Label
*
*   FUNCTION
*       Declare a label string for the Levelmeter interface.
*
*   INPUTS
*       Initialize, set and get; STRPTR.
*
*   BUGS
*       This AROS implementation has no local storage, getter, drawing or size
*       handling for the attribute. Supplying a string has no visible effect
*       and transfers no ownership to the object.
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_Style ************
*
*   NAME
*       MUIA_Levelmeter_Style
*
*   FUNCTION
*       Select the Levelmeter rendering style.
*
*   INPUTS
*       Initialize, set and get; ULONG. The default is
*       MUIV_Levelmeter_Style_Needle. MUIV_Levelmeter_Style_Segments selects
*       the segmented display. Other values are ignored.
*
*   NOTES
*       Changing the style requests parent relayout and a full redraw. The
*       stored Numeric value and Numeric range are not changed.
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_Segments ************
*
*   NAME
*       MUIA_Levelmeter_Segments
*
*   FUNCTION
*       Set or retrieve the segment count used by the Segments style.
*
*   INPUTS
*       Initialize, set and get; ULONG from 1 through
*       MUIV_Levelmeter_Segments_Max. The default is 16. Values outside the
*       supported range are ignored.
*
*   NOTES
*       Changing the count requests parent relayout and a full redraw.
*       CriticalSegments is limited to the new segment count and
*       WarningSegments is then limited to the remaining range.
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_Horiz ************
*
*   NAME
*       MUIA_Levelmeter_Horiz
*
*   FUNCTION
*       Select the orientation used by the Segments style.
*
*   INPUTS
*       Initialize, set and get; BOOL. The default is TRUE. TRUE draws active
*       segments from left to right. FALSE draws them from bottom to top.
*
*   NOTES
*       Changing orientation requests parent relayout and a full redraw. The
*       attribute is stored in Needle style but does not alter Needle drawing.
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_WarningSegments ************
*
*   NAME
*       MUIA_Levelmeter_WarningSegments
*
*   FUNCTION
*       Set or retrieve the size of the warning zone.
*
*   INPUTS
*       Initialize, set and get; ULONG. The default is 2. The stored value is
*       limited to the segment range remaining below the critical zone.
*
*   NOTES
*       The warning zone occupies the highest segments below the critical
*       zone. Changing the value requests an update redraw.
*
*/
/****** Levelmeter.mui/MUIA_Levelmeter_CriticalSegments ************
*
*   NAME
*       MUIA_Levelmeter_CriticalSegments
*
*   FUNCTION
*       Set or retrieve the size of the critical zone.
*
*   INPUTS
*       Initialize, set and get; ULONG. The default is 1. The stored value is
*       limited to the configured segment count.
*
*   NOTES
*       The critical zone always occupies the highest segments. Changing the
*       value also re-limits WarningSegments to the remaining range and
*       requests an update redraw.
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

#define LEVELMETER_CHANGE_REDRAW   0x01
#define LEVELMETER_CHANGE_FULL     0x02
#define LEVELMETER_CHANGE_RELAYOUT 0x04

/*
 * Return the smaller unsigned value. This helper owns no resources and has
 * no side effects.
 */
static ULONG Levelmeter_MinUlong(ULONG a, ULONG b)
{
    return a < b ? a : b;
}

/*
 * Apply Levelmeter attributes with last-value-wins tag semantics. Ignore an
 * unsupported Style or Segments value, then normalize CriticalSegments to
 * Segments and WarningSegments to the remaining range. Store changed values
 * and return LEVELMETER_CHANGE_* flags describing redraw and relayout needs.
 * The caller owns data and the tag list.
 */
static ULONG Levelmeter_ApplyTags(struct Levelmeter_DATA *data,
    struct TagItem *tags)
{
    struct TagItem *state = tags;
    struct TagItem *tag;
    ULONG style = data->style;
    ULONG segments = data->segments;
    ULONG warning = data->warningSegments;
    ULONG critical = data->criticalSegments;
    BOOL horiz = data->horiz;
    ULONG changes = 0;

    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Levelmeter_Style:
            if (tag->ti_Data == MUIV_Levelmeter_Style_Needle ||
                tag->ti_Data == MUIV_Levelmeter_Style_Segments)
                style = (ULONG) tag->ti_Data;
            break;
        case MUIA_Levelmeter_Segments:
            if (tag->ti_Data >= 1 && tag->ti_Data <= MUIV_Levelmeter_Segments_Max)
                segments = (ULONG) tag->ti_Data;
            break;
        case MUIA_Levelmeter_Horiz:
            horiz = (tag->ti_Data != 0);
            break;
        case MUIA_Levelmeter_WarningSegments:
            warning = (ULONG) tag->ti_Data;
            break;
        case MUIA_Levelmeter_CriticalSegments:
            critical = (ULONG) tag->ti_Data;
            break;
        }
    }

    critical = Levelmeter_MinUlong(critical, segments);
    warning = Levelmeter_MinUlong(warning, segments - critical);

    if (data->style != style)
    {
        data->style = style;
        changes |= LEVELMETER_CHANGE_FULL | LEVELMETER_CHANGE_RELAYOUT;
    }
    if (data->segments != segments)
    {
        data->segments = segments;
        changes |= LEVELMETER_CHANGE_FULL | LEVELMETER_CHANGE_RELAYOUT;
    }
    if (data->horiz != horiz)
    {
        data->horiz = horiz;
        changes |= LEVELMETER_CHANGE_FULL | LEVELMETER_CHANGE_RELAYOUT;
    }
    if (data->warningSegments != warning)
    {
        data->warningSegments = warning;
        changes |= LEVELMETER_CHANGE_REDRAW;
    }
    if (data->criticalSegments != critical)
    {
        data->criticalSegments = critical;
        changes |= LEVELMETER_CHANGE_REDRAW;
    }

    return changes;
}

/*
 * Construct Numeric with MUIA_FillArea FALSE. Initialize all pen sentinels,
 * Needle compatibility state and segmented-display defaults, then apply the
 * constructor tag list. Return the superclass object or NULL. Pen acquisition
 * is deferred to MUIM_Setup.
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
        data->normalpen = -1;
        data->warningpen = -1;
        data->criticalpen = -1;
        data->prevangle = 0.0;
        data->style = MUIV_Levelmeter_Style_Needle;
        data->segments = MUIV_Levelmeter_Segments_Default;
        data->warningSegments = 2;
        data->criticalSegments = 1;
        data->horiz = TRUE;
        Levelmeter_ApplyTags(data, msg->ops_AttrList);
    }

    return (IPTR) obj;
}

/*
 * Apply local Levelmeter attributes and pass the same tag list to Numeric.
 * Request parent relayout for style, segment-count or orientation changes;
 * request a full or update redraw according to the resulting change flags.
 * Return Numeric's OM_SET result.
 */
IPTR Levelmeter__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
    ULONG changes = Levelmeter_ApplyTags(data, msg->ops_AttrList);
    IPTR result = DoSuperMethodA(cl, obj, (Msg) msg);

    if ((changes & LEVELMETER_CHANGE_RELAYOUT) && _parent(obj))
    {
        DoMethod(_parent(obj), MUIM_Group_InitChange);
        DoMethod(_parent(obj), MUIM_Group_ExitChange);
    }

    if (changes & LEVELMETER_CHANGE_FULL)
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    else if (changes & LEVELMETER_CHANGE_REDRAW)
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return result;
}

/*
 * Return stored Levelmeter attributes through opg_Storage. Delegate unknown
 * attributes to Numeric. This method does not modify object state.
 */
IPTR Levelmeter__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    const struct Levelmeter_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Levelmeter_Style:
        *msg->opg_Storage = data->style;
        return TRUE;
    case MUIA_Levelmeter_Segments:
        *msg->opg_Storage = data->segments;
        return TRUE;
    case MUIA_Levelmeter_Horiz:
        *msg->opg_Storage = data->horiz;
        return TRUE;
    case MUIA_Levelmeter_WarningSegments:
        *msg->opg_Storage = data->warningSegments;
        return TRUE;
    case MUIA_Levelmeter_CriticalSegments:
        *msg->opg_Storage = data->criticalSegments;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Run Numeric setup and, on success, obtain the legacy background pen plus
 * normal, warning and critical segmented-display pens from the screen ColorMap.
 * Store -1 for each failed acquisition. Return Numeric's setup result; segment
 * drawing has theme-pen fallbacks, while the legacy Needle background path
 * retains its historical missing-fallback bug.
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
        data->normalpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
            0x33333333, 0xdddddddd, 0x33333333,
            OBP_FailIfBad, FALSE, OBP_Precision, PRECISION_GUI, TAG_DONE);
        data->warningpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
            0xeeeeeeee, 0xaaaaaaaa, 0x33333333,
            OBP_FailIfBad, FALSE, OBP_Precision, PRECISION_GUI, TAG_DONE);
        data->criticalpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
            0xeeeeeeee, 0x44444444, 0x44444444,
            OBP_FailIfBad, FALSE, OBP_Precision, PRECISION_GUI, TAG_DONE);
    }

    return retval;
}

/*
 * Release every successfully obtained background or segmented-display pen,
 * reset each pen sentinel to -1, then return Numeric's cleanup result. Requires
 * the same live screen/ColorMap ownership context established by MUIM_Setup.
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
    if (data->normalpen != -1)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->normalpen);
        data->normalpen = -1;
    }
    if (data->warningpen != -1)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->warningpen);
        data->warningpen = -1;
    }
    if (data->criticalpen != -1)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->criticalpen);
        data->criticalpen = -1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
/*
 * Ask Numeric for its size constraints. For Segments, derive the primary
 * minimum and default size from the configured segment count, use a compact
 * cross-axis size and allow expansion to MUI_MAXMAX. For Needle, preserve the
 * historical fixed 53 by 46 contribution. Return TRUE.
 */
IPTR Levelmeter__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    const struct Levelmeter_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->style == MUIV_Levelmeter_Style_Segments)
    {
        ULONG primaryMin = data->segments * 3 - 1;
        ULONG primaryDef = data->segments * 8 - 1;

        if (data->horiz)
        {
            msg->MinMaxInfo->MinWidth += primaryMin;
            msg->MinMaxInfo->MinHeight += 6;
            msg->MinMaxInfo->DefWidth += primaryDef;
            msg->MinMaxInfo->DefHeight += 10;
        }
        else
        {
            msg->MinMaxInfo->MinWidth += 6;
            msg->MinMaxInfo->MinHeight += primaryMin;
            msg->MinMaxInfo->DefWidth += 10;
            msg->MinMaxInfo->DefHeight += primaryDef;
        }
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
        return TRUE;
    }

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
 * Select the active-segment pen for a zero-based segment index. Critical
 * segments occupy the top end, warning segments the range immediately below,
 * and all lower segments are normal. If a requested color pen was not obtained
 * during setup, return the corresponding Zune theme pen.
 */
static LONG Levelmeter_ActivePen(Object *obj,
    const struct Levelmeter_DATA *data, ULONG segment)
{
    ULONG critical = Levelmeter_MinUlong(data->criticalSegments, data->segments);
    ULONG warning = Levelmeter_MinUlong(data->warningSegments,
        data->segments - critical);
    ULONG criticalStart = data->segments - critical;
    ULONG warningStart = criticalStart - warning;

    if (segment >= criticalStart)
        return data->criticalpen != -1 ? data->criticalpen : _pens(obj)[MPEN_SHADOW];
    if (segment >= warningStart)
        return data->warningpen != -1 ? data->warningpen : _pens(obj)[MPEN_SHINE];
    return data->normalpen != -1 ? data->normalpen : _pens(obj)[MPEN_FILL];
}

/*
 * Draw the Segments style for full or update redraws. Map Numeric value to
 * 0..Segments with MUIM_Numeric_ValueToScale(), divide the current object
 * rectangle into horizontal left-to-right or vertical bottom-to-top segments,
 * and fill active segments with zone colors. Inactive segments use the legacy
 * background pen when available and MPEN_SHADOW otherwise. Return FALSE only
 * when neither supported draw flag is present; otherwise return TRUE.
 */
static IPTR Levelmeter_DrawSegments(Object *obj,
    const struct Levelmeter_DATA *data, const struct MUIP_Draw *msg)
{
    ULONG active;
    ULONG segment;
    LONG left, top, width, height;
    LONG lastPen = -1;

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;
    if (data->segments == 0 || _rp(obj) == NULL || _pens(obj) == NULL)
        return TRUE;

    active = (ULONG) DoMethod(obj, MUIM_Numeric_ValueToScale, 0, data->segments);
    left = _mleft(obj);
    top = _mtop(obj);
    width = _mwidth(obj);
    height = _mheight(obj);

    if (width <= 0 || height <= 0)
        return TRUE;

    for (segment = 0; segment < data->segments; segment++)
    {
        LONG x1 = left;
        LONG y1 = top;
        LONG x2 = left + width - 1;
        LONG y2 = top + height - 1;
        LONG pen;

        if (data->horiz)
        {
            x1 = left + (LONG) ((segment * (ULONG) width) / data->segments);
            x2 = left + (LONG) (((segment + 1) * (ULONG) width) / data->segments) - 1;
            if (segment + 1 < data->segments)
                x2--;
        }
        else
        {
            y1 = top + height - (LONG) (((segment + 1) * (ULONG) height) / data->segments);
            y2 = top + height - (LONG) ((segment * (ULONG) height) / data->segments) - 1;
            if (segment + 1 < data->segments)
                y1++;
        }

        if (x2 < x1 || y2 < y1)
            continue;

        if (segment < active)
            pen = Levelmeter_ActivePen(obj, data, segment);
        else if (data->levelbgpen != -1)
            pen = data->levelbgpen;
        else
            pen = _pens(obj)[MPEN_SHADOW];

        if (pen != lastPen)
        {
            SetAPen(_rp(obj), pen);
            lastPen = pen;
        }
        RectFill(_rp(obj), x1, y1, x2, y2);
    }

    return TRUE;
}

/*
 * Run Numeric drawing first. Dispatch Segments style to the segmented renderer.
 * Otherwise preserve the historical Needle path: on a full draw paint frame,
 * background, scale and empty label field; on update erase the previous needle,
 * map Numeric value to 0..180 and draw the new needle. The Needle path retains
 * the historical unchecked background-pen failure sentinel.
 */
IPTR Levelmeter__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
    struct RastPort *rp;
    WORD x1, y1, x2, y2;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->style == MUIV_Levelmeter_Style_Segments)
        return Levelmeter_DrawSegments(obj, data, msg);

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
 * Route OM_NEW, OM_SET, OM_GET, MUIM_Setup, MUIM_Cleanup, MUIM_AskMinMax and
 * MUIM_Draw to Levelmeter handlers when the class is built into muimaster.
 * Delegate all other messages to Numeric. Resource ownership follows the
 * Levelmeter setup/cleanup lifecycle.
 */
BOOPSI_DISPATCHER(IPTR, Levelmeter_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Levelmeter__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Levelmeter__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Levelmeter__OM_GET(cl, obj, (struct opGet *)msg);
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
