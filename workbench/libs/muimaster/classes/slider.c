/*
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2013, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>

#include <clib/alib_protos.h>
#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

/*#define MYDEBUG 1*/
#include "debug.h"

#include "support.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "prefs.h"
#include "imspec.h"

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
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    return (IPTR) obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
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
IPTR Slider__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *knob_frame;
    UWORD knob_frame_state;
    LONG val = 0;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

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

    if (data->flags & SLIDER_HORIZ)
    {
        data->knob_top = _mtop(obj);
        data->knob_left = _mleft(obj) + data->knob_offset;
    }
    else
    {
        data->knob_top = _mtop(obj) + data->knob_offset;
        data->knob_left = _mleft(obj);
    }

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
            data->text_buffer = (CONST_STRPTR) DoMethod(obj,
                MUIM_Numeric_Stringify, val);
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
IPTR Slider__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
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
                    data->flags &= ~SLIDER_VALIDSTRING;
                    IPTR tag_list[] = {MUIA_Numeric_Value, newval, TAG_END};
                    DoSuperMethod(cl, obj, OM_SET, tag_list, NULL);
                }
                else
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
            }
        }
        break;
    }

    return 0;
}

BOOPSI_DISPATCHER(IPTR, Slider_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Slider__OM_NEW(cl, obj, (struct opSet *)msg);
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
