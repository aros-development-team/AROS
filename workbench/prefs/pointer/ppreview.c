/*
    Copyright  2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/pointer.h>

#define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

#include "locale.h"
#include "ppreview.h"
#include "prefs.h"

/*** Instance Data **********************************************************/
struct PPreview_DATA
{
    Object                     *pprv_prevEditor;
    UWORD                       pprv_alpha;
    UWORD                       pprv_hspot_x;
    UWORD                       pprv_hspot_y;
    STRPTR                      pprv_filename;
    struct MUI_EventHandlerNode pprv_ehn;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PPreview_DATA *data = INST_DATA(cl, obj)

/*** Methods ****************************************************************/
Object *PPreview__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    const struct TagItem  *tstate = msg->ops_AttrList;
    struct TagItem        *tag    = NULL;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return 0;

    SETUP_INST_DATA;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PPreview_Alpha:
                data->pprv_alpha = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotX:
                data->pprv_hspot_x = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotY:
                data->pprv_hspot_y = tag->ti_Data;
                break;

            case MUIA_PPreview_FileName:
                data->pprv_filename = (STRPTR)tag->ti_Data;
                break;

        }
    }

    data->pprv_ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->pprv_ehn.ehn_Priority = 0;
    data->pprv_ehn.ehn_Flags    = 0;
    data->pprv_ehn.ehn_Object   = obj;
    data->pprv_ehn.ehn_Class    = cl;

    return obj;
}

IPTR PPreview__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    //...
    return DoSuperMethodA(cl, obj, msg);
}

IPTR PPreview__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    SETUP_INST_DATA;

    const struct TagItem *tags  = msg->ops_AttrList;
    struct TagItem       *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_PPreview_Alpha:
                data->pprv_alpha = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotX:
                data->pprv_hspot_x = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotY:
                data->pprv_hspot_y = tag->ti_Data;
                break;

            case MUIA_PPreview_FileName:
                data->pprv_filename = (STRPTR)tag->ti_Data;
                break;

        } /* switch(tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tags)) != NULL) */

    MUI_Redraw(obj, MADF_DRAWUPDATE);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    SETUP_INST_DATA;

    switch (msg->opg_AttrID)
    {
        case MUIA_PPreview_Alpha:
            *msg->opg_Storage = data->pprv_alpha;
            return TRUE;

        case MUIA_PPreview_HSpotX:
            *msg->opg_Storage = data->pprv_hspot_x;
            return TRUE;

        case MUIA_PPreview_HSpotY:
            *msg->opg_Storage = data->pprv_hspot_y;
            return TRUE;

        case MUIA_PPreview_FileName:
            *msg->opg_Storage = (IPTR)data->pprv_filename;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    SETUP_INST_DATA;

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->pprv_ehn);

    data->pprv_prevEditor = (Object *)XGET((Object *)XGET(obj, MUIA_Parent), MUIA_Parent);

    return TRUE;
}

IPTR PPreview__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    SETUP_INST_DATA;

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->pprv_ehn);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    SETUP_INST_DATA;

    DoSuperMethodA(cl, obj, (Msg)msg);

    SetAPen(_rp(obj), 0);
    RectFill
    (
        _rp(obj),
        _mleft(obj), _mtop(obj),
        _mright(obj), _mbottom(obj)
    );

    SetAPen(_rp(obj), 2);
    WritePixel(_rp(obj), _mleft(obj) + data->pprv_hspot_x, _mtop(obj) + data->pprv_hspot_y);

    return 0;
}

IPTR PPreview__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    //SETUP_INST_DATA;

    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth  += 64;
    msg->MinMaxInfo->MinHeight += 64;
    msg->MinMaxInfo->DefWidth  += 64;
    msg->MinMaxInfo->DefHeight += 64;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;

    return TRUE;
}

IPTR PPreview__MUIM_HandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    SETUP_INST_DATA;

    #define _between(a,x,b) ((x)>=(a) && (x)<=(b))
    #define _isinobject(x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))

    ALIVE

    if (msg->imsg)
    {
        switch (msg->imsg->Class)
        {
            case IDCMP_MOUSEBUTTONS:
            {
                if (msg->imsg->Code==SELECTUP)
                {
                    if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
                    {
                        data->pprv_hspot_x = msg->imsg->MouseX - _mleft(obj);
                        data->pprv_hspot_y = msg->imsg->MouseY - _mtop(obj);
                        D(bug("[PPreview/HandleEvent] X %d Y %d\n", data->pprv_hspot_x, data->pprv_hspot_y));
                        MUI_Redraw(obj, MADF_DRAWUPDATE);

                        SET(data->pprv_prevEditor, MUIA_PrefsEditor_Changed, TRUE);
                    }
                }
            }
            break;

        }
    }

    #undef _between
    #undef _isinobject

    return 0;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_9
(
    PPreview, NULL, MUIC_Area, NULL,
    OM_NEW,             struct opSet *,
    OM_DISPOSE,         Msg,
    OM_SET,             struct opSet *,
    OM_GET,             struct opGet *,
    MUIM_Setup,         struct MUIP_Setup *,
    MUIM_Cleanup,       struct MUIP_Cleanup *,
    MUIM_Draw,          struct MUIP_Draw *,
    MUIM_AskMinMax,     struct MUIP_AskMinMax *,
    MUIM_HandleEvent,   struct MUIP_HandleEvent *
);
