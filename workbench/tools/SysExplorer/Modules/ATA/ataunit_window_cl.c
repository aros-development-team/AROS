/*
    Copyright (C) 2013-2019, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/sysexp.h>
#include <proto/storage.h>

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
#include <hidd/ata.h>

#include "locale.h"

#include "ata_classes.h"
#include "ata_intern.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

extern OOP_AttrBase HiddATAUnitAB;
extern OOP_AttrBase HiddStorageUnitAB;

struct MUI_CustomClass * ATAUnitWindow_CLASS;

/*** Instance Data **********************************************************/
struct ATAUnitWindow_DATA
{
    /* Nothing to add */
};

static const char *const xferModeNames[] =
{
    "PIO0" , "PIO1" , "PIO2 ", "PIO3" , "PIO4" ,
    "MDMA0", "MDMA1", "MDMA2",
    "UDMA0", "UDMA1", "UDMA2", "UDMA3", "UDMA4", "UDMA5", "UDMA6",
    (const char *)AB_XFER_48BIT,
    "LBA48", "Multisector", "ATAPI", "LBA", "PIO32",
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

static Object *ATAUnitWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    Object *window, *atagroup;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    char xfermodes_str[256];
    char usemodes_str[256];
    char multisector_str[4];
    IPTR val;

    if (!dev)
        return NULL;

    /* ATA specific attributes ... */
    OOP_GetAttr(dev, aHidd_ATAUnit_XferModes, &val);
    DecodeBits(xfermodes_str, val, xferModeNames);

    OOP_GetAttr(dev, aHidd_ATAUnit_ConfiguredModes, &val);
    DecodeBits(usemodes_str, val, xferModeNames);

    OOP_GetAttr(dev, aHidd_ATAUnit_MultiSector, &val);
    snprintf(multisector_str, sizeof(multisector_str), "%ld", val);
    window = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        Child, (IPTR)(atagroup = (ColGroup(2),
            MUIA_Group_SameSize, TRUE,
            MUIA_FrameTitle, (IPTR)"IDE/ATA",
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
            Child, (IPTR)Label(_(MSG_TRANSFER_MODES)),
            Child, (IPTR)(ScrollgroupObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Scrollgroup_Contents, (IPTR)(NFloattextObject,
                    NoFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Floattext_Text, (IPTR)xfermodes_str,
                End),
            End),
            Child, (IPTR)Label(_(MSG_CONFIG_MODES)),
            Child, (IPTR)(ScrollgroupObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Scrollgroup_Contents, (IPTR)(NFloattextObject,
                    NoFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Floattext_Text, (IPTR)usemodes_str,
                End),
            End),
            Child, (IPTR)Label(_(MSG_MULTISECTOR)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)multisector_str,
            End),
        End)),
        TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE
    );
    if (window)
    {
        IPTR unitdev;
        OOP_GetAttr(dev, aHidd_StorageUnit_Device, &unitdev);
        OOP_GetAttr(dev, aHidd_StorageUnit_Number, &val);
        QueryATAStorageFeatures(atagroup, (char *)unitdev, val);
    }
    return window;
}

/*** Setup ******************************************************************/
BOOPSI_DISPATCHER_PROTO(IPTR, ATAUnitWindow_Dispatcher, __class, __self, __msg);
BOOPSI_DISPATCHER(IPTR, ATAUnitWindow_Dispatcher, __class, __self, __msg)
{
    switch (__msg->MethodID)
    {
    case OM_NEW:
        return (IPTR) ATAUnitWindow__OM_NEW(__class, __self, (struct opSet *)__msg);

    default:
        return DoSuperMethodA(__class, __self, __msg);
    }

    return (IPTR) NULL;
}
BOOPSI_DISPATCHER_END

int ATAUnitWindow_Initialize(struct SysexpBase *SysexpBase)
{
    struct SysexpATABase *AtaBase = GetBase("ATA.Module");
    struct MUI_CustomClass *SUWClass;

    D(bug("[ata.sysexp] %s: AtaBase @ %p\n", __func__, AtaBase));

    if (AtaBase)
    {
        SUWClass = GetBase("StorageUnitWindow.Class");
        D(bug("[ata.sysexp] %s: StorageUnitWindow.Class @ %p\n", __func__, SUWClass));
        AtaBase->seab_ATAUnitWindowCLASS = MUI_CreateCustomClass
        (
            NULL, NULL, SUWClass,
            sizeof(struct ATAUnitWindow_DATA), (APTR) ATAUnitWindow_Dispatcher
        );
        if (AtaBase->seab_ATAUnitWindowCLASS)
        {
            D(bug("[ata.sysexp] %s: ATAUnitWindowCLASS @ %p\n", __func__, AtaBase->seab_ATAUnitWindowCLASS));
            AtaBase->seab_ATAUnitWindowCLASS->mcc_Class->cl_UserData = (IPTR)AtaBase;
        }
    }
    if (!AtaBase || !AtaBase->seab_ATAUnitWindowCLASS)
    {
        __showerror
        ( (char *)
            "Failed to create `ATAUnitWindow' custom class.", NULL
        );

        return 0;
    }

    return 1;
}

void ATAUnitWindow_Deinitialize(struct SysexpBase *SysexpBase)
{
    struct SysexpATABase *AtaBase = GetBase("ATA.Module");
    MUI_DeleteCustomClass(AtaBase->seab_ATAUnitWindowCLASS);
}
