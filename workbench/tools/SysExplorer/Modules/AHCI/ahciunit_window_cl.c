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

#include <hidd/storage.h>
#include <hidd/ahci.h>

#include "locale.h"

#include "ahci_classes.h"
#include "ahci_intern.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

extern OOP_AttrBase HiddAHCIUnitAB;

/*** Instance Data **********************************************************/
struct AHCIUnitWindow_DATA
{
    /* Nothing to add */
};

static const char *const featNames[] =
{
    "Write Cache" , "Read Ahead" , "Security Freeze Lock ",
    NULL
};

static void DecodeBits(char *str, ULONG flags, const char *const *names)
{
    unsigned char i, done;

    for (i = 0, done = 0; *names; names++)
    {
        if ((IPTR)*names < 32)
        {
            i = (IPTR)*names;
            continue;
        }
        if (flags & (1 << i))
        {
            strcpy(str, *names);
            str += strlen(*names);
            if ((done % 5) == 4)
                *str++ = '\n';
            else
                *str++ = ' ';
            done++;
        }
        i++;
    }

    *str = 0;
}

static Object *AHCIUnitWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    char feature_str[256];
    IPTR val;

    if (!dev)
        return NULL;

    /* AHCI specific attributes ... */
    OOP_GetAttr(dev, aHidd_AHCIUnit_Features, &val);
    DecodeBits(feature_str, val, featNames);

    return (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        Child, (IPTR)(ColGroup(2),
            MUIA_Group_SameSize, TRUE,
            MUIA_FrameTitle, (IPTR)"AHCI",
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
            Child, (IPTR)Label("Features"),
            Child, (IPTR)(ScrollgroupObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Scrollgroup_Contents, (IPTR)(NFloattextObject,
                    NoFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Floattext_Text, (IPTR)feature_str,
                End),
            End),
        End),
        TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE
    );;
}

/*** Setup ******************************************************************/
BOOPSI_DISPATCHER_PROTO(IPTR, AHCIUnitWindow_Dispatcher, cl, self, msg);
BOOPSI_DISPATCHER(IPTR, AHCIUnitWindow_Dispatcher, cl, self, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return (IPTR) AHCIUnitWindow__OM_NEW(cl, self, (struct opSet *)msg);

    default:
        return DoSuperMethodA(cl, self, msg);
    }

    return (IPTR) NULL;
}
BOOPSI_DISPATCHER_END

int AHCIUnitWindow_Initialize(struct SysexpBase *SysexpBase)
{
    struct SysexpAHCIBase *AhciBase = GetBase("AHCI.Module");
    struct MUI_CustomClass *SUWClass;

    D(bug("[ahci.sysexp] %s: AhciBase @ %p\n", __func__, AhciBase));

    if (AhciBase)
    {
        SUWClass = GetBase("StorageUnitWindow.Class");
        D(bug("[ahci.sysexp] %s: StorageUnitWindow.Class @ %p\n", __func__, SUWClass));
        AhciBase->seab_AHCIUnitWindowCLASS = MUI_CreateCustomClass
        (
            NULL, NULL, SUWClass,
            sizeof(struct AHCIUnitWindow_DATA), (APTR) AHCIUnitWindow_Dispatcher
        );
        if (AhciBase->seab_AHCIUnitWindowCLASS)
        {
            D(bug("[ahci.sysexp] %s: AHCIUnitWindowCLASS @ %p\n", __func__, AhciBase->seab_AHCIUnitWindowCLASS));
            AhciBase->seab_AHCIUnitWindowCLASS->mcc_Class->cl_UserData = (IPTR)AhciBase;
        }
    }
    if (!AhciBase || !AhciBase->seab_AHCIUnitWindowCLASS)
    {
        __showerror
        ( (char *)
            "Failed to create `AHCIUnitWindow' custom class.", NULL
        );

        return 0;
    }

    return 1;
}

void AHCIUnitWindow_Deinitialize(struct SysexpBase *SysexpBase)
{
    struct SysexpAHCIBase *AhciBase = GetBase("AHCI.Module");
    MUI_DeleteCustomClass(AhciBase->seab_AHCIUnitWindowCLASS);
}
