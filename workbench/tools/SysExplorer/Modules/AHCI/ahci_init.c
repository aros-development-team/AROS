/*
    Copyright (C) 2018-2019, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "ahci_intern.h"

extern void AHCIStartup(struct SysexpBase *);
extern void AHCIShutdown(struct SysexpBase *);

#if (1) // TODO : Move into libbase
OOP_AttrBase HiddStorageUnitAB;
OOP_AttrBase HiddAHCIUnitAB;

OOP_MethodID HiddStorageControllerBase;
#endif

const struct OOP_ABDescr ahci_abd[] =
{
    {IID_Hidd_StorageUnit ,  &HiddStorageUnitAB },
    {IID_Hidd_AHCIUnit,  &HiddAHCIUnitAB},
    {NULL            ,  NULL          }
};

/*****i ahci/ahcienum_init
    NAME
        int ahcienum_init(

    SYNOPSIS
        struct SysexpAHCIBase *AhciBase)

    FUNCTION
        Obtain the Hidd_StorageUnit and Hidd_AHCIUnit attribute bases
        and the Hidd_StorageController method ID at offset zero.
        Store the results in module-global variables.

    INPUTS
        AhciBase - Module base supplied by library initialization.
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
static int ahcienum_init(struct SysexpAHCIBase *AhciBase)
{
    D(bug("[ahci.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(ahci_abd);

    HiddStorageControllerBase = OOP_GetMethodID(IID_Hidd_StorageController, 0);

    return 2;
}

ADD2INITLIB(ahcienum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register AHCI.Module with SysExplorer at priority 90.
        Save the host library base and install AHCIStartup
        and AHCIShutdown as the lifecycle callbacks.

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
                struct SysexpAHCIBase *, AhciBase, 5, Ahci
)
{
    AROS_LIBFUNC_INIT

    D(bug("[ahci.sysexp] %s(%p)\n", __func__, SysexpBase));

    AhciBase->seab_SysexpBase = SysexpBase;
    AhciBase->seab_Module.sem_Node.ln_Name = "AHCI.Module";
    AhciBase->seab_Module.sem_Node.ln_Pri = 90;
    AhciBase->seab_Module.sem_Startup = AHCIStartup;
    AhciBase->seab_Module.sem_Shutdown = AHCIShutdown;
    RegisterModule(&AhciBase->seab_Module, AhciBase);

    return;

    AROS_LIBFUNC_EXIT
}
