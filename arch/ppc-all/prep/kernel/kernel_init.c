/*
    Copyright C 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kernel.resource startup file
    Lang: English
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>

#include <asm/macros.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel.h"
#include "libdefs.h"

static const char Kernel_VersionID[] = VERSION_STRING;
static const char Kernel_Name[] = NAME_STRING;

struct KernelBase * Kernel_init();

#ifndef __ROM__
static LONG __no_exec()
{
    return(-1);
}
#endif

struct Resident Kernel_Resident SECTION_CODE = {
    RTC_MATCHWORD,
    &Kernel_Resident,
    &Kernel_end,
    0,
    VERSION_NUMBER,
    NT_RESOURCE,
    125,
    Kernel_Name,
    Kernel_VersionID,
    Kernel_init
};

#ifdef SysBase
#undef SysBase
#endif

AROS_UFH3(struct KernelBase *, Kernel_init,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(ULONG, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct KernelBase *KernelBase = NULL;
    UWORD neg = AROS_ALIGN(LIB_VECTSIZE * VECTOR_COUNT);

    KernelBase = (struct KernelBase *)
	((ULONG)AllocMem(neg + sizeof(struct KernelBase),
	    MEMF_CLEAR | MEMF_PUBLIC) + neg);

    if (KernelBase)
    {
	KernelBase->sysBase = SysBase;
	KernelBase->node.ln_Pri = Kernel_Resident.rt_Pri;
	KernelBase->node.ln_Name = Kernel_Resident.rt_Name;
	KernelBase->node.ln_Type = NT_RESOURCE;

	MakeFunctions(KernelBase, (APTR)LIBFUNCTABLE, NULL);
	AddResource(KernelBase);
    }

    return(KernelBase);

    AROS_USERFUNC_EXIT
}

