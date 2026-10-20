/*
    Copyright (C) 2022-2025, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "power_intern.h"

extern void PowerStartup(struct SysexpBase *);

/*****i power/powerenum_init
    NAME
        int powerenum_init(

    SYNOPSIS
        struct SysexpPowerBase *PowerBase)

    FUNCTION
        Obtain the Hidd, HW, Hidd_Telemetry and Hidd_Power attribute
        bases. Store the HW, Hidd_Telemetry and Hidd_Power method IDs
        at offset zero in the module base.

    INPUTS
        PowerBase - Writable power module base; must not be NULL.
                    OOP dependencies must be available.

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
static int powerenum_init(struct SysexpPowerBase *PowerBase)
{
    const struct OOP_ABDescr power_abd[] =
    {
        {IID_Hidd,              &HiddAttrBase           },
        {IID_HW,                &HWAttrBase             },
        {IID_Hidd_Telemetry,    &HiddTelemetryAB        },
        {IID_Hidd_Power,        &HiddPowerAB            },
        {NULL,                  NULL                    }
    };
    D(bug("[power.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(power_abd);

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddTelemetryBase = OOP_GetMethodID(IID_Hidd_Telemetry, 0);
    HiddPowerBase = OOP_GetMethodID(IID_Hidd_Power, 0);

   return 2;
}

ADD2INITLIB(powerenum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register Power.Module with SysExplorer at priority 100.
        Save the host library base and install PowerStartup
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
