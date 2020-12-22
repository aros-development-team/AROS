/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "ia_install.h"
#include "ia_installoption_intern.h"

IPTR InstallOption__OM_NEW(Class * CLASS, Object * self, struct opSet *message)
{
    char *iaID;

    D(bug("[InstallAROS:Opt] %s()\n", __func__));

    iaID = (char *)GetTagData(MUIA_InstallOption_ID, 0, message->ops_AttrList);
    if (iaID)
    {
        Object *iaObj = (Object *)GetTagData(MUIA_InstallOption_Obj, 0, message->ops_AttrList);
        if (!iaObj)
        {
            switch((LONG)iaID)
            {
                case MUIV_InstallOptionID_Source:
                    {
                        iaObj = StringObject,
                                MUIA_CycleChain, 1,
                                MUIA_Frame, MUIV_Frame_String,
                            End;
                        D(bug("[InstallAROS:Opt] %s: MUIV_InstallOptionID_Source Obj @ 0x%p\n", __func__, iaObj));
                    }
                    break;
                case MUIV_InstallOptionID_Dest:
                    {
                        iaObj = StringObject,
                                MUIA_CycleChain, 1,
                                MUIA_Frame, MUIV_Frame_String,
                            End;
                        D(bug("[InstallAROS:Opt] %s: MUIV_InstallOptionID_Dest Obj @ 0x%p\n", __func__, iaObj));
                    }
                    break;
                case MUIV_InstallOptionID_StorageAvail:
                    break;
            }
        }
        if (iaObj)
        {
            struct TagItem newTags[] =
            {
                { MUIA_Group_Child,     iaObj                   },
                { TAG_MORE,             message->ops_AttrList   }
            };
            struct opSet newMsg =
            {
                .MethodID = message->MethodID,
                .ops_AttrList = newTags,
                .ops_GInfo = message->ops_GInfo
            };
            if (!newTags[1].ti_Data)
                newTags[1].ti_Tag = TAG_DONE;
            self = (Object *) DoSuperMethodA(CLASS, self, (Msg) &newMsg);
            if (self)
            {
                struct InstallOption_Data *data = INST_DATA(CLASS, self);
                data->io_Object = iaObj;
                data->io_ID = iaID;
                SET(data->io_Object, MUIA_UserData, self);
            }
            return self;
        }
    }
    return (IPTR)NULL;
}

IPTR InstallOption__OM_GET(Class * CLASS, Object * self, struct opGet *message)
{
    struct InstallOption_Data *data = INST_DATA(CLASS, self);

    //D(bug("[InstallAROS:Opt] %s()\n", __func__));

    switch(message->opg_AttrID)
    {
        case MUIA_InstallOption_Obj:
            *message->opg_Storage = data->io_Object;
            return TRUE;
        case MUIA_InstallOption_ID:
            *message->opg_Storage = data->io_ID;
            return TRUE;
    }
    return DoSuperMethodA(CLASS, self, message);
}

IPTR InstallOption__OM_SET(Class * CLASS, Object * self, struct opSet *message)
{
    struct InstallOption_Data *data = INST_DATA(CLASS, self);

    //D(bug("[InstallAROS:Opt] %s()\n", __func__));

    return DoSuperMethodA(CLASS, self, message);
}

BOOPSI_DISPATCHER(IPTR, InstallOption__Dispatcher, CLASS, self, message)
{
    //D(bug("[InstallAROS:Opt] %s(%08x)\n", __func__, message->MethodID));

    /* Handle our methods */
    switch (message->MethodID)
    {
    case OM_NEW:
        return InstallOption__OM_NEW(CLASS, self, (struct opSet *)message);

    case OM_GET:
        return InstallOption__OM_GET(CLASS, self, (struct opGet *)message);

    case OM_SET:
        return InstallOption__OM_SET(CLASS, self, (struct opSet *)message);
    }
    return DoSuperMethodA(CLASS, self, message);
}
BOOPSI_DISPATCHER_END
