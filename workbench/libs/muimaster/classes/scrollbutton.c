/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <intuition/intuitionbase.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/****** Scrollbutton.mui/MUIC_Scrollbutton ************
*
*   NAME
*       MUIC_Scrollbutton
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Provide a drag-position control that reports two coordinates packed into
*       NewPosition. Construction prepends ButtonFrame, RelVerify InputMode and
*       ButtonBack background; later caller tags can override them. The class
*       creates or owns no Prop objects and installs no automatic scrolling
*       connection.
*       Setup first sets up Area and registers its own mouse-button handler.
*       SELECTDOWN inside the object enables mouse-move events, sets Selected
*       TRUE and records the mouse origin. Any later non-SELECTDOWN button event
*       removes mouse-move tracking and clears Selected. While selected, mouse
*       moves add displacement to the stored Horiz/Vert bases, pack the result
*       and issue a NewPosition setter for notifications.
*
*   METHODS
*       Setup returns FALSE on Area failure, otherwise TRUE without checking
*       event registration. Cleanup removes its handler, invokes Area Cleanup
*       and returns zero. HandleEvent returns zero and does not delegate to
*       Area. AskMinMax adds two to each minimum/default and MUI_MAXMAX to each
*       maximum, returning zero; this addition is not a validated maximum clamp.
*       Pair Setup/Cleanup and normal Area Show/Hide before disposal. Use
*       application notifications to translate NewPosition into content or Prop
*       setters. HorizProp and VertProp getters return unassigned instance
*       pointers, normally NULL; they do not identify connected scrollers.
*
*   NOTES
*       Horiz/Vert setters store WORD bases but no local getters exist, contrary
*       to the header. Initialization tags for those bases are not consumed. The
*       drag handler reads bases on every move, so changing them mid-drag
*       affects subsequent positions. There is no clamping to a content range;
*       each packed component wraps as UWORD.
*
*/
/****** Scrollbutton.mui/MUIA_Scrollbutton_NewPosition ************
*
*   NAME
*       MUIA_Scrollbutton_NewPosition
*
*   FUNCTION
*       Return the last packed drag position: horizontal UWORD in bits 31..16
*       and vertical UWORD in bits 15..0.
*
*   INPUTS
*       ULONG; get and notification output.
*
*   NOTES
*       Initial value is zero. Mouse movement while Selected updates storage
*       before issuing the setter that triggers notifications. Decode the two
*       halves explicitly; interpret them as signed WORD only if your
*       application uses signed coordinates. External setting does not update
*       the stored position because the local setter has no case for it. No
*       automatic range clamp or Prop update occurs.
*
*/
/****** Scrollbutton.mui/MUIA_Scrollbutton_Horiz ************
*
*   NAME
*       MUIA_Scrollbutton_Horiz
*
*   FUNCTION
*       Set the horizontal base added to displacement from the drag-start mouse
*       position.
*
*   INPUTS
*       WORD; set only in this implementation. Default zero.
*
*   NOTES
*       Values convert to WORD without validation. Initialization is not
*       consumed and the header-advertised getter is absent. The base is read
*       afresh on each mouse move; it is not automatically advanced after
*       release. Set a new base before a later drag when your application needs
*       cumulative movement.
*
*/
/****** Scrollbutton.mui/MUIA_Scrollbutton_Vert ************
*
*   NAME
*       MUIA_Scrollbutton_Vert
*
*   FUNCTION
*       Set the vertical base added to displacement from the drag-start mouse
*       position.
*
*   INPUTS
*       WORD; set only in this implementation. Default zero.
*
*   NOTES
*       Values convert to WORD without validation. Initialization is not
*       consumed and the header-advertised getter is absent. The base is read
*       afresh on each mouse move; it is not automatically advanced after
*       release. Set a new base before a later drag when your application needs
*       cumulative movement.
*
*/
/****** Scrollbutton.mui/MUIA_Scrollbutton_HorizProp ************
*
*   NAME
*       MUIA_Scrollbutton_HorizProp
*
*   FUNCTION
*       Return the corresponding internal pointer field, which this
*       implementation never assigns.
*
*   INPUTS
*       Object pointer; get only.
*
*   NOTES
*       The normal result is NULL with TRUE getter status. No Prop object is
*       constructed, attached, borrowed or controlled through this field. There
*       is no setter or initialization handling; this declaration is not a
*       working scroller association API.
*
*/
/****** Scrollbutton.mui/MUIA_Scrollbutton_VertProp ************
*
*   NAME
*       MUIA_Scrollbutton_VertProp
*
*   FUNCTION
*       Return the corresponding internal pointer field, which this
*       implementation never assigns.
*
*   INPUTS
*       Object pointer; get only.
*
*   NOTES
*       The normal result is NULL with TRUE getter status. No Prop object is
*       constructed, attached, borrowed or controlled through this field. There
*       is no setter or initialization handling; this declaration is not a
*       working scroller association API.
*
*/

