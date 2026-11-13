/*
    Copyright (C) 1999, David Le Corfec.
    Copyright (C) 2002-2026, The AROS Development Team.
    All rights reserved.

*/

#include <string.h>

#include <clib/alib_protos.h>
#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

//#define MYDEBUG 1
#include "debug.h"

#include "support.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "prefs.h"
#include "imspec.h"

/****** Slider.mui/MUIC_Slider ************
*
*   NAME
*       MUIC_Slider
*
*   SUPERCLASS
*       Numeric.mui
*
*   DESCRIPTION
*       Display Numeric Value with a custom drawn knob and optional text.
*       Default orientation is horizontal; Quiet defaults FALSE. Construction
*       prepends SliderBack, Knob font and Slider frame before caller tags. This
*       class is a Numeric subclass and does not own a Prop child or native
*       proportional gadget.
*       Setup calls Numeric, prepares a knob background image and calculates
*       knob dimensions, then registers a mouse handler. Cleanup releases the
*       background and removes the handler before Numeric Cleanup. Show shows
*       the prepared image and returns one regardless of superclass result; Hide
*       hides it and invalidates the cached position. Dispose frees owned copied
*       display text, then Numeric. Complete Hide/Cleanup first.
*
*   METHODS
*       Knob sizing calls Stringify for every value from Min to Max. If the
*       computed span exceeds MUI_MAXMAX it instead scans zero through
*       MUI_MAXMAX, which can miss wide strings for the real range.
*       CheckAllSizes is ignored by this scan. Signed span/loop arithmetic is
*       unchecked; a maximum at LONG_MAX can overflow the loop increment.
*       Draw maps Value to pixel offset and caches both offset and a copied
*       Stringify result. Text-copy allocation is unchecked before strlen. Setup
*       does not turn knob-background preparation failure into FALSE. Mouse
*       handling requires geometry established by drawing and a nonnegative
*       travel length.
*       A non-shift click on the knob starts dragging and sets Pressed. A
*       background click changes Value by one with Reverse honored. A later
*       non-SELECTDOWN button event ends a pressed drag. Mouse moves map offset
*       through ScaleToValue on a zero-based scale and use the Numeric setter
*       directly to preserve pixel offset. Notifications remain enabled even in
*       Quiet mode.
*
*   NOTES
*       Slider HandleEvent handles mouse events only and does not delegate to
*       Numeric. Numeric Setup separately installs its raw-key handler with the
*       Numeric class. Keep normal event registration intact; direct calls to
*       Slider HandleEvent are not a substitute for that handler.
*       Value/Min/Max/Format setters invalidate cached position/text, but only
*       orientation setting explicitly recalculates knob dimensions after Setup.
*       Thus live format/bound changes can leave stale measured knob size
*       despite Numeric requesting parent relayout. Reverse alone does not
*       invalidate the position cache. Quiet hides text only; it does not reduce
*       the knob-size scan or suppress notifications.
*
*/
/****** Slider.mui/MUIA_Slider_Horiz ************
*
*   NAME
*       MUIA_Slider_Horiz
*
*   FUNCTION
*       Select horizontal orientation when true; vertical otherwise. Default
*       TRUE.
*
*   INPUTS
*       BOOL; initialize, set and get.
*
*   NOTES
*       Getter normalizes to TRUE/FALSE. Runtime setting immediately
*       recalculates knob dimensions using font and application preferences,
*       invalidates caches and requests a full redraw. Perform it with valid
*       setup/connection state; it does not itself request parent relayout.
*       Construction stores the flag without that runtime calculation.
*
*/
/****** Slider.mui/MUIA_Slider_Quiet ************
*
*   NAME
*       MUIA_Slider_Quiet
*
*   FUNCTION
*       Suppress drawing the text inside the knob. Default FALSE.
*
*   INPUTS
*       BOOL; initialize only. No local getter or runtime setter.
*
*   NOTES
*       Value changes and notifications continue normally. Knob dimensions are
*       still calculated by formatting the selected value range. Quiet does not
*       mean release-only updates or notification suppression.
*
*/
/****** Slider.mui/MUIA_Slider_Level ************
*
*   NAME
*       MUIA_Slider_Level
*
*   FUNCTION
*       Obsolete name with the same attribute ID as MUIA_Numeric_Value.
*
*   INPUTS
*       Available in the header only when MUI_OBSOLETE is enabled. Same type and
*       access as Numeric_Value.
*
*   NOTES
*       Use MUIA_Numeric_Value in new code. There is no separate Slider state or
*       conversion for this alias; inherited Numeric behavior and limitations
*       apply.
*
*/
/****** Slider.mui/MUIA_Slider_Min ************
*
*   NAME
*       MUIA_Slider_Min
*
*   FUNCTION
*       Obsolete name with the same attribute ID as MUIA_Numeric_Min.
*
*   INPUTS
*       Available in the header only when MUI_OBSOLETE is enabled. Same type and
*       access as Numeric_Min.
*
*   NOTES
*       Use MUIA_Numeric_Min in new code. There is no separate Slider state or
*       conversion for this alias; inherited Numeric behavior and limitations
*       apply.
*
*/
/****** Slider.mui/MUIA_Slider_Max ************
*
*   NAME
*       MUIA_Slider_Max
*
*   FUNCTION
*       Obsolete name with the same attribute ID as MUIA_Numeric_Max.
*
*   INPUTS
*       Available in the header only when MUI_OBSOLETE is enabled. Same type and
*       access as Numeric_Max.
*
*   NOTES
*       Use MUIA_Numeric_Max in new code. There is no separate Slider state or
*       conversion for this alias; inherited Numeric behavior and limitations
*       apply.
*
*/
/****** Slider.mui/MUIA_Slider_Reverse ************
*
*   NAME
*       MUIA_Slider_Reverse
*
*   FUNCTION
*       Obsolete name with the same attribute ID as MUIA_Numeric_Reverse.
*
*   INPUTS
*       Available in the header only when MUI_OBSOLETE is enabled. Same type and
*       access as Numeric_Reverse.
*
*   NOTES
*       Use MUIA_Numeric_Reverse in new code. There is no separate Slider state
*       or conversion for this alias; inherited Numeric behavior and limitations
*       apply.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_SliderData
{
    ULONG flags;
    struct MUI_EventHandlerNode ehn;
    struct MUI_ImageSpec_intern *knob_bg;
    LONG knob_offset;
    LONG scale_length;
    LONG knob_length;
    LONG knob_left;
    LONG knob_top;
    LONG knob_width;
    LONG knob_height;
    LONG knob_click;
    UWORD max_text_width;
    CONST_STRPTR text_buffer;
    UWORD text_width;
    UWORD text_length;
};


enum slider_flags
{
    SLIDER_HORIZ = (1 << 0),
    SLIDER_QUIET = (1 << 1),
    SLIDER_VALIDOFFSET = (1 << 2),
    SLIDER_VALIDSTRING = (1 << 3),
};

#define longget(obj,attr,var)             \
    do                                          \
    {                                                \
            IPTR _iptr_var = *(var);    \
        get(obj,attr,&_iptr_var);   \
        *var = (LONG)_iptr_var;     \
    } while(0)


/*
 * Measure Stringify results across the selected scan range and derive knob
 * size from font/frame preferences. Requires setup resources and valid non-
 * NULL strings. Span above MUI_MAXMAX substitutes 0..MUI_MAXMAX;
 * CheckAllSizes is ignored. Mutates geometry, no return status. LONG_MAX
 * loop increment and span subtraction can overflow; UWORD text width can
 * truncate.
 */
