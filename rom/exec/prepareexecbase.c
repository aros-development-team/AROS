/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sets up the ExecBase a bit. (Mostly clearing).
    Lang:
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <clib/macros.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <aros/arossupportbase.h>
#include <aros/asmcall.h>

#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>

#include LC_LIBDEFS_FILE
#include "memory.h"
#include "exec_intern.h"

#undef kprintf /* This can't be used in the code here */

extern void *LIBFUNCTABLE[];

extern struct Resident Exec_resident; /* Need this for lib_IdString */

extern void Exec_TrapHandler(ULONG trapNum);
AROS_LD3(ULONG, MakeFunctions,
	 AROS_LDA(APTR, target, A0),
	 AROS_LDA(CONST_APTR, functionArray, A1),
	 AROS_LDA(CONST_APTR, funcDispBase, A2),
         struct ExecBase *, SysBase, 15, Exec);

/* Default finaliser. */
static void Exec_TaskFinaliser(void)
{
    /* Get rid of current task. */
    RemTask(SysBase->ThisTask);
}

#undef kprintf
#undef rkprintf
#undef vkprintf

void _aros_not_implemented(char *X)
{
    kprintf("Unsupported function at offset -0x%h in %s\n",
	    ABS(*(WORD *)((&X)[-1]-2)),
	    ((struct Library *)(&X)[-2])->lib_Node.ln_Name);
}

struct Library *PrepareAROSSupportBase (struct MemHeader *mh)
{
	struct AROSSupportBase *AROSSupportBase;

	AROSSupportBase = Allocate(mh, sizeof(struct AROSSupportBase));
	
	AROSSupportBase->kprintf = (void *)kprintf;
	AROSSupportBase->rkprintf = (void *)rkprintf;
	AROSSupportBase->vkprintf = (void *)vkprintf;
    	NEWLIST(&AROSSupportBase->AllocMemList);

	/* FIXME: Add code to read in the debug options */
	AROSSupportBase->StdOut = NULL;
	AROSSupportBase->DebugConfig = NULL;

	return (struct Library *)AROSSupportBase;
}

/*
 *  PrepareExecBase() will initialize the ExecBase to default values,
 *  and not add anything yet (except for the MemHeader).
 *  WARNING: this routine intentionally sets up global SysBase.
 *  This is done because:
 *  1. PrepareAROSSupportBase() calls AllocMem() which relies on functional SysBase
 *  2. After PrepareAROSSupportBase() it is possible to call debug output functions
 *     (kprintf() etc). Yes, KernelBase is not set up yet, but remember that kernel.resource
 *     may have patched functions in AROSSupportBase so that KernelBase is not needed there.
 *  3. Existing ports (at least UNIX-hosted and Windows-hosted) rely on the fact that SysBase is
 *     set up here.
 *
 *  Resume: please be extremely careful, study existing code, and think five times if you decide to
 *  change this. You WILL break existing ports if you do not modify their code accordingly. There's
 *  nothing really bad in the fact that global SysBase is touched here and changing this does not
 *  really win something.
 *						Pavel Fedin <pavel_fedin@mail.ru>
*/

struct ExecBase *PrepareExecBase(struct MemHeader *mh, char *args, struct HostInterface *data)
{
    ULONG   negsize = 0;
    ULONG totalsize, i;
    VOID  **fp      = LIBFUNCTABLE;

    /* TODO: at this point we should check if SysBase already exists and, if so,
       take special care about reset-surviving things. */

    /* Calculate the size of the vector table */
    while (*fp++ != (VOID *) -1) negsize += LIB_VECTSIZE;
    
    /* Align library base */
    negsize = AROS_ALIGN(negsize);
    
    /* Allocate memory for library base */
    totalsize = negsize + sizeof(struct IntExecBase);
    SysBase = (struct ExecBase *)((UBYTE *)stdAlloc(mh, totalsize, 0, NULL) + negsize);

    /* Clear the library base */
    memset(SysBase, 0, sizeof(struct IntExecBase));

#ifdef HAVE_PREPAREPLATFORM
    /* Setup platform-specific data. This is needed for CacheClearU() and CacheClearE() on MinGW32 */
    if (!Exec_PreparePlatform(&PD(SysBase), data))
	return NULL;
#endif

