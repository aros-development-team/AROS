/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Loadable module init for the SysExplorer graphics enumerator.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <hidd/hidd.h>
#include <hidd/gfx.h>

#include "gfx_intern.h"

extern void GfxStartup(struct SysexpBase *);

#if (1) // TODO : Move into libbase
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddDisplayAttrBase;
#endif

const struct OOP_ABDescr gfx_abd[] =
{
    {IID_Hidd,          &HiddAttrBase    },
    {IID_Hidd_Gfx,      &HiddGfxAttrBase },
    {IID_Hidd_Display,  &HiddDisplayAttrBase },
    {NULL,              NULL             }
};

/*****i gfx/gfxenum_init
    NAME
        int gfxenum_init(

    SYNOPSIS
        struct SysexpGfxBase *GfxBase)

    FUNCTION
        Obtain the Hidd, Hidd_Gfx and Hidd_Display attribute bases
        and store them in module-global variables.

    INPUTS
        GfxBase - Module base supplied by library initialization.
                  Not accessed by this function. OOP dependencies must
                  be available.

    RESULT
        Returns 2 if the call completes, including when attribute
        acquisition fails.

    NOTES
        Run once during library initialization. Retain acquired
        attribute bases until their last use and release them
        through OOP when no longer needed.

    BUGS
        Attribute acquisition failure is ignored.
        Failure to acquire an interface before the last descriptor
        can prevent OOP_ObtainAttrBases() from returning.

*****************************************************************************/
static int gfxenum_init(struct SysexpGfxBase *GfxBase)
{
    D(bug("[gfx.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(gfx_abd);

    return 2;
}

ADD2INITLIB(gfxenum_init, 10);

/*****************************************************************************
    NAME
        void ModuleInit(

    SYNOPSIS
        void *SysexpBase)

    FUNCTION
        Register Gfx.Module with SysExplorer at priority 60.
        Save the host library base and install GfxStartup
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
                struct SysexpGfxBase *, GfxBase, 5, Gfx
)
{
    AROS_LIBFUNC_INIT

    D(bug("[gfx.sysexp] %s(%p)\n", __func__, SysexpBase));

    GfxBase->segb_SysexpBase = SysexpBase;
    GfxBase->segb_Module.sem_Node.ln_Name = "Gfx.Module";
    GfxBase->segb_Module.sem_Node.ln_Pri = 60;
    GfxBase->segb_Module.sem_Startup = GfxStartup;
    RegisterModule(&GfxBase->segb_Module, GfxBase);

    return;

    AROS_LIBFUNC_EXIT
}
