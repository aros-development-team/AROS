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
/* #define DEBUG 1 */
#include <zune/customclasses.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>

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
    UWORD alpha;
    UWORD hspot_x;
    UWORD hspot_y;
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
                data->alpha = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotX:
                data->hspot_x = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotY:
                data->hspot_y = tag->ti_Data;
                break;

        }
    }

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
                data->alpha = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotX:
                data->hspot_x = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotY:
                data->hspot_y = tag->ti_Data;
                break;

        } /* switch(tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tags)) != NULL) */

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    SETUP_INST_DATA;

    IPTR retval = TRUE;

    switch (msg->opg_AttrID)
    {
        case MUIA_PPreview_Alpha:
            *msg->opg_Storage = data->alpha;
            return TRUE;

        case MUIA_PPreview_HSpotX:
            *msg->opg_Storage = data->hspot_x;
            return TRUE;

        case MUIA_PPreview_HSpotY:
            *msg->opg_Storage = data->hspot_y;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    SETUP_INST_DATA;

    DoSuperMethodA(cl, obj, (Msg)msg);
    return 0;
}

IPTR PPreview__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    SETUP_INST_DATA;

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

    if (msg->imsg)
    {
        switch (msg->imsg->Class)
        {
            case IDCMP_MOUSEBUTTONS:
            {
                if (msg->imsg->Code==SELECTDOWN)
                {
                    if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
                    {
#if 0
                        data->x = msg->imsg->MouseX;
                        data->y = msg->imsg->MouseY;
                        MUI_Redraw(obj,MADF_DRAWUPDATE);
#endif
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
ZUNE_CUSTOMCLASS_7
(
    PPreview, NULL, MUIC_Area, NULL,
    OM_NEW,             struct opSet *,
    OM_DISPOSE,         Msg,
    OM_SET,             struct opSet *,
    OM_GET,             struct opGet *,
    MUIM_Draw,          struct MUIP_Draw *,
    MUIM_AskMinMax,     struct MUIP_AskMinMax *,
    MUIM_HandleEvent,   struct MUIP_HandleEvent *
);
