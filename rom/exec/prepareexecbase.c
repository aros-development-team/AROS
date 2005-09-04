/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sets up the ExecBase a bit. (Mostly clearing).
    Lang:
*/


#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <string.h>

#include <proto/exec.h>

#include LC_LIBDEFS_FILE
#include "memory.h"
#include "exec_intern.h"

#undef kprintf /* This can't be used in the code here */

extern void *LIBFUNCTABLE[];

extern struct Library * PrepareAROSSupportBase (struct ExecBase *);
extern struct Resident Exec_resident; /* Need this for lib_IdString */
extern void AROS_SLIB_ENTRY(CacheClearU,Exec)();
AROS_UFP1(void, Exec_TrapHandler,
	  AROS_UFPA(struct ExecBase *, SysBase, A6)
);
AROS_UFP1(void, Exec_TaskFinaliser,
	  AROS_UFPA(struct ExecBase *, SysBase, A6)
);

extern void AROS_SLIB_ENTRY(TrapHandler,Exec)();
extern void AROS_SLIB_ENTRY(TaskFinaliser,Exec)();

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

/*
    PrepareExecBase() will initialize the ExecBase to default values,
    and not add anything yet (except for the MemHeader).
*/
extern void *stderr;

struct ExecBase *PrepareExecBase(struct MemHeader *mh)
{
    ULONG   negsize = 0, i;
    VOID  **fp      = LIBFUNCTABLE;
    
    /*
       Basically this does not get anything useful, but I still need to have 
       SysBase defined here...
    */
    AROS_GET_SYSBASE
    
    /* Calculate the size of the vector table */
    while (*fp++ != (VOID *) -1) negsize += LIB_VECTSIZE;
    
    /* Align library base */
    negsize = AROS_ALIGN(negsize);
    
    /* Allocate memory for library base */
    SysBase = (struct ExecBase *)
	    ((UBYTE *)allocmem(mh, negsize + sizeof(struct ExecBase)) + negsize);

    /* Clear the library base */
    memset(SysBase, 0, sizeof(struct ExecBase));

    /* Setup function vectors */
    i  = 1;
    fp = LIBFUNCTABLE;
    
    while(*fp != (VOID *) -1)
    {
        /* Decrement vector pointer by one and install vector */
        __AROS_INITVEC(SysBase, i);
        if (*fp != NULL) __AROS_SETVECADDR(SysBase, i, *fp);

        /* Use next array entry */
        fp++; i++;
    }

    
    AROS_LC0(void, CacheClearU,
	struct ExecBase *, SysBase, 106, Exec);

    SysBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    SysBase->LibNode.lib_Node.ln_Pri  = -100;
    SysBase->LibNode.lib_Node.ln_Name = "exec.library";
    SysBase->LibNode.lib_IdString     = Exec_resident.rt_IdString;
    SysBase->LibNode.lib_Version      = VERSION_NUMBER;
    SysBase->LibNode.lib_Revision     = REVISION_NUMBER;
    SysBase->LibNode.lib_OpenCnt      = 1;
    SysBase->LibNode.lib_NegSize      = negsize;
    SysBase->LibNode.lib_PosSize      = sizeof(struct ExecBase);
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

    SysBase->SoftVer        = VERSION_NUMBER;

    SysBase->ColdCapture    = SysBase->CoolCapture
	                    = SysBase->WarmCapture
			    = NULL;

    SysBase->SysStkUpper    = (APTR)0xFFFFFFFF;
    SysBase->SysStkLower    = (APTR)0x00000000;

    SysBase->MaxLocMem      = (ULONG)mh->mh_Upper;

    SysBase->Quantum        = 4;

    SysBase->TaskTrapCode   = AROS_SLIB_ENTRY(TrapHandler,Exec);
    SysBase->TaskExceptCode = NULL;
    SysBase->TaskExitCode   = AROS_SLIB_ENTRY(TaskFinaliser,Exec);
    SysBase->TaskSigAlloc   = 0xFFFF;
    SysBase->TaskTrapAlloc  = 0;

    SysBase->VBlankFrequency = 50;
    SysBase->PowerSupplyFrequency = 1;
    
    SysBase->DebugAROSBase  = PrepareAROSSupportBase(SysBase);

    return SysBase;
}
