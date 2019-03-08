/*
    Copyright (C) 2015-2018, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#define __STORAGE_NOLIBBASE__

#include <proto/sysexp.h>
#include <proto/storage.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <zune/customclasses.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <hidd/storage.h>
#include <hidd/hidd.h>
#include <hidd/ata.h>

#include "locale.h"
#include "storage.h"

#include "ata_classes.h"
#include "ata_intern.h"

extern int ATABusWindow_Initialize(struct SysexpBase *SysexpBase);
extern void ATABusWindow_Deinitialize(struct SysexpBase *SysexpBase);
extern int ATAUnitWindow_Initialize(struct SysexpBase *SysexpBase);
extern void ATAUnitWindow_Deinitialize(struct SysexpBase *SysexpBase);

#if (1) // TODO : Move into libbase
extern OOP_AttrBase HiddStorageUnitAB;
extern OOP_AttrBase HiddATABusAB;
extern OOP_AttrBase HiddATAUnitAB;

extern OOP_MethodID HiddStorageControllerBase;
#endif

AROS_UFH3S(BOOL, ataunitenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, unit, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    struct SysexpEnum_data *edata = (struct SysexpEnum_data *)h->h_Data;
    struct InsertObjectMsg msg =
    {
        .obj      = unit,
    };
    CONST_STRPTR name;
    struct SysexpBase *SysexpBase;
    struct SysexpATABase *AtaBase;

    D(bug("[ata.sysexp] %s()\n", __func__));

    if (!edata)
        return TRUE;

    SysexpBase = edata->ed_sysexpbase;

    D(bug("[ata.sysexp] ataunit @ %p\n", unit));
    if (!unit)
        return TRUE;

    AtaBase = GetBase("ATA.Module");
    msg.winClass = AtaBase->seab_ATAUnitWindowCLASS;
    SysexpBase->GlobalCount++;

    OOP_GetAttr(unit, aHidd_StorageUnit_Model, (IPTR *)&name);
    DoMethod(SysexpBase->sesb_Tree, MUIM_NListtree_Insert, name, &msg,
             parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);

    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}

struct SysexpStorageBase *StorageBase;
static struct SysexpEnum_data privatehookdata =
{
    NULL,
    NULL
};

static void ataBusEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent)
{
    D(bug("[ata.sysexp] atabus: \n"));

    EnumBusUnits(obj, parent, ataunitenumFunc, &privatehookdata);
}

void ATAStartup(struct SysexpBase *SysexpBase)
{
    struct SysexpATABase *AtaBase;
    D(bug("[ata.sysexp] %s(%p)\n", __func__, SysexpBase));

    StorageBase = GetBase("Storage.Module");
    D(bug("[ata.sysexp] %s: StorageBase @ %p\n", __func__, StorageBase));
    if (!StorageBase)
    {
            __showerror
        ( (char *)
            "ATA enumerator requires `storage.sysexp'.", NULL
        );
    }
    AtaBase = GetBase("ATA.Module");
    D(bug("[ata.sysexp] %s: AtaBase @ %p\n", __func__, AtaBase));

    privatehookdata.ed_sysexpbase = SysexpBase;

    AtaBase->seab_DevicePageCLASS = GetBase("DevicePage.Class");
    AtaBase->seab_GenericWindowCLASS = GetBase("GenericWindow.Class");

    ATABusWindow_Initialize(SysexpBase);
    ATAUnitWindow_Initialize(SysexpBase);

    RegisterStorageControllerHandler(CLID_Hidd_ATA, 90, AtaBase->seab_GenericWindowCLASS, NULL, NULL);
    RegisterStorageBusHandler(CLID_Hidd_ATABus, 90, AtaBase->seab_ATABusWindowCLASS, ataBusEnum, NULL);
}

void ATAShutdown(struct SysexpBase *SysexpBase)
{
    D(bug("[ata.sysexp] %s(%p)\n", __func__, SysexpBase));

    ATAUnitWindow_Deinitialize(SysexpBase);
    ATABusWindow_Deinitialize(SysexpBase);
}
