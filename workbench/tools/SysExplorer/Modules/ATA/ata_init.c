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

static int ataenum_init(struct SysexpATABase *AtaBase) 
{
    D(bug("[ata.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(ata_abd);

    HiddStorageControllerBase = OOP_GetMethodID(IID_Hidd_StorageController, 0);

    return 2;
}

ADD2INITLIB(ataenum_init, 10);

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
