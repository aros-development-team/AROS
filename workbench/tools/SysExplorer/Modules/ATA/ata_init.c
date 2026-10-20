/*
    Copyright (C) 2018-2019, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "ata_intern.h"

extern void ATAStartup(struct SysexpBase *);
extern void ATAShutdown(struct SysexpBase *);

#if (1) // TODO : Move into libbase
OOP_AttrBase HiddStorageUnitAB;
OOP_AttrBase HiddATABusAB;
OOP_AttrBase HiddATAUnitAB;

OOP_MethodID HiddStorageControllerBase;
#endif

const struct OOP_ABDescr ata_abd[] =
{
    {IID_Hidd_StorageUnit ,  &HiddStorageUnitAB },
    {IID_Hidd_ATABus ,  &HiddATABusAB },
    {IID_Hidd_ATAUnit,  &HiddATAUnitAB},
    {NULL            ,  NULL          }
};

/*****i ata/ataenum_init
    NAME
        int ataenum_init(

    SYNOPSIS
        struct SysexpATABase *AtaBase)

    FUNCTION
        Obtain the Hidd_StorageUnit, Hidd_ATABus and Hidd_ATAUnit
        attribute bases and the Hidd_StorageController method ID at
        offset zero. Store the results in module-global variables.

    INPUTS
        AtaBase - Module base supplied by library initialization.
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
static int ataenum_init(struct SysexpATABase *AtaBase)
{
    D(bug("[ata.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(ata_abd);

    HiddStorageControllerBase = OOP_GetMethodID(IID_Hidd_StorageController, 0);

    return 2;
}

ADD2INITLIB(ataenum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register ATA.Module with SysExplorer at priority 90.
        Save the host library base and install ATAStartup
        and ATAShutdown as the lifecycle callbacks.

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

        Storage.Module must be registered and started before the
        host invokes ATAStartup.

    BUGS
        The registration helper does not check its allocation result.
        Allocation failure can cause an invalid memory access.

        Repeated calls create duplicate registrations and can cause
        repeated startup calls and multiple closes of the same module.

*****************************************************************************/
AROS_LH1(void, ModuleInit,
                AROS_LHA(void *, SysexpBase, A0),
                struct SysexpATABase *, AtaBase, 5, Ata
)
{
    AROS_LIBFUNC_INIT

    D(bug("[ata.sysexp] %s(%p)\n", __func__, SysexpBase));

    AtaBase->seab_SysexpBase = SysexpBase;
    AtaBase->seab_Module.sem_Node.ln_Name = "ATA.Module";
    AtaBase->seab_Module.sem_Node.ln_Pri = 90;
    AtaBase->seab_Module.sem_Startup = ATAStartup;
    AtaBase->seab_Module.sem_Shutdown = ATAShutdown;
    RegisterModule(&AtaBase->seab_Module, AtaBase);

    return;

    AROS_LIBFUNC_EXIT
}
