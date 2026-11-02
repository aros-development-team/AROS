/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "gauge_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

/****** Gauge.mui/MUIC_Gauge ************
*
*   NAME
*       MUIC_Gauge
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display a filled progress bar with optional formatted text. The default
*       is vertical, with Current zero, Max 100 and Divide 1. This is an Area
*       subclass, not a Numeric subclass. No automatic progress source, timer or
*       binding to another object is installed.
*       Current, Max and Divide are stored as ULONG despite the public header's
*       LONG annotations. Supply nonnegative representable values; negative
*       inputs are not rejected. Construction first reads all tags, then divides
*       Current if Divide is nonzero and clamps it to Max. Every subsequent
*       OM_SET repeats division and clamping, even if no Gauge attribute
*       changes. The stored value is therefore not a retained raw progress
*       count.
*
*   METHODS
*       Setup calls Area and measures the current formatted string once using
*       TextLength and the current font. AskMinMax uses those cached dimensions
*       and orientation, allowing expansion along the bar. With no InfoText it
*       permits expansion in both directions. Later value or text changes do not
*       recompute the cached dimensions or request relayout.
*       Draw calls Area, paints the horizontal fill from the left or vertical
*       fill from the bottom, and centers formatted text through the Zune text
*       engine. It does not test drawing flags locally. Temporary text-engine
*       objects are destroyed after drawing; allocation failure skips text. Draw
*       and AskMinMax return zero. Setup returns zero on superclass failure,
*       otherwise one.
*
*   NOTES
*       Dispose frees InfoText only when DupInfoText was enabled at
*       construction. Otherwise the format string remains borrowed. Getters
*       return stored values or a borrowed format pointer; there is no getter
*       for Horiz. Setter return values come from Area and do not report
*       string-copy failure.
*       Use a nonzero Max for vertical drawing. The vertical path divides by Max
*       without a zero check; the horizontal path uses zero fill when Max is
*       zero. Fill-size multiplication is not widened to 64 bits and can
*       overflow. Keep dimensions and values within that arithmetic range. These
*       implementation limits are not corrected by this class.
*
*/
/****** Gauge.mui/MUIA_Gauge_Current ************
*
*   NAME
*       MUIA_Gauge_Current
*
*   FUNCTION
*       Set or retrieve the stored progress value.
*
*   INPUTS
*       Initialize, set and get. Header type LONG; actual storage ULONG. Default
*       zero.
*
*   NOTES
*       At construction the supplied value is divided by Divide when nonzero,
*       then limited to Max. On set, comparison uses the incoming tag value
*       against the already transformed stored value. An equal tag becomes
*       TAG_IGNORE; a different value is stored and requests text reformatting
*       and full redraw. Division and upper clamping then run after the entire
*       tag list, including on equal or unrelated set calls.
*       Get returns the transformed value. The forwarded Current tag retains its
*       raw argument, so a notification can receive a value different from Get.
*       Repeated division or clamping does not synthesize a new Current
*       notification. For example, Current 80 with Divide 2 constructs a stored
*       value of 40; a later unrelated set changes it to 20 without a local
*       redraw request.
*
*/
/****** Gauge.mui/MUIA_Gauge_Divide ************
*
*   NAME
*       MUIA_Gauge_Divide
*
*   FUNCTION
*       Set or retrieve the divisor applied to Current.
*
*   INPUTS
*       Initialize, set and get. Header type LONG; actual storage ULONG. Default
*       1. Zero disables division.
*
*   NOTES
*       The divisor applies after all tags have been processed, not immediately
*       when encountered. A changed divisor requests reformatting and redraw; an
*       unchanged tag is ignored for superclass notification processing.
*       Regardless of whether it changes, every set call divides the existing
*       Current again when the divisor is nonzero. Changing Divide does not
*       recover an earlier raw Current. Use 1 to avoid repeated scaling of
*       stored values.
*
*/
/****** Gauge.mui/MUIA_Gauge_Max ************
*
*   NAME
*       MUIA_Gauge_Max
*
*   FUNCTION
*       Set or retrieve the upper bound and fill denominator.
*
*   INPUTS
*       Initialize, set and get. Header type LONG; actual storage ULONG. Default
*       100.
*
*   NOTES
*       Current is upper-clamped after construction and every set call. Changing
*       Max requests full redraw but does not itself reformat InfoText; text can
*       therefore show an old value after clamping. An unchanged Max tag becomes
*       TAG_IGNORE. No Current notification is generated for the clamp.
*       Vertical drawing requires Max greater than zero; it has no
*       zero-denominator guard. Horizontal drawing handles zero by selecting a
*       zero fill width. Max does not change size requirements.
*
*/
/****** Gauge.mui/MUIA_Gauge_Horiz ************
*
*   NAME
*       MUIA_Gauge_Horiz
*
*   FUNCTION
*       Select the bar orientation during construction.
*
*   INPUTS
*       Initialize only, BOOL. Default FALSE for vertical; TRUE selects
*       horizontal.
*
*   NOTES
*       The constructor stores the value. This class has no setter or getter for
*       Horiz; later tags do not change its orientation. Horizontal fill grows
*       from the left, vertical fill from the bottom. Orientation also
*       determines which size dimension can expand when text is present.
*
*/
/****** Gauge.mui/MUIA_Gauge_InfoText ************
*
*   NAME
*       MUIA_Gauge_InfoText
*
*   FUNCTION
*       Provide the format string used to display Current.
*
*   INPUTS
*       Initialize, set and get, STRPTR. Default NULL. Supply a readable
*       NUL-terminated printf format compatible with the single ULONG Current
*       argument, such as "%lu%%", or literal text with no conversions.
*
*   NOTES
*       Formatting uses snprintf into an internal 256-byte buffer, including the
*       terminator. Get returns the format pointer, not the rendered buffer.
*       Construction permits NULL for no text. DupInfoText decides whether the
*       format is borrowed or copied. With borrowing, keep it valid until
*       replacement or disposal; do not modify it in place to request an update.
*       Setting compares string contents. Equal text becomes TAG_IGNORE and
*       retains the old pointer, even if a different pointer was supplied.
*       Replacing an existing non-NULL string with NULL reaches strcmp with NULL
*       and is unsafe. An empty string can display no characters but still
*       counts as a present InfoText for sizing.
*       A changed string is formatted with the post-division Current and
*       requests redraw, but no size remeasurement. Copy failure leaves InfoText
*       NULL without reporting setter failure. In copy mode the old string is
*       freed before duplication; a replacement must not point inside that owned
*       allocation. Current and Divide changes also reformat; Max alone does
*       not.
*
*/
/****** Gauge.mui/MUIA_Gauge_DupInfoText ************
*
*   NAME
*       MUIA_Gauge_DupInfoText
*
*   FUNCTION
*       Choose whether Gauge owns a copy of InfoText.
*
*   INPUTS
*       Initialize, BOOL, default FALSE. AROS also implements get although the
*       header marks this attribute initialize-only. No local setter is
*       implemented.
*
*   NOTES
*       TRUE duplicates the final constructor InfoText and changed replacement
*       strings with StrDup. The class frees its old copy on replacement and its
*       surviving copy on disposal. NULL input or allocation failure leaves no
*       stored string; constructor allocation failure does not fail object
*       creation. FALSE borrows the string and never frees it.
*       The flag is read after all constructor tags, so its order relative to
*       InfoText does not alter copying. Get returns the stored flag. A pointer
*       returned by InfoText remains class-owned in copy mode and must not be
*       freed or modified by the caller.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Construct Area, apply final tags/defaults, divide and clamp Current,
 * optionally duplicate InfoText and format into owned inline buffer. Return
 * object or zero for superclass failure; copy failure does not fail
 * creation. Borrowed strings must remain live; no notifications are sent
 * locally.
 */
