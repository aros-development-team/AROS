/*
    Copyright (C) 2018, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/sysexp.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <exec/memory.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <mui/NFloattext_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <oop/oop.h>

#include <hidd/ahci.h>

#include "locale.h"

#include "ahci_classes.h"
#include "ahci_intern.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

extern OOP_AttrBase HiddAHCIBusAB;

/*** Instance Data **********************************************************/
struct AHCIBusWindow_DATA
{
    /* Nothing to add */
};

static Object *AHCIBusWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpAHCIBase *AhciBase = (struct SysexpAHCIBase *)cl->cl_UserData;

    D(bug("[ahci.sysexp] %s: cl @ %p\n", __func__, cl));

    if (!AhciBase)
        return NULL;

    return (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        Child, (IPTR)(ColGroup(3),
            MUIA_Group_SameSize, TRUE,
            MUIA_FrameTitle, (IPTR)"AHCI",
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
            Child, (IPTR)HSpace(0),
            Child, (IPTR)HSpace(0),
            Child, (IPTR)HSpace(0),
        End),
        TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE
    );
}

/*** Setup ******************************************************************/
BOOPSI_DISPATCHER_PROTO(IPTR, AHCIBusWindow_Dispatcher, cl, self, msg);
BOOPSI_DISPATCHER(IPTR, AHCIBusWindow_Dispatcher, cl, self, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return (IPTR) AHCIBusWindow__OM_NEW(cl, self, (struct opSet *)msg);

    default:
        return DoSuperMethodA(cl, self, msg);
    }

    return (IPTR) NULL;
}
BOOPSI_DISPATCHER_END

int AHCIBusWindow_Initialize(struct SysexpBase *SysexpBase)
{
    struct SysexpAHCIBase *AhciBase = GetBase("AHCI.Module");
    struct MUI_CustomClass *SBWClass;

    D(bug("[ahci.sysexp] %s: AhciBase @ %p\n", __func__, AhciBase));

    if (AhciBase)
    {
        SBWClass = GetBase("StorageBusWindow.Class");
        D(bug("[ahci.sysexp] %s: StorageBusWindow.Class @ %p\n", __func__, SBWClass));
        AhciBase->seab_AHCIBusWindowCLASS = MUI_CreateCustomClass
        (
            NULL, NULL, SBWClass,
            sizeof(struct AHCIBusWindow_DATA), (APTR) AHCIBusWindow_Dispatcher
        );
        if (AhciBase->seab_AHCIBusWindowCLASS)
        {
            D(bug("[ahci.sysexp] %s: AHCIBusWindowCLASS @ %p\n", __func__, AhciBase->seab_AHCIBusWindowCLASS));
            AhciBase->seab_AHCIBusWindowCLASS->mcc_Class->cl_UserData = (IPTR)AhciBase;
        }
    }
    if (!AhciBase || !AhciBase->seab_AHCIBusWindowCLASS)
    {
        __showerror
        ( (char *)
            "Failed to create `AHCIBusWindow' custom class.", NULL
        );

        return 0;
    }

    return 1;
}

void AHCIBusWindow_Deinitialize(struct SysexpBase *SysexpBase)
{
    struct SysexpAHCIBase *AhciBase = GetBase("AHCI.Module");
    MUI_DeleteCustomClass(AhciBase->seab_AHCIBusWindowCLASS);
}
