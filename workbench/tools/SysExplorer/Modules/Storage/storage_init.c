/*
    Copyright (C) 2018, The AROS Development Team.
    $Id$
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