static void CalcKnobDimensions(struct IClass *cl, Object *obj)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *knob_frame;
    LONG min = 0;
    LONG max = 0;
    LONG val;
    LONG width;
    struct RastPort rp;

    knob_frame =
        zune_zframe_get(obj,
        &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob]);

    InitRastPort(&rp);
    SetFont(&rp, _font(obj));

    width = 0;

    longget(obj, MUIA_Numeric_Min, &min);
    longget(obj, MUIA_Numeric_Max, &max);

    if ((max - min) > MUI_MAXMAX)
    {
        min = 0;
        max = MUI_MAXMAX;
    }

    /* Determine the width of the knob */
    for (val = min; val <= max; val++)
    {
        LONG nw;
        char *buf;

        buf = (char *)DoMethod(obj, MUIM_Numeric_Stringify, val);
        nw = TextLength(&rp, buf, strlen(buf));
        if (nw > width)
            width = nw;
    }
    data->max_text_width = width;
    data->knob_width = width +
        knob_frame->ileft +
        knob_frame->iright +
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].innerLeft +
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].innerRight;

    data->knob_height = _font(obj)->tf_YSize +
        knob_frame->itop +
        knob_frame->ibottom +
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].innerTop +
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].innerBottom;

    if (data->flags & SLIDER_HORIZ)
        data->knob_length = data->knob_width;
    else
        data->knob_length = data->knob_height;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Parse orientation/Quiet, prepend visual defaults and construct Numeric.
 * Store flags and mouse-handler descriptor; return object or zero on
 * superclass failure. No knob image or text copy allocated yet.
 */
