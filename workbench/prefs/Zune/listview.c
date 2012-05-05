/*
    Copyright (C) 2012, The AROS Development Team.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <devices/rawkeycodes.h>
#include <zune/customclasses.h>

#ifdef __AROS__
#include <proto/alib.h>
#endif

#include "zunestuff.h"

IPTR ClassListview__OM_NEW(struct IClass *CLASS, Object *obj, struct opSet *message)
{
    struct ClassListview_DATA        *data = NULL;

    obj = (Object *)DoSuperNewTags(CLASS, obj, NULL,
        TAG_MORE, (IPTR) message->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(CLASS, obj);

    data->list = (Object *)GetTagData(MUIA_Listview_List, (IPTR)NULL, message->ops_AttrList);
    
    data->ehn.ehn_Events   = IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = CLASS;

    return (IPTR)obj;
}

IPTR ClassListview__MUIM_Setup(struct IClass *CLASS, Object *obj, struct MUIP_Setup *message)
{
    struct ClassListview_DATA        *data = INST_DATA(CLASS, obj);

    if (!DoSuperMethodA(CLASS, obj, (Msg) message)) return (IPTR)NULL;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    return 1;
}

IPTR ClassListview__MUIM_Cleanup(struct IClass *CLASS, Object *obj, struct MUIP_Cleanup *message)
{
    struct ClassListview_DATA        *data = INST_DATA(CLASS, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(CLASS, obj, (Msg)message);
}

IPTR ClassListview__MUIM_HandleEvent(struct IClass *CLASS, Object *obj, struct MUIP_HandleEvent *message)
{
    struct ClassListview_DATA        *data = INST_DATA(CLASS, obj);
    IPTR retval = MUI_EventHandlerRC_Eat;

    if ((message->imsg) && (message->imsg->Class == IDCMP_RAWKEY) && (obj == XGET(_win(obj), MUIA_Window_ActiveObject)))
    {
        switch(message->imsg->Code)
        {
            case RAWKEY_HOME:
                SET(data->list, MUIA_List_Active, MUIV_List_Active_Top);
                break;
            case RAWKEY_END:
                SET(data->list, MUIA_List_Active, MUIV_List_Active_Bottom);
                break;
            case RAWKEY_PAGEUP:
                SET(data->list, MUIA_List_Active, MUIV_List_Active_PageUp);
                break;
            case RAWKEY_PAGEDOWN:
                SET(data->list, MUIA_List_Active, MUIV_List_Active_PageDown);
                break;
            case RAWKEY_UP:
                SET(data->list, MUIA_List_Active, MUIV_List_Active_Up);
                break;
            case RAWKEY_DOWN:
                SET(data->list, MUIA_List_Active, MUIV_List_Active_Down);
                break;
            default:
                retval = 0;
                break;
        }
    }
    else
        retval = 0;

    return retval;
}

BOOPSI_DISPATCHER(IPTR, ClassListview_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW:
        return ClassListview__OM_NEW(CLASS, self, (struct opSet *)message);

    case MUIM_Setup:
        return ClassListview__MUIM_Setup(CLASS, self, (struct MUIP_Setup *)message);

    case MUIM_Cleanup:
        return ClassListview__MUIM_Cleanup(CLASS, self, (struct MUIP_Cleanup *)message);

    case MUIM_HandleEvent:
        return ClassListview__MUIM_HandleEvent(CLASS, self, (struct MUIP_HandleEvent *)message);

    default:
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