extern struct Library *MUIMasterBase;

struct Scrollbutton_DATA
{
    WORD mx, my;
    WORD cx, cy;
    ULONG pos;
    Object *horiz_prop;
    Object *vert_prop;
    struct MUI_EventHandlerNode ehn;
};


/*
 * Prepend button frame, RelVerify input and button background before caller
 * tags, returning Area constructor result. No local state parsing or Prop
 * allocation; caller overrides may alter defaults.
 */
IPTR Scrollbutton__OM_NEW(struct IClass *cl, Object *o, struct opSet *msg)
{
    return DoSuperNewTags(cl, o, NULL,
        ButtonFrame,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Background, MUII_ButtonBack,
        TAG_MORE, (IPTR) msg->ops_AttrList);
}

/*
 * Write NewPosition or unassigned HorizProp/VertProp pointer fields to IPTR
 * storage and return TRUE. Other getters delegate to Area; no local
 * Horiz/Vert getter. No ownership transfer.
 */
IPTR Scrollbutton__OM_GET(struct IClass *cl, Object *o,
    struct opGet *msg)
{
    struct Scrollbutton_DATA *data =
        (struct Scrollbutton_DATA *)INST_DATA(cl, o);
    switch (msg->opg_AttrID)
    {
    case MUIA_Scrollbutton_NewPosition:
        *msg->opg_Storage = data->pos;
        return TRUE;

    case MUIA_Scrollbutton_HorizProp:
        *msg->opg_Storage = (IPTR) data->horiz_prop;
        return TRUE;

    case MUIA_Scrollbutton_VertProp:
        *msg->opg_Storage = (IPTR) data->vert_prop;
        return TRUE;

    default:
        return DoSuperMethodA(cl, o, (Msg) msg);
    }
}

/*
 * Store Horiz/Vert into WORD bases then return Area setter result. Caller
 * tags remain available for notifications; no NewPosition storage update or
 * pointer association. Conversion is unchecked; initialization uses no
 * corresponding local parser.
 */
IPTR Scrollbutton__OM_SET(struct IClass *cl, Object *o,
    struct opSet *msg)
{
    struct Scrollbutton_DATA *data =
        (struct Scrollbutton_DATA *)INST_DATA(cl, o);
    struct TagItem *tl = msg->ops_AttrList;
    struct TagItem *ti;

    while ((ti = NextTagItem(&tl)))
    {
        switch (ti->ti_Tag)
        {
        case MUIA_Scrollbutton_Horiz:
            data->cx = ti->ti_Data;
            break;

        case MUIA_Scrollbutton_Vert:
            data->cy = ti->ti_Data;
            break;
        }
    }
    return DoSuperMethodA(cl, o, (Msg) msg);
}

/*
 * Ask Area then add two to minimum/default dimensions and MUI_MAXMAX to
 * maximum dimensions. Requires writable MinMaxInfo; addition is not range
 * checked. Return zero ignoring superclass result.
 */
IPTR Scrollbutton__MUIM_AskMinMax(struct IClass *cl, Object *o,
    struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, o, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 2;
    msg->MinMaxInfo->DefWidth += 2;
    msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;

    msg->MinMaxInfo->MinHeight += 2;
    msg->MinMaxInfo->DefHeight += 2;
    msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
    return 0;
}

/*
 * Set up Area; return FALSE on failure. Fill/register mouse-button handler
 * and return TRUE ignoring registration result. Requires connected window
 * and matching Cleanup; no Prop resources are acquired.
 */