IPTR Slider__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_SliderData *data;
    struct TagItem *tags, *tag;
    ULONG flags = SLIDER_HORIZ;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Slider_Horiz:
            _handle_bool_tag(flags, tag->ti_Data, SLIDER_HORIZ);
            break;
        case MUIA_Slider_Quiet:
            _handle_bool_tag(flags, tag->ti_Data, SLIDER_QUIET);
            break;
        }
    }

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        MUIA_Background, MUII_SliderBack,
        MUIA_Font,       MUIV_Font_Knob,
        MUIA_Frame,      MUIV_Frame_Slider,
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj)
    {
        return 0;
    }

    data = INST_DATA(cl, obj);
    data->flags = flags;

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Free owned copied text if present and return Numeric disposal result.
 * Requires prior Hide/Cleanup of knob image and event handlers; no Prop
 * child exists.
 */
IPTR Slider__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    if (data->text_buffer) FreeVec((APTR)data->text_buffer);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Recalculate dimensions immediately for Horiz, invalidate caches and
 * redraw. Value/bounds/Format invalidate caches only, then delegate to
 * Numeric and return its result. Requires live font/preferences for Horiz;
 * bounds/format alone do not rerun measurement. Reverse is not a cache
 * invalidator.
 */
IPTR Slider__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Slider_Horiz:
            _handle_bool_tag(data->flags, tag->ti_Data, SLIDER_HORIZ);
            CalcKnobDimensions(cl, obj);
            data->flags &= ~(SLIDER_VALIDOFFSET | SLIDER_VALIDSTRING);
            MUI_Redraw(obj, MADF_DRAWOBJECT);
            break;
        case MUIA_Numeric_Value:
        case MUIA_Numeric_Min:
        case MUIA_Numeric_Max:
        case MUIA_Numeric_Format:
            /* reset the knob position and string */
            data->flags &= ~(SLIDER_VALIDOFFSET | SLIDER_VALIDSTRING);
            break;
        }
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Return normalized Horiz through IPTR storage with TRUE; otherwise
 * delegate to Numeric. Quiet has no local getter. No ownership transfer.
 */
IPTR Slider__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Slider_Horiz:
        *store = ((data->flags & SLIDER_HORIZ) != 0);
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Set up Numeric, prepare owned knob background, measure knob and register
 * mouse handler. Return FALSE only on superclass failure, otherwise TRUE
 * without validating image/handler success. Requires valid formatter and
 * connected resources; pair Cleanup.
 */
IPTR Slider__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    data->knob_bg = zune_imspec_setup(MUII_SliderKnob, muiRenderInfo(obj));

    CalcKnobDimensions(cl, obj);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Clean prepared knob image when present and remove mouse handler before
 * Numeric Cleanup; return superclass result. Requires matched setup. Cached
 * text remains until replacement/disposal.
 */
IPTR Slider__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (data->knob_bg)
    {
        zune_imspec_cleanup(data->knob_bg);
        data->knob_bg = NULL;
    }
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
/*
 * Ask superclass then add cached knob constraints according to orientation;
 * set travel-axis maximum to MUI_MAXMAX. Requires prior knob measurement
 * and writable MinMaxInfo. Return TRUE ignoring superclass result; does not
 * remeasure text.
 */
IPTR Slider__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    LONG min = 0, max = 0;

    DoSuperMethodA(cl, obj, (Msg) msg);

    longget(obj, MUIA_Numeric_Min, &min);
    longget(obj, MUIA_Numeric_Max, &max);

    if (data->flags & SLIDER_HORIZ)
    {
        msg->MinMaxInfo->MinWidth += data->knob_width + 1;
        msg->MinMaxInfo->MinHeight += data->knob_height;
        msg->MinMaxInfo->DefWidth += data->knob_width * 4 + 2;
        msg->MinMaxInfo->DefHeight += data->knob_height;
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        msg->MinMaxInfo->MaxHeight += data->knob_height;
    }
    else
    {
        msg->MinMaxInfo->MinWidth += data->knob_width;
        msg->MinMaxInfo->MinHeight += data->knob_height + 1;
        msg->MinMaxInfo->DefWidth += data->knob_width;
        msg->MinMaxInfo->DefHeight += data->knob_height * 4 + 2;
        msg->MinMaxInfo->MaxWidth += data->knob_width;
        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }

    return TRUE;
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
/*
 * Call Numeric/Area Show, show knob background if present, return one
 * ignoring superclass result. Requires completed Setup and live render
 * state.
 */
