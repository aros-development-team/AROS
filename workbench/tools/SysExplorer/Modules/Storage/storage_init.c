/*
    Copyright (C) 2018-2019, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "storage_intern.h"

extern void StorageStartup(struct SysexpBase *);

#if (1) // TODO : Move into libbase
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;
OOP_AttrBase HiddBusAB;
OOP_AttrBase HiddStorageUnitAB;

OOP_MethodID HWBase;
OOP_MethodID HiddStorageControllerBase;
OOP_MethodID HiddStorageBusBase;
OOP_MethodID HiddStorageUnitBase;
#endif

const struct OOP_ABDescr storage_abd[] =
{
    {IID_Hidd                   , &HiddAttrBase         },
    {IID_HW                     , &HWAttrBase           },
    {IID_Hidd_Bus               , &HiddBusAB            },
    {IID_Hidd_StorageUnit       , &HiddStorageUnitAB    },
    {NULL                       , NULL                  }
};

/*****i storage/storageenum_init
    NAME
        int storageenum_init(

    SYNOPSIS
        struct SysexpStorageBase *StorageBase)

    FUNCTION
        Obtain the Hidd, HW, Hidd_Bus and Hidd_StorageUnit attribute
        bases and the HW, Hidd_StorageController, Hidd_StorageBus
        and Hidd_StorageUnit method IDs at offset zero. Store the
        results in module-global variables and initialize the
        module base handler list as empty.

    INPUTS
        StorageBase - Writable storage module base; must not be NULL.
                      Its handler list must not contain registered
                      handlers. OOP dependencies must be available.

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

        Reinitializing a populated handler list discards its links
        without freeing the registered handlers.

*****************************************************************************/
static int storageenum_init(struct SysexpStorageBase *StorageBase)
{
    D(bug("[storage.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(storage_abd);

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddStorageControllerBase = OOP_GetMethodID(IID_Hidd_StorageController, 0);
    HiddStorageBusBase = OOP_GetMethodID(IID_Hidd_StorageBus, 0);
    HiddStorageUnitBase = OOP_GetMethodID(IID_Hidd_StorageUnit, 0);

    NEWLIST(&StorageBase->sesb_HandlerList);
    D(bug("[storage.sysexp] %s: class list @ 0x%p\n", __func__, &StorageBase->sesb_HandlerList));

   return 2;
}

ADD2INITLIB(storageenum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register Storage.Module with SysExplorer at priority 100.
        Save the host library base and install StorageStartup
        as the startup callback.

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
                struct SysexpStorageBase *, StorageBase, 5, Storage
)
{
    AROS_LIBFUNC_INIT

    D(bug("[storage.sysexp] %s(%p)\n", __func__, SysexpBase));

    StorageBase->sesb_SysexpBase = SysexpBase;
    StorageBase->sesb_Module.sem_Node.ln_Name = "Storage.Module";
    StorageBase->sesb_Module.sem_Node.ln_Pri = 100;
    StorageBase->sesb_Module.sem_Startup = StorageStartup;
    RegisterModule(&StorageBase->sesb_Module, StorageBase);

    return;

    AROS_LIBFUNC_EXIT
}
