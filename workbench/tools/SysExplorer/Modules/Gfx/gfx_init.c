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
#endif

const struct OOP_ABDescr gfx_abd[] =
{
    {IID_Hidd,          &HiddAttrBase    },
    {IID_Hidd_Gfx,      &HiddGfxAttrBase },
    {NULL,              NULL             }
};

static int gfxenum_init(struct SysexpGfxBase *GfxBase)
{
    D(bug("[gfx.sysexp] %s()\n", __func__));

    OOP_ObtainAttrBases(gfx_abd);

    return 2;
}

ADD2INITLIB(gfxenum_init, 10);

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