IPTR Gauge__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Gauge_DATA *data;
    struct TagItem *tag;
    struct TagItem *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->divide = 1;
    data->max = 100;
    data->horiz = FALSE;
    data->current = 0;
    data->dupinfo = FALSE;
    data->info = NULL;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Gauge_Current:
            data->current = tag->ti_Data;
            break;
        case MUIA_Gauge_Divide:
            data->divide = tag->ti_Data;
            break;
        case MUIA_Gauge_Horiz:
            data->horiz = tag->ti_Data;
            break;
        case MUIA_Gauge_InfoText:
            data->info = (STRPTR) tag->ti_Data;
            break;
        case MUIA_Gauge_Max:
            data->max = tag->ti_Data;
            break;
        case MUIA_Gauge_DupInfoText:
            data->dupinfo = tag->ti_Data;
            break;
        }
    }
    if (data->divide != 0)
        data->current /= data->divide;
    if (data->current > data->max)
        data->current = data->max;
    if (data->dupinfo)
        data->info = StrDup(data->info);

    if (data->info)
    {
        snprintf(data->buf, GAUGE_BUFSIZE, data->info, data->current);
    }
    else
        data->buf[0] = 0;

/*      D(bug("Gauge_New(0x%p, %d)\n",obj,muiAreaData(obj)->mad_Frame)); */

    return (IPTR) obj;
}

