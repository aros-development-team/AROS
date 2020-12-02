/*
    Copyright (C) 2020, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#include <proto/sysexp.h>

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

#include <hidd/pci.h>
#include <hardware/pci.h>

#include "locale.h"
#include "pci_classes.h"
#include "enums.h"

#include "pci_intern.h"

extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HiddPCIDriverAB;
extern OOP_AttrBase HiddPCIDeviceAB;

extern OOP_MethodID HiddPCIDeviceBase;

struct SysexpPCIEnum_data
{
    struct SysexpBase *ed_sysexpbase; 
    struct MUI_NListtree_TreeNode *ed_tn;
    OOP_Object *ed_Controller;
};

static
AROS_UFH3(void, PCIEnumerator_h,
    AROS_UFHA(struct Hook *,    h,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    struct SysexpPCIEnum_data *edata = (struct SysexpPCIEnum_data *)h->h_Data;
    struct InsertObjectMsg msg =
    {
        .obj      = Device,
    };
    struct SysexpBase *SysexpBase;
    char *name;
    UWORD devSubClass;

//    struct SysexpPCIBase *PciBase;

    D(bug("[ata.sysexp] %s()\n", __func__));

    if (!edata)
        return;

    SysexpBase = edata->ed_sysexpbase;

    D(bug("[pci.sysexp] %s: PCI device @ 0x%p\n", __func__, Device));
    if (!Device)
        return;

//    PciBase = GetBase("PCI.Module");
    msg.winClass = PCIDevWindow_CLASS;
    SysexpBase->GlobalCount++;

    OOP_GetAttr(Device, aHidd_HardwareName, (IPTR *)&name);
    D(bug("[pci.sysexp] %s:     '%s'\n", __func__, name));

    /* Check if this is a PCI-PCI bridge */
    devSubClass = HIDD_PCIDevice_ReadConfigWord(Device, PCIBR_SUBCLASS);
    D(bug("[pci.sysexp] %s: subclass = %04x\n", __func__, devSubClass));
    if ((devSubClass & 0xFF00) == 0x0600)
    {
        D(bug("[pci.sysexp] %s: previous treenode @ 0x%p\n", __func__, edata->ed_tn));
        DoMethod(SysexpBase->sesb_Tree, MUIM_NListtree_Insert, name, &msg,
            edata->ed_tn, MUIV_NListtree_Insert_PrevNode_Tail, 0);
    }
    return;

    AROS_USERFUNC_EXIT
}

static struct SysexpPCIEnum_data privatehookdata =
{
    NULL
};

void pciEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    struct Hook FindHook =
    {
        h_Entry:    (IPTR (*)())PCIEnumerator_h,
        h_Data:     &privatehookdata
    };
    static const struct TagItem Requirements[] =
    {
        {TAG_DONE }
    };
    struct pHidd_PCI_EnumDevices enummsg =
    {
        mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
        callback:       &FindHook,
        requirements:   Requirements,
    };
    OOP_Object *pci;
    privatehookdata.ed_Controller = obj;

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
        privatehookdata.ed_tn = tn;
        D(bug("[pci.sysexp] %s: Enumerating devices attached to PCI controller (treenode @ 0x%p)\n", __func__, tn));
        OOP_DoMethod(pci, &enummsg.mID);
        OOP_DisposeObject(pci);
    }
}

BOOL pciValid(OOP_Object *obj, ULONG *flags)
{
    D(bug("[pci.sysexp] %s()\n", __func__));
#if (0)
    if (OOP_OCLASS(obj) == OOP_FindClass(CLID_Hidd_PCI))
    {
        ULONG _flags = *flags;
        _flags &= ~ TNF_LIST;
        *flags = _flags;
    }
#endif
    return TRUE;
}

void PCIStartup(struct SysexpBase *SysexpBase)
{
    struct SysexpPCIBase *PciBase;
    D(bug("[pci.sysexp] %s(%p)\n", __func__, SysexpBase));

    PciBase = GetBase("PCI.Module");
    D(bug("[pci.sysexp] %s: PciBase @ %p\n", __func__, PciBase));

    privatehookdata.ed_sysexpbase = SysexpBase;

    PciBase->sepb_DevicePageCLASS = GetBase("DevicePage.Class");
    PciBase->sepb_GenericWindowCLASS = GetBase("GenericWindow.Class");

    RegisterClassHandler(CLID_Hidd_PCIDriver, 90, PCIWindow_CLASS, pciEnum, NULL);
//    RegisterClassHandler(CLID_Hidd_PCIDevice, 90, PCIDevWindow_CLASS, NULL, pciValid);
}

void PCIShutdown(struct SysexpBase *SysexpBase)
{
    D(bug("[pci.sysexp] %s(%p)\n", __func__, SysexpBase));

}