IPTR Slider__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);
    if (data->knob_bg)
        zune_imspec_show(data->knob_bg, obj);
    return 1;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
/*
 * Hide knob background if present, invalidate offset cache and return
 * superclass Hide result. Does not free image or text; Cleanup still
 * required.
 */
IPTR Slider__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (data->knob_bg)
        zune_imspec_hide(data->knob_bg);

    /* This may look ugly when window is resized but it is easier than
     * recalculating the knob offset in the Show method */
    data->flags &= ~SLIDER_VALIDOFFSET;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/**************************************************************************
 MUIM_Draw
**************************************************************************/
/*
 * Call superclass, return FALSE without full/update flags; otherwise
 * compute travel, cached offset, knob/frame and optional text. Own a StrDup
 * of Stringify output; allocation result is unchecked before strlen. Return
 * TRUE ignoring render statuses. Requires valid images/frame, setup and
 * nonnegative travel length.
 */
IPTR Slider__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *knob_frame;
    UWORD knob_frame_state;
    LONG val = 0;

    D(bug("[slider] %s: obj @ 0x%p\n", __func__, obj);)

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    D(bug("[slider] %s: %d,%d->%d,%d (%d,%d)\n", __func__, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), _mwidth(obj), _mheight(obj));)

    if (data->flags & SLIDER_HORIZ)
        data->scale_length = _mwidth(obj);
    else
        data->scale_length = _mheight(obj);
    data->scale_length -= data->knob_length;

    /* Update knob position if not cached */
    if (!(data->flags & SLIDER_VALIDOFFSET))
    {
        data->knob_offset =
            DoMethod(obj, MUIM_Numeric_ValueToScale, 0,
                data->scale_length);
        data->flags |= SLIDER_VALIDOFFSET;
    }

    data->knob_top = _mtop(obj);
    data->knob_left = _mleft(obj);
    if (data->flags & SLIDER_HORIZ)
        data->knob_left += data->knob_offset;
    else
        data->knob_top += data->knob_offset;

    DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
        _mwidth(obj), _mheight(obj), 0, 0, 0);

    zune_imspec_draw(data->knob_bg, muiRenderInfo(obj),
        data->knob_left, data->knob_top, data->knob_width,
        data->knob_height, 0, 0, 0);

    knob_frame_state =
        muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].state;
    if (XGET(obj, MUIA_Pressed))
        knob_frame_state ^= 1;
    knob_frame = zune_zframe_get_with_state(obj,
        &muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob],
        knob_frame_state);
    knob_frame->draw(knob_frame->customframe, muiRenderInfo(obj),
        data->knob_left, data->knob_top, data->knob_width,
        data->knob_height, data->knob_left, data->knob_top,
        data->knob_width, data->knob_height);

    if (!(data->flags & SLIDER_QUIET))
    {
        SetFont(_rp(obj), _font(obj));
        SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_TEXT],
            _pens(obj)[MPEN_BACKGROUND], JAM1);
        if (!(data->flags & SLIDER_VALIDSTRING))
        {
            longget(obj, MUIA_Numeric_Value, &val);
            if (data->text_buffer) FreeVec((APTR)data->text_buffer);
            data->text_buffer = StrDup((CONST_STRPTR) DoMethod(obj,
                MUIM_Numeric_Stringify, val));
            data->text_length = strlen(data->text_buffer);
            data->text_width =
                TextLength(_rp(obj), data->text_buffer, data->text_length);
            data->flags |= SLIDER_VALIDSTRING;
        }

        Move(_rp(obj),
            data->knob_left + knob_frame->ileft +
            muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].innerLeft +
            (data->max_text_width - data->text_width) / 2,
            data->knob_top + _font(obj)->tf_Baseline + knob_frame->itop +
            muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Knob].innerTop);
        Text(_rp(obj), data->text_buffer, data->text_length);
    }

    return TRUE;
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
/*
 * Handle knob drag and background clicks, suppress shift-click selection,
 * update Pressed and Numeric Value, return Eat when handled. Mouse moves
 * rely on registration rather than testing Pressed locally. NULL imsg
 * returns zero. No superclass delegation; Numeric has its own key handler.
 * Requires previously drawn hit geometry and valid travel; drag bypasses
 * Slider setter to retain offset.
 */