    /* Setup function vectors */
    AROS_CALL3(ULONG, AROS_SLIB_ENTRY(MakeFunctions, Exec),
	      AROS_UFCA(APTR, SysBase, A0),
	      AROS_UFCA(CONST_APTR, LIBFUNCTABLE, A1),
	      AROS_UFCA(CONST_APTR, NULL, A2),
	      struct ExecBase *, SysBase);

    SysBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    SysBase->LibNode.lib_Node.ln_Pri  = -100;
    SysBase->LibNode.lib_Node.ln_Name = "exec.library";
    SysBase->LibNode.lib_IdString     = (char *)Exec_resident.rt_IdString;
    SysBase->LibNode.lib_Version      = VERSION_NUMBER;
    SysBase->LibNode.lib_Revision     = REVISION_NUMBER;
    SysBase->LibNode.lib_OpenCnt      = 1;
    SysBase->LibNode.lib_NegSize      = negsize;
    SysBase->LibNode.lib_PosSize      = sizeof(struct IntExecBase);
    SysBase->LibNode.lib_Flags        = 0;

    NEWLIST(&SysBase->MemList);
    SysBase->MemList.lh_Type = NT_MEMORY;
    ADDHEAD(&SysBase->MemList, &mh->mh_Node);
    
    NEWLIST(&SysBase->ResourceList);
    SysBase->ResourceList.lh_Type = NT_RESOURCE;
    
    NEWLIST(&SysBase->DeviceList);
    SysBase->DeviceList.lh_Type = NT_DEVICE;

    NEWLIST(&SysBase->IntrList);
    SysBase->IntrList.lh_Type = NT_INTERRUPT;

    NEWLIST(&SysBase->LibList);
    SysBase->LibList.lh_Type = NT_LIBRARY;
    ADDHEAD(&SysBase->LibList, &SysBase->LibNode.lib_Node);

    NEWLIST(&SysBase->PortList);
    SysBase->PortList.lh_Type = NT_MSGPORT;

    NEWLIST(&SysBase->TaskReady);
    SysBase->TaskReady.lh_Type = NT_TASK;

    NEWLIST(&SysBase->TaskWait);
    SysBase->TaskWait.lh_Type = NT_TASK;

    NEWLIST(&SysBase->SemaphoreList);
    SysBase->TaskWait.lh_Type = NT_SEMAPHORE;

    NEWLIST(&SysBase->ex_MemHandlers);

    for (i = 0; i < 5; i++)
    {
	NEWLIST(&SysBase->SoftInts[i].sh_List);
	SysBase->SoftInts[i].sh_List.lh_Type = NT_INTERRUPT;
    }

    NEWLIST(&((struct IntExecBase *)SysBase)->ResetHandlers);

    SysBase->SoftVer        = VERSION_NUMBER;

    SysBase->ColdCapture    = SysBase->CoolCapture
	                    = SysBase->WarmCapture
			    = NULL;

    SysBase->MaxLocMem      = (IPTR)mh->mh_Upper;

    SysBase->Quantum        = 4;

    SysBase->TaskTrapCode   = Exec_TrapHandler;
    SysBase->TaskExceptCode = NULL;
    SysBase->TaskExitCode   = Exec_TaskFinaliser;
    SysBase->TaskSigAlloc   = 0xFFFF;
    SysBase->TaskTrapAlloc  = 0;

    /* Default frequencies */
    SysBase->VBlankFrequency = 50;
    SysBase->PowerSupplyFrequency = 1;

    SysBase->ChkBase=~(IPTR)SysBase;

    /* Parse some arguments from command line */
    if (args)
    {
	char *s;

	/* Set VBlank and EClock frequencies if specified */
	s = strstr(args, "vblank=");
	if (s)
	    SysBase->VBlankFrequency = atoi(&s[7]);

	s = strstr(args, "eclock=");
	if (s)
	    SysBase->ex_EClockFrequency = atoi(&s[7]);

	/* Enable mungwall before the first AllocMem() */
	s = strstr(args, "mungwall");
	if (s)
	    PrivExecBase(SysBase)->IntFlags = EXECF_MungWall;
    }

    SysBase->DebugAROSBase = PrepareAROSSupportBase(mh);

    return SysBase;
}
