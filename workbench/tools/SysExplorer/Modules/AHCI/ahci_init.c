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

static int ahcienum_init(struct SysexpAHCIBase *AhciBase) 
{
    D(bug("[ahci.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(ahci_abd);

    HiddStorageControllerBase = OOP_GetMethodID(IID_Hidd_StorageController, 0);

    return 2;
}

ADD2INITLIB(ahcienum_init, 10);

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
