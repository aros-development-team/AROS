/*
    Copyright (C) 2022, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "power_intern.h"

extern void PowerStartup(struct SysexpBase *);

#if (0) // TODO : Move into libbase
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;
OOP_AttrBase HiddPowerAB;

OOP_MethodID HWBase;
OOP_MethodID HiddPowerBase;
#endif

static int powerenum_init(struct SysexpPowerBase *PowerBase)
{
    const struct OOP_ABDescr power_abd[] =
    {
        {IID_Hidd                   , &HiddAttrBase         },
        {IID_HW                     , &HWAttrBase           },
        {IID_Hidd_Power       , &HiddPowerAB    },
        {NULL                       , NULL                  }
    };
    D(bug("[power.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(power_abd);

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddPowerBase = OOP_GetMethodID(IID_Hidd_Power, 0);

   return 2;
}

ADD2INITLIB(powerenum_init, 10);

AROS_LH1(void, ModuleInit,
                AROS_LHA(void *, SysexpBase, A0),
                struct SysexpPowerBase *, PowerBase, 5, Power
)
{
    AROS_LIBFUNC_INIT

    D(bug("[power.sysexp] %s(%p)\n", __func__, SysexpBase));

    PowerBase->sesb_SysexpBase = SysexpBase;
    PowerBase->sesb_Module.sem_Node.ln_Name = "Power.Module";
    PowerBase->sesb_Module.sem_Node.ln_Pri = 100;
    PowerBase->sesb_Module.sem_Startup = PowerStartup;
    RegisterModule(&PowerBase->sesb_Module, PowerBase);

    return;

    AROS_LIBFUNC_EXIT
}