IPTR Scrollbutton__MUIM_Setup(struct IClass *cl, Object *o, Msg msg)
{
    struct Scrollbutton_DATA *data =
        (struct Scrollbutton_DATA *)INST_DATA(cl, o);
    if (!DoSuperMethodA(cl, o, msg))
        return FALSE;

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ehn.ehn_Object = o;
    data->ehn.ehn_Class = cl;

    DoMethod(_win(o), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return TRUE;
}

/*
 * Remove this event handler, call Area Cleanup, and return zero ignoring
 * its result. Requires a matching Setup and live window. Does not
 * separately reset Selected or coordinate bases.
 */
IPTR Scrollbutton__MUIM_Cleanup(struct IClass *cl, Object *o, Msg msg)
{
    struct Scrollbutton_DATA *data =
        (struct Scrollbutton_DATA *)INST_DATA(cl, o);
    DoMethod(_win(o), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);
    DoSuperMethodA(cl, o, msg);
    return 0;
}

/*
 * SELECTDOWN inside enables mouse moves, selects object and saves mouse
 * origin. Other button codes stop tracking and clear selection. Selected
 * moves add WORD bases to displacement, wrap to UWORD halves, store packed
 * position and notify. Return zero, no Area delegation. Requires valid
 * event/window and completed layout.
 */
IPTR Scrollbutton__MUIM_HandleEvent(struct IClass *cl, Object *o,
    struct MUIP_HandleEvent *msg)
{
    struct Scrollbutton_DATA *data =
        (struct Scrollbutton_DATA *)INST_DATA(cl, o);
    if (msg->imsg)
    {
        switch (msg->imsg->Class)
        {
        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code == SELECTDOWN)
            {
                if (msg->imsg->MouseX >= _left(o)
                    && msg->imsg->MouseX <= _right(o)
                    && msg->imsg->MouseY >= _top(o)
                    && msg->imsg->MouseY <= _bottom(o))
                {
                    DoMethod(_win(o), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                    DoMethod(_win(o), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);

                    set(o, MUIA_Selected, TRUE);
                    data->mx = msg->imsg->MouseX;
                    data->my = msg->imsg->MouseY;

                }
            }
            else
            {
                if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
                {
                    DoMethod(_win(o), MUIM_Window_RemEventHandler,
                        (IPTR) & data->ehn);
                    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                    DoMethod(_win(o), MUIM_Window_AddEventHandler,
                        (IPTR) & data->ehn);

                    set(o, MUIA_Selected, FALSE);
                }
            }
            break;

        case IDCMP_MOUSEMOVE:
            {
                IPTR sel = 0;

                get(o, MUIA_Selected, &sel);
                if (sel)
                {
                    UWORD x = (msg->imsg->MouseX - data->mx) + data->cx;
                    UWORD y = (msg->imsg->MouseY - data->my) + data->cy;
                    ULONG pos = x << 16 | y;

                    data->pos = pos;
                    set(o, MUIA_Scrollbutton_NewPosition, pos);
                }
                break;
            }
        }
    }
    return 0;
}


/*
 * Route local New/Get/Set/size/setup/cleanup/event handlers and delegate
 * other messages to Area. Return chosen handler result. No local Prop
 * ownership or automatic scrolling link.
 */
BOOPSI_DISPATCHER(IPTR, Scrollbutton_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Scrollbutton__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Scrollbutton__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_SET:
        return Scrollbutton__OM_SET(cl, obj, (struct opSet *)msg);
    case MUIM_AskMinMax:
        return Scrollbutton__MUIM_AskMinMax(cl, obj,
            (struct MUIP_AskMinMax *)msg);
    case MUIM_Setup:
        return Scrollbutton__MUIM_Setup(cl, obj, msg);
    case MUIM_Cleanup:
        return Scrollbutton__MUIM_Cleanup(cl, obj, msg);
    case MUIM_HandleEvent:
        return Scrollbutton__MUIM_HandleEvent(cl, obj,
            (struct MUIP_HandleEvent *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Scrollbutton_desc =
{
    MUIC_Scrollbutton,
    MUIC_Area,
    sizeof(struct Scrollbutton_DATA),
    (void *) Scrollbutton_Dispatcher
};
