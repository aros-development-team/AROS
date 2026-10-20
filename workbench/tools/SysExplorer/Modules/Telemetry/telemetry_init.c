/*
    Copyright (C) 2025, The AROS Development Team.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include "telemetry_intern.h"

extern void TelemetryStartup(struct SysexpBase *);

/*****i telemetry/telemetryenum_init
    NAME
        int telemetryenum_init(

    SYNOPSIS
        struct SysexpTelemetryBase *SeTelemetryBase)

    FUNCTION
        Obtain the Hidd, HW and Hidd_Telemetry attribute bases, then
        store the HW and Hidd_Telemetry method IDs at offset zero.

    INPUTS
        SeTelemetryBase - Writable, initialized telemetry module base.
                          Must not be NULL. OOP dependencies must be
                          available before this hook runs.

    RESULT
        Returns 2 if control reaches the end. This value does not
        confirm successful attribute acquisition or valid method IDs.

    NOTES
        The module base retains the attribute bases and method IDs.
        Run once during library initialization. Successful attribute
        acquisitions require corresponding releases when no longer
        needed.

    BUGS
        Attribute acquisition and method lookup failures are not
        checked. An unknown method interface can leave an invalid
        method ID in the module base.

        The called OOP_ObtainAttrBases() implementation can fail to
        return if acquisition fails before the final interface. Its
        loop clearing the remaining destinations does not advance.

*****************************************************************************/
static int telemetryenum_init(struct SysexpTelemetryBase *SeTelemetryBase)
{
    const struct OOP_ABDescr telemetry_abd[] =
    {
        {IID_Hidd                   , &HiddAttrBase         },
        {IID_HW                     , &HWAttrBase           },
        {IID_Hidd_Telemetry         , &HiddTelemetryAB      },
        {NULL                       , NULL                  }
    };
    D(bug("[telemetry.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(telemetry_abd);

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HiddTelemetryBase = OOP_GetMethodID(IID_Hidd_Telemetry, 0);

    return 2;
}

ADD2INITLIB(telemetryenum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register this telemetry module with SysExplorer under the
        name Telemetry.Module and priority 100. Save the supplied
        SysExplorer base for subsequent module operations.

    INPUTS
        SysexpBase - Live SysExplorer library base with an initialized
                     module list. Must not be NULL. It must remain
                     valid throughout module startup, use and shutdown.

    RESULT
        None. The call provides no registration status.

    NOTES
        Call once for a newly opened telemetry module, from the
        SysExplorer initialization task. Serialize changes to the
        host module list.

        Registration copies the module descriptor and retains its
        name, callback and module-base pointers. It installs
        TelemetryStartup for the host to call later; it does not
        start telemetry enumeration or create windows here.

        The host closes the registered module at shutdown. Keep its
        code and data resident until that shutdown completes.

    BUGS
        The registration helper does not check its allocation result.
        Allocation failure can cause an invalid memory access instead
        of a reported failure.

        Repeated calls create duplicate registrations. They can cause
        repeated startup calls and multiple closes of the same module.

*****************************************************************************/
AROS_LH1(void, ModuleInit,
                AROS_LHA(void *, SysexpBase, A0),
                struct SysexpTelemetryBase *, SeTelemetryBase, 5, Telemetry
)
{
    AROS_LIBFUNC_INIT

    D(bug("[telemetry.sysexp] %s(%p)\n", __func__, SysexpBase));

    SeTelemetryBase->sesb_SysexpBase = SysexpBase;
    SeTelemetryBase->sesb_Module.sem_Node.ln_Name = "Telemetry.Module";
    SeTelemetryBase->sesb_Module.sem_Node.ln_Pri = 100;
    SeTelemetryBase->sesb_Module.sem_Startup = TelemetryStartup;
    RegisterModule(&SeTelemetryBase->sesb_Module, SeTelemetryBase);

    return;

    AROS_LIBFUNC_EXIT
}
