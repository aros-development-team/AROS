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
#include <hidd/ahci.h>

#include "locale.h"
#include "storage.h"

#include "ahci_classes.h"
#include "ahci_intern.h"

extern int AHCIBusWindow_Initialize(struct SysexpBase *SysexpBase);
extern void AHCIBusWindow_Deinitialize(struct SysexpBase *SysexpBase);
extern int AHCIUnitWindow_Initialize(struct SysexpBase *SysexpBase);
extern void AHCIUnitWindow_Deinitialize(struct SysexpBase *SysexpBase);

#if (1) // TODO : Move into libbase
extern OOP_AttrBase HiddStorageUnitAB;
extern OOP_AttrBase HiddAHCIUnitAB;
#endif

struct MUI_CustomClass *DevicePage_CLASS;

AROS_UFH3S(BOOL, ahciunitenumFunc,
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
    struct SysexpAHCIBase *AhciBase;

    D(bug("[ahci.sysexp] %s()\n", __func__));

    if (!edata)
        return TRUE;

    SysexpBase = edata->ed_sysexpbase;

    D(bug("[ahci.sysexp] ahciunit @ %p\n", unit));
    if (!unit)
        return TRUE;

    AhciBase = GetBase("AHCI.Module");
    msg.winClass = AhciBase->seab_AHCIUnitWindowCLASS;
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

static void ahciBusEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent)
{
    D(bug("[ahci.sysexp] ahcibus: \n"));

    EnumBusUnits(obj, parent, ahciunitenumFunc, &privatehookdata);
}

void AHCIStartup(struct SysexpBase *SysexpBase)
{
    struct SysexpAHCIBase *AhciBase;
    D(bug("[ahci.sysexp] %s(%p)\n", __func__, SysexpBase));

    StorageBase = GetBase("Storage.Module");
    D(bug("[ahci.sysexp] %s: StorageBase @ %p\n", __func__, StorageBase));
    if (!StorageBase)
    {
            __showerror
        ( (char *)
            "AHCI enumerator requires `storage.sysexp'.", NULL
        );
    }
    AhciBase = GetBase("AHCI.Module");
    D(bug("[ahci.sysexp] %s: AhciBase @ %p\n", __func__, AhciBase));

    privatehookdata.ed_sysexpbase = SysexpBase;

    AhciBase->seab_DevicePageCLASS = GetBase("DevicePage.Class");
    AhciBase->seab_GenericWindowCLASS = GetBase("GenericWindow.Class");

    AHCIBusWindow_Initialize(SysexpBase);
    AHCIUnitWindow_Initialize(SysexpBase);

    RegisterStorageControllerHandler((CONST_STRPTR)CLID_Hidd_AHCI, 90, AhciBase->seab_GenericWindowCLASS, NULL, NULL);
    RegisterStorageBusHandler((CONST_STRPTR)CLID_Hidd_AHCIBus, 90, AhciBase->seab_AHCIBusWindowCLASS, ahciBusEnum, NULL);
}

void AHCIShutdown(struct SysexpBase *SysexpBase)
{
    D(bug("[ahci.sysexp] %s(%p)\n", __func__, SysexpBase));

    AHCIUnitWindow_Deinitialize(SysexpBase);
    AHCIBusWindow_Deinitialize(SysexpBase);
}