IPTR Slider__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    IPTR result = 0;
    BOOL increase;

    if (!msg->imsg)
        return 0;
    switch (msg->imsg->Class)
    {
    case IDCMP_MOUSEBUTTONS:
        if (msg->imsg->Code == SELECTDOWN)
        {
            if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY)
                && (msg->imsg->Qualifier
                & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) == 0)
            {
                if (data->flags & SLIDER_HORIZ)
                {
                    data->knob_click =
                        msg->imsg->MouseX - data->knob_offset - _mleft(obj);
                }
                else
                {
                    data->knob_click =
                        msg->imsg->MouseY - data->knob_offset - _mtop(obj);
                }

                if (_between(data->knob_left, msg->imsg->MouseX,
                        data->knob_left + data->knob_width)
                    && _between(data->knob_top, msg->imsg->MouseY,
                        data->knob_top + data->knob_height))
                {
                    /* Clicked on knob */
                    DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                    DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);
                    set(obj, MUIA_Pressed, TRUE);
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
                else
                {
                    /* Clicked on background */
                    if (data->flags & SLIDER_HORIZ)
                        increase = msg->imsg->MouseX > data->knob_left;
                    else
                        increase = msg->imsg->MouseY > data->knob_top;

                    if (XGET(obj, MUIA_Numeric_Reverse))
                        increase = !increase;
                    
                    DoMethod(obj, increase ?
                        MUIM_Numeric_Increase : MUIM_Numeric_Decrease, 1);
                }
                result = MUI_EventHandlerRC_Eat;
            }
        }
        else
        {
            if (XGET(obj, MUIA_Pressed))
            {
                DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                    (IPTR) & data->ehn);
                data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                    (IPTR) & data->ehn);
                set(obj, MUIA_Pressed, FALSE);
                MUI_Redraw(obj, MADF_DRAWUPDATE);
                result = MUI_EventHandlerRC_Eat;
            }
        }
        break;

    case IDCMP_MOUSEMOVE:
        {
            IPTR oldval = 0;
            LONG newval;
            LONG old_offset = data->knob_offset;

            if (data->flags & SLIDER_HORIZ)
                data->knob_offset =
                    msg->imsg->MouseX - data->knob_click - _mleft(obj);
            else
                data->knob_offset =
                    msg->imsg->MouseY - data->knob_click - _mtop(obj);

            /* Ensure knob offset is within range */
            if (data->knob_offset < 0)
                data->knob_offset = 0;
            else if (data->knob_offset > data->scale_length)
                data->knob_offset = data->scale_length;

            newval = DoMethod(obj, MUIM_Numeric_ScaleToValue,
                0, data->scale_length, data->knob_offset);

            if (data->knob_offset != old_offset)
            {
                get(obj, MUIA_Numeric_Value, &oldval);
                if ((LONG) oldval != newval)
                {
                    /* Bypass our own set method so that knob position is not
                     * reset */
                    struct TagItem superSetTags[] =
                    {
                        { MUIA_Numeric_Value,   newval  },
                        { TAG_DONE,             0       }
                    };
                    struct opSet superSet =
                    {
                        .MethodID = OM_SET,
                        .ops_AttrList = superSetTags,
                        .ops_GInfo = NULL
                    };
                    data->flags &= ~SLIDER_VALIDSTRING;
                    DoSuperMethodA(cl, obj, (Msg)&superSet);
                }
                else
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                result = MUI_EventHandlerRC_Eat;
            }
        }
        break;
    }

    return result;
}

/*
 * Route Slider access/lifecycle/draw/mouse overrides and delegate other
 * methods to Numeric. Return handler result. Requires valid object and
 * message; no native Prop gadget ownership.
 */
BOOPSI_DISPATCHER(IPTR, Slider_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Slider__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Slider__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Slider__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Slider__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Slider__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Slider__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Slider__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Slider__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_AskMinMax:
        return Slider__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Slider__MUIM_Draw(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Slider__MUIM_HandleEvent(cl, obj, (APTR) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Slider_desc =
{
    MUIC_Slider,
    MUIC_Numeric,
    sizeof(struct MUI_SliderData),
    (void *) Slider_Dispatcher
};
