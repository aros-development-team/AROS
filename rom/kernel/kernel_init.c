/*
    Copyright � 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Main kernel.resource initialization.
    Lang: english
*/

#include <aros/asmcall.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include LC_LIBDEFS_FILE

#include <kernel_globals.h>

#include <kernel_debug.h>

/* We have own bug(), so don't use aros/debug.h to avoid conflicts */
#define D(x)

static const UBYTE version[];
extern const char LIBEND;

AROS_UFP3S(struct KernelBase *, Kernel_Init,
    AROS_UFPA(ULONG, dummy, D0),
    AROS_UFPA(BPTR, segList, A0),
    AROS_UFPA(struct ExecBase *, sysBase, A6));

const struct Resident Kernel_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Kernel_resident,
    (APTR)&LIBEND,
    RTF_SINGLETASK,
    VERSION_NUMBER,
    NT_LIBRARY,
    RESIDENTPRI,
    MOD_NAME_STRING,
    (STRPTR)&version[6],
    Kernel_Init
};

static const UBYTE version[] = VERSION_STRING;

void __clear_bss(const struct KernelBSS *bss)
{
    while (bss->addr)
    {
	bzero((void*)bss->addr, bss->len);
        bss++;
    }
}

extern const APTR GM_UNIQUENAME(FuncTable)[];

THIS_PROGRAM_HANDLES_SYMBOLSET(INITLIB)
DEFINESET(INITLIB)

/*
 * Init routine is intentionally written by hands.
 * It can use kernel's own memory allocator (if implemented) for KernelBase creation.
 * This allows not to rely on working exec's memory management before kernel.resource
 * is set up. This can simplify exec.library code on MMU-aware systems.
 * exec.library catches our AddResource() and sets up its pooled memory manager. 
 */

AROS_UFH3S(struct KernelBase *, Kernel_Init,
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct KernelBase *KernelBase = NULL;
    int i;

    D(bug("[KRN] Kernel_Init()\n"));

    KernelBase = AllocKernelBase(SysBase);
    if (!KernelBase)
    	return NULL;

    KernelBase->kb_Node.ln_Type = NT_RESOURCE;
    KernelBase->kb_Node.ln_Pri  = RESIDENTPRI;
    KernelBase->kb_Node.ln_Name = MOD_NAME_STRING;

    MakeFunctions(KernelBase, GM_UNIQUENAME(FuncTable), NULL);

    D(bug("[KRN] KernelBase 0x%p\n", KernelBase));

    for (i=0; i < EXCEPTIONS_COUNT; i++)
	NEWLIST(&KernelBase->kb_Exceptions[i]);

    for (i=0; i < IRQ_COUNT; i++)
        NEWLIST(&KernelBase->kb_Interrupts[i]);

    /*
     * Everything is ok, add our resource.
     * exec.library catches this call and sets up its memory management.
     * At this point kernel.resource's debug I/O and memory management must be
     * fully functional. After this we'll be able to safely call CreatePool() etc.
     */
    AddResource(KernelBase);

    /* Call platform-specific init code */
    if (!set_call_libfuncs(SETNAME(INITLIB), 1, 1, KernelBase))
    	return NULL;

    D(bug("[KRN] Kernel_Init() done\n"));

    /* Set global KernelBase storage and return */
    D(bug("[%s] Set global KernelBase\n"));
    setKernelBase(KernelBase);

    return KernelBase;
    
    AROS_USERFUNC_EXIT;
}
