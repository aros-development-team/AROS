/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sets up the ExecBase a bit. (Mostly clearing).
    Lang:
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
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

extern struct Library * PrepareAROSSupportBase (struct ExecBase *);
extern struct Resident Exec_resident; /* Need this for lib_IdString */

extern void Exec_TrapHandler(ULONG trapNum);
AROS_LD3(ULONG, MakeFunctions,
	 AROS_LDA(APTR, target, A0),
	 AROS_LDA(CONST_APTR, functionArray, A1),
	 AROS_LDA(CONST_APTR, funcDispBase, A2),
         struct ExecBase *, SysBase, 15, Exec);

static APTR allocmem(struct MemHeader *mh, ULONG size)
{
    UBYTE *ret = NULL;
    
    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;
        if (mhe->mhe_Alloc)
	    ret = mhe->mhe_Alloc(mhe, size, NULL);
    }
    else
    {
        size = (size + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);
        ret  = (UBYTE *)mh->mh_First;

        mh->mh_First          = (struct MemChunk *)(ret + size);
        mh->mh_First->mc_Next = NULL;
        mh->mh_Free           = mh->mh_First->mc_Bytes
	                      = ((struct MemChunk *)ret)->mc_Bytes - size;
    }

    return ret;
}

/* Default finaliser. */
static void Exec_TaskFinaliser(void)
{
    /* Get rid of current task. */
    RemTask(SysBase->ThisTask);
}

/*
    PrepareExecBase() will initialize the ExecBase to default values,
    and not add anything yet (except for the MemHeader).
*/

struct ExecBase *PrepareExecBase(struct MemHeader *mh, char *args, struct HostInterface *data)
{
    ULONG   negsize = 0, i;
    VOID  **fp      = LIBFUNCTABLE;
    
    /* Calculate the size of the vector table */
    while (*fp++ != (VOID *) -1) negsize += LIB_VECTSIZE;
    
    /* Align library base */
    negsize = AROS_ALIGN(negsize);
    
    /* Allocate memory for library base */
    SysBase = (struct ExecBase *)
	    ((UBYTE *)allocmem(mh, negsize + sizeof(struct IntExecBase)) + negsize);

    /* Clear the library base */
    memset(SysBase, 0, sizeof(struct IntExecBase));

#ifdef HAVE_PREPAREPLATFORM
    /* Setup platform-specific data. This is needed for CacheClearU() and CacleClearE() on MinGW32 */
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

    SysBase->DebugAROSBase  = PrepareAROSSupportBase(SysBase);

    return SysBase;
}