/*
 * Free the stored format only in copy mode, then return Area disposal
 * result. Requires no active drawing or notification use. Borrowed strings
 * are not freed; inline buffer belongs to instance.
 */
IPTR Gauge__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Gauge_DATA *data = INST_DATA(cl, obj);
    if (data->dupinfo && data->info)
        FreeVec(data->info);
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Process writable tags, suppress equal supported tags, then divide/clamp
 * Current on every call. Reformat and redraw only when corresponding flags
 * were set; Max alone does not reformat. Return Area setter result. NULL
 * replacement of non-NULL InfoText is unsafe; see public ownership and
 * notification contracts.
 */
IPTR Gauge__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Gauge_DATA *data;
    struct TagItem *tag;
    struct TagItem *tags;
    int info_changed = 0;
    int need_redraw = 0;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Gauge_Current:
            if (data->current != tag->ti_Data)
            {
                data->current = tag->ti_Data;
                info_changed = 1;
                need_redraw = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Gauge_Divide:
            if (data->divide != tag->ti_Data)
            {
                data->divide = tag->ti_Data;
                info_changed = 1;
                need_redraw = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Gauge_InfoText:
            if (!data->info || strcmp(data->info, (STRPTR) tag->ti_Data))
            {
                if (data->dupinfo)
                {
                    if (data->info)
                        FreeVec(data->info);
                    data->info = StrDup((STRPTR) tag->ti_Data);
                }
                else
                {
                    data->info = (STRPTR) tag->ti_Data;
                }
                need_redraw = info_changed = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;

        case MUIA_Gauge_Max:
            if (data->max != tag->ti_Data)
            {
                data->max = tag->ti_Data;
                need_redraw = 1;
            }
            else
            {
                tag->ti_Tag = TAG_IGNORE;
            }
            break;
        }
    }

    if (data->divide != 0)
        data->current /= data->divide;
    if (data->current > data->max)
        data->current = data->max;

    if (info_changed)
    {
        if (data->info)
        {
            snprintf(data->buf, GAUGE_BUFSIZE, data->info, data->current);
        }
        else
            data->buf[0] = 0;
    }

    if (need_redraw)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Write Current, Divide, Max, InfoText or DupInfoText to valid caller-owned
 * IPTR storage and return TRUE; delegate other attributes, including Horiz,
 * to Area. String pointer is borrowed, not transferred; no allocation or
 * state change.
 */
IPTR Gauge__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Gauge_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;
    ULONG tag = msg->opg_AttrID;

    switch (tag)
    {
    case MUIA_Gauge_Current:
        *store = data->current;
        break;

    case MUIA_Gauge_Divide:
        *store = data->divide;
        break;

    case MUIA_Gauge_InfoText:
        *store = (IPTR) data->info;
        break;

    case MUIA_Gauge_Max:
        *store = data->max;
        break;

    case MUIA_Gauge_DupInfoText:
        *store = (IPTR) data->dupinfo;
        break;

    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
}

/*
 * Call Area setup; on success cache current formatted text width/font
 * height or zeros without text. Return zero on superclass failure,
 * otherwise one. Requires valid font and live format when present; no
 * persistent allocation or later automatic remeasurement.
 */
IPTR Gauge__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Gauge_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    if (data->info)
    {
        struct RastPort rp;

        InitRastPort(&rp);
        SetFont(&rp, _font(obj));

        data->info_width = TextLength(&rp, data->buf, strlen(data->buf));
        data->info_height = _font(obj)->tf_YSize;
    }
    else
    {
        if (data->horiz)
        {
            data->info_width = 0;
            data->info_height = 0;
        }
        else
        {
            data->info_width = 0;
            data->info_height = 0;
        }
    }

    return 1;
}

