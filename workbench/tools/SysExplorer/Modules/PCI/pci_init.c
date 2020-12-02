/*
    Copyright (C) 2020, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include "pci_intern.h"

extern void PCIStartup(struct SysexpBase *);
extern void PCIShutdown(struct SysexpBase *);

#if (1) // TODO : Move into libbase
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HiddPCIDriverAB;
OOP_AttrBase HiddPCIDeviceAB;

OOP_MethodID HiddPCIDeviceBase;
#endif

const struct OOP_ABDescr pci_abd[] =
{
    {IID_Hidd,  &HiddAttrBase },
    {IID_Hidd_PCIDriver,  &HiddPCIDriverAB },
    {IID_Hidd_PCIDevice,  &HiddPCIDeviceAB},
    {NULL            ,  NULL          }
};

static int pcienum_init(struct SysexpPCIBase *PciBase) 
{
    D(bug("[pci.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(pci_abd);

    HiddPCIDeviceBase = OOP_GetMethodID(IID_Hidd_PCIDevice, 0);

    return 2;
}

ADD2INITLIB(pcienum_init, 10);

AROS_LH1(void, ModuleInit,
                AROS_LHA(void *, SysexpBase, A0),
                struct SysexpPCIBase *, PciBase, 5, Pci
)
{
    AROS_LIBFUNC_INIT

    D(bug("[pci.sysexp] %s(%p)\n", __func__, SysexpBase));

    PciBase->sepb_SysexpBase = SysexpBase;
    PciBase->sepb_Module.sem_Node.ln_Name = "PCI.Module";
    PciBase->sepb_Module.sem_Node.ln_Pri = 90;
    PciBase->sepb_Module.sem_Startup = PCIStartup;
    PciBase->sepb_Module.sem_Shutdown = PCIShutdown;
    RegisterModule(&PciBase->sepb_Module, PciBase);

    return;

    AROS_LIBFUNC_EXIT
}
