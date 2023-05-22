/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#define INTUITION_NO_INLINE_STDARG

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <clib/alib_protos.h>

#include <dos/dos.h>
#include <exec/types.h>

#include "ia_install.h"
#include "ia_install_intern.h"
#include "ia_stage.h"
#include "ia_stage_intern.h"
#include "ia_option.h"
#include "ia_option_intern.h"

IPTR Install__OM_NEW(Class * CLASS, Object * self, struct opSet *message)
{
    struct MUI_CustomClass *iscMcc =
        MUI_CreateCustomClass(NULL, MUIC_Notify, NULL,
        sizeof(struct InstallStage_DATA), InstallStage__Dispatcher);
    if (!iscMcc)
    {
        return 0;
    }

    struct MUI_CustomClass *iocMcc =
        MUI_CreateCustomClass(NULL, MUIC_Group, NULL,
        sizeof(struct InstallOption_Data), InstallOption__Dispatcher);
    if (!iocMcc)
    {
        MUI_DeleteCustomClass(iscMcc);
        return 0;
    }

    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    if (self)
    {
        struct Install_DATA *data = INST_DATA(CLASS, self);
		data->id_OptionClass = iocMcc;
		data->id_StageClass = iscMcc;
        NEWLIST(&data->id_Options);
        NEWLIST(&data->id_Stages);
    }
    else
    {
        MUI_DeleteCustomClass(iocMcc);
        MUI_DeleteCustomClass(iscMcc);
    }

    D(bug("[InstallAROS:Install] %s: returning 0x%p\n", __func__,self));

    return (IPTR) self;
}

IPTR Install__OM_DISPOSE(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);

    D(bug("[InstallAROS:Install] %s(0x%p)\n", __func__, self));

    MUI_DeleteCustomClass(data->id_OptionClass);
    MUI_DeleteCustomClass(data->id_StageClass);

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Install__OM_GET(Class * CLASS, Object * self, struct opGet * message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);

    switch(message->opg_AttrID)
    {
        case MUIA_Install_StageClass:
			*((struct IClass  **)message->opg_Storage) = data->id_StageClass->mcc_Class;
            return TRUE;

        case MUIA_Install_OptionClass:
			*((struct IClass  **)message->opg_Storage) = data->id_OptionClass->mcc_Class;
            return TRUE;
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Install__MUIM_Install_AddOption(Class * CLASS, Object * self, struct MUIP_Install_NewOption * message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    struct TagItem ioTags[] = {
        { MUIA_InstallOption_List, (IPTR)&data->id_Options},
        { TAG_MORE, (IPTR)message->OptTags }
    };

    Object * OptionObj = NULL;

    OptionObj = NewObjectA(data->id_OptionClass->mcc_Class, NULL, ioTags);

    return (IPTR)OptionObj;
}

BOOPSI_DISPATCHER(IPTR, Install__Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW:
        return Install__OM_NEW(CLASS, self, (struct opSet *)message);

    case OM_DISPOSE:
        return Install__OM_DISPOSE(CLASS, self, message);

    case OM_GET:
        return Install__OM_GET(CLASS, self, (struct opGet *)message);

    case MUIM_Install_AddOption:
        return Install__MUIM_Install_AddOption(CLASS, self, (struct MUIP_Install_NewOption *)message);

    default:
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