/*
 * Call Area then adjust writable size fields for orientation and cached
 * text dimensions, with expansion along the bar or both axes without text.
 * Return zero ignoring superclass result. Requires successful Setup
 * measurements; does not format or measure.
 */
IPTR Gauge__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct Gauge_DATA *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->horiz)
    {
        msg->MinMaxInfo->MinWidth += data->info_width;
        msg->MinMaxInfo->MinHeight += data->info_height + 2;
        msg->MinMaxInfo->DefWidth += data->info_width + 10;
        msg->MinMaxInfo->DefHeight += data->info_height + 2;
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        if (data->info)
            msg->MinMaxInfo->MaxHeight += data->info_height + 2;
        else
            msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    }
    else
    {
        msg->MinMaxInfo->MinWidth += data->info_width + 2;
        msg->MinMaxInfo->MinHeight += data->info_height;
        msg->MinMaxInfo->DefWidth += data->info_width + 2;
        msg->MinMaxInfo->DefHeight += data->info_height + 10;
        if (data->info)
            msg->MinMaxInfo->MaxWidth += data->info_width + 2;
        else
            msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    return 0;
}

/*
 * Call Area and paint oriented fill plus temporary centered Zune text; free
 * each temporary text object and skip failed allocation. Return zero
 * without checking draw flags locally. Requires valid render context and
 * nonzero Max vertically; multiplication may overflow. Does not reformat
 * cached text.
 */
IPTR Gauge__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Gauge_DATA *data = INST_DATA(cl, obj);
    //ULONG val;

    DoSuperMethodA(cl, obj, (Msg) msg);

/*      D(bug("Gauge_Draw(0x%p) %ldx%ldx%ldx%ld :: %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj), _mleft(obj),_mtop(obj), _mright(obj), _mbottom(obj))); */

    if (data->horiz)
    {
        ULONG w;
        if (data->max != 0)
        {
            /* NOTE: should be 64 bit */
            w = _mwidth(obj) * data->current / data->max;
        }
        else
        {
            w = 0;
        }
        SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_FILL], 0, JAM1);
        RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mleft(obj) + w - 1,
            _mbottom(obj));

        if (data->info)
        {
            ZText *ztext =
                zune_text_new("\33c\0338", data->buf, ZTEXT_ARG_NONE, 0);
            if (ztext)
            {
                zune_text_get_bounds(ztext, obj);
                SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
                zune_text_draw(ztext, obj, _mleft(obj), _mright(obj),
                    _mtop(obj) + (_mheight(obj) - ztext->height) / 2);
                zune_text_destroy(ztext);
            }
        }
    }
    else
    {
        ULONG h;

        /* NOTE: should be 64 bit */
        h = _mheight(obj) * data->current / data->max;

        SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_FILL], 0, JAM1);
        RectFill(_rp(obj), _mleft(obj), _mbottom(obj) - h + 1, _mright(obj),
            _mbottom(obj));

        if (data->info)
        {
            ZText *ztext =
                zune_text_new("\33c\0338", data->buf, ZTEXT_ARG_NONE, 0);
            if (ztext)
            {
                zune_text_get_bounds(ztext, obj);
                SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
                zune_text_draw(ztext, obj, _mleft(obj), _mright(obj),
                    _mtop(obj) + (_mheight(obj) - ztext->height) / 2);
                zune_text_destroy(ztext);
            }
        }
    }
    return 0;
}


#if ZUNE_BUILTIN_GAUGE
/*
 * When built in, dispatch local methods and return their results; delegate
 * other messages to Area. Requires ABI-correct message and live object
 * except construction. Ownership and lifecycle follow the class contract.
 */
BOOPSI_DISPATCHER(IPTR, Gauge_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Gauge__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Gauge__OM_DISPOSE(cl, obj, (Msg) msg);
    case OM_SET:
        return Gauge__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Gauge__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Gauge__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_AskMinMax:
        return Gauge__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Gauge__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Gauge_desc =
{
    MUIC_Gauge,
    MUIC_Area,
    sizeof(struct Gauge_DATA),
    (void *) Gauge_Dispatcher
};
#endif /* ZUNE_BUILTIN_GAUGE */
