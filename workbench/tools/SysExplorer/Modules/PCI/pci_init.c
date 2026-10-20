/*
    Copyright (C) 2020, The AROS Development Team.
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

/*****i pci/pcienum_init
    NAME
        int pcienum_init(

    SYNOPSIS
        struct SysexpPCIBase *PciBase)

    FUNCTION
        Obtain the Hidd, Hidd_PCIDriver and Hidd_PCIDevice attribute
        bases and the Hidd_PCIDevice method ID at offset zero. Store
        the results in module-global variables.

    INPUTS
        PciBase - Module base supplied by library initialization.
                  Not accessed by this function. OOP dependencies must
                  be available.

    RESULT
        Returns 2 if the call completes. This does not establish
        successful attribute acquisition or valid method IDs.

    NOTES
        Run once during library initialization. Retain acquired
        attribute bases until their last use and release them
        through OOP when no longer needed.

    BUGS
        Attribute acquisition and method lookup failures are ignored.
        Failure to acquire an interface before the last descriptor
        can prevent OOP_ObtainAttrBases() from returning.

*****************************************************************************/
static int pcienum_init(struct SysexpPCIBase *PciBase)
{
    D(bug("[pci.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(pci_abd);

    HiddPCIDeviceBase = OOP_GetMethodID(IID_Hidd_PCIDevice, 0);

    return 2;
}

ADD2INITLIB(pcienum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register PCI.Module with SysExplorer at priority 90.
        Save the host library base and install PCIStartup
        and PCIShutdown as the lifecycle callbacks.

    INPUTS
        SysexpBase - Live SysExplorer library base with an initialized
                     module list. Must not be NULL; keep it valid
                     throughout module startup, use and shutdown.

    RESULT
        None. Registration failure is not reported.

    NOTES
        Call once for a newly opened module, before the host starts
        its registered modules. Serialize access to the host module
        list.

        The host copies the descriptor and retains its name, callback
        and module-base pointers. Callbacks run later; this call does
        not enumerate hardware or create windows.

        Keep the module resident until host shutdown, which closes
        the registered module. Registration does not acquire an
        additional library open reference.

    BUGS
        The registration helper does not check its allocation result.
        Allocation failure can cause an invalid memory access.

        Repeated calls create duplicate registrations and can cause
        repeated startup calls and multiple closes of the same module.

*****************************************************************************/
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
