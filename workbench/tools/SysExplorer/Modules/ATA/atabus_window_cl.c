/*
    Copyright (C) 2013-2018, The AROS Development Team.
    $Id: ata_window_cl.c 51417 2016-01-25 18:10:16Z NicJA $
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

#include <hidd/ata.h>

#include "locale.h"

#include "ata_classes.h"
#include "ata_intern.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

extern OOP_AttrBase HiddATABusAB;

/*** Instance Data **********************************************************/
struct ATABusWindow_DATA
{
    /* Nothing to add */
};

static inline void SetCheckState(Object *img, OOP_Object *dev, ULONG attr)
{
    LONG state = OOP_GET(dev, attr) ? IDS_SELECTED : IDS_NORMAL;

    SET(img, MUIA_Image_State, state);
}

static Object *ATABusWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpATABase *AtaBase =  (struct SysexpATABase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    LONG ioalt, pio32, use80wire, dma;

    D(bug("[ata.sysexp] %s: cl @ %p\n", __func__, cl));

    if ((!AtaBase) || (!dev))
        return NULL;

    ioalt     = OOP_GET(dev, aHidd_ATABus_UseIOAlt)  ? IDS_SELECTED : IDS_NORMAL;
    pio32     = OOP_GET(dev, aHidd_ATABus_Use32Bit)  ? IDS_SELECTED : IDS_NORMAL;
    use80wire = OOP_GET(dev, aHidd_ATABus_Use80Wire) ? IDS_SELECTED : IDS_NORMAL;
    dma       = OOP_GET(dev, aHidd_ATABus_UseDMA)    ? IDS_SELECTED : IDS_NORMAL;

    return (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        Child, (IPTR)(ColGroup(3),
            MUIA_Group_SameSize, TRUE,
            MUIA_FrameTitle, (IPTR)"IDE/ATA",
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
            Child, (IPTR)Label(_(MSG_USE_IOALT)),
            Child, (IPTR)(ImageObject,
                MUIA_Image_Spec, MUII_CheckMark,
                MUIA_Image_State, ioalt,
                TextFrame,
                MUIA_CycleChain, 1,
                MUIA_Background, MUII_TextBack,
            End),
            Child, (IPTR)HSpace(0),
            Child, (IPTR)Label(_(MSG_USE_32BIT)),
            Child, (IPTR)(ImageObject,
                MUIA_Image_Spec, MUII_CheckMark,
                MUIA_Image_State, pio32,
                TextFrame,
                MUIA_CycleChain, 1,
                MUIA_Background, MUII_TextBack,
            End),
            Child, (IPTR)HSpace(0),
            Child, (IPTR)Label(_(MSG_USE_80WIRE)),
            Child, (IPTR)(ImageObject,
                MUIA_Image_Spec, MUII_CheckMark,
                MUIA_Image_State, use80wire,
                TextFrame,
                MUIA_CycleChain, 1,
                MUIA_Background, MUII_TextBack,
            End),
            Child, (IPTR)HSpace(0),
            Child, (IPTR)Label(_(MSG_USE_DMA)),
            Child, (IPTR)(ImageObject,
                MUIA_Image_Spec, MUII_CheckMark,
                MUIA_Image_State, dma,
                TextFrame,
                MUIA_CycleChain, 1,
                MUIA_Background, MUII_TextBack,
            End),
            Child, (IPTR)HSpace(0),
        End),
        TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE
    );
}

/*** Setup ******************************************************************/
BOOPSI_DISPATCHER_PROTO(IPTR, ATABusWindow_Dispatcher, __class, __self, __msg);
BOOPSI_DISPATCHER(IPTR, ATABusWindow_Dispatcher, __class, __self, __msg)
{
    switch (__msg->MethodID)
    {
    case OM_NEW:
        return (IPTR) ATABusWindow__OM_NEW(__class, __self, (struct opSet *)__msg);

    default:
        return DoSuperMethodA(__class, __self, __msg);
    }

    return (IPTR) NULL;
}
BOOPSI_DISPATCHER_END

int ATABusWindow_Initialize(struct SysexpBase *SysexpBase)
{
    struct SysexpATABase *AtaBase = GetBase("ATA.Module");
    struct MUI_CustomClass *SBWClass;

    D(bug("[ata.sysexp] %s: AtaBase @ %p\n", __func__, AtaBase));

    if (AtaBase)
    {
        SBWClass = GetBase("StorageBusWindow.Class");
        D(bug("[ata.sysexp] %s: StorageBusWindow.Class @ %p\n", __func__, SBWClass));
        AtaBase->seab_ATABusWindowCLASS = MUI_CreateCustomClass
        (
            NULL, NULL, SBWClass,
            sizeof(struct ATABusWindow_DATA), (APTR) ATABusWindow_Dispatcher
        );
        if (AtaBase->seab_ATABusWindowCLASS)
        {
            D(bug("[ata.sysexp] %s: ATABusWindowCLASS @ %p\n", __func__, AtaBase->seab_ATABusWindowCLASS));
            AtaBase->seab_ATABusWindowCLASS->mcc_Class->cl_UserData = (IPTR)AtaBase;
        }
    }
    if (!AtaBase || !AtaBase->seab_ATABusWindowCLASS)
    {
        __showerror
        ( (char *)
            "Failed to create `ATABusWindow' custom class.", NULL
        );

        return 0;
    }

    return 1;
}

void ATABusWindow_Deinitialize(struct SysexpBase *SysexpBase)
{
    struct SysexpATABase *AtaBase = GetBase("ATA.Module");
    MUI_DeleteCustomClass(AtaBase->seab_ATABusWindowCLASS);
}
