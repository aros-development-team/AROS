/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc: Sets up the ExecBase a bit. (Mostly clearing).
    Lang:
*/

#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <string.h>

#include <proto/exec.h>

#include "libdefs.h"
#include "memory.h"
#include "exec_intern.h"

#define EXEC_NUMVECT    135
#undef kprintf /* This can't be used in the code here */

extern void *ExecFunctions[];

//extern struct ExecBase *SysBase; /* Get rid of this */
extern struct Library * PrepareAROSSupportBase (void);
extern struct Resident Exec_resident; /* Need this for lib_IdString */
extern void AROS_SLIB_ENTRY(CacheClearU,Exec)();
extern void AROS_SLIB_ENTRY(TrapHandler,Exec)();
extern void AROS_SLIB_ENTRY(TaskFinaliser,Exec)();

static APTR allocmem(struct MemHeader *mh, ULONG size)
{
    UBYTE *ret;

    size = (size + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);
    ret = (UBYTE *)mh->mh_First;

    mh->mh_First = (struct MemChunk *)(ret + size);
    mh->mh_First->mc_Next = NULL;
    mh->mh_Free = mh->mh_First->mc_Bytes
	= ((struct MemChunk *)ret)->mc_Bytes - size;


    return ret;
}

/*
    PrepareExecBase() will initialize the ExecBase to default values,
    and not add anything yet (except for the MemHeader).
*/
struct ExecBase *PrepareExecBase(struct MemHeader *mh)
{
    ULONG neg, i;

    neg = AROS_ALIGN(LIB_VECTSIZE * EXEC_NUMVECT);

    SysBase = (struct ExecBase *)
	    ((UBYTE *)allocmem(mh, neg + sizeof(struct ExecBase)) + neg);

    /* Zero out the memory. Makes below a bit smaller. */
    memset(SysBase, 0, sizeof(struct ExecBase));

    for(i=1; i <= EXEC_NUMVECT; i++)
    {
	__AROS_INITVEC(SysBase, i);
	__AROS_SETVECADDR(SysBase, i, ExecFunctions[i-1]);
    }

    AROS_UFC1(void, AROS_SLIB_ENTRY(CacheClearU,Exec),
	AROS_UFCA(struct ExecBase *, SysBase, A6));

    SysBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    SysBase->LibNode.lib_Node.ln_Pri  = -100;
    SysBase->LibNode.lib_Node.ln_Name = "exec.library";
    SysBase->LibNode.lib_IdString = Exec_resident.rt_IdString;
    SysBase->LibNode.lib_Version = VERSION_NUMBER;
    SysBase->LibNode.lib_Revision = REVISION_NUMBER;
    SysBase->LibNode.lib_OpenCnt = 1;
    SysBase->LibNode.lib_NegSize = neg;
    SysBase->LibNode.lib_PosSize = sizeof(struct ExecBase);
    SysBase->LibNode.lib_Flags = 0;

    NEWLIST(&SysBase->MemList);
    SysBase->MemList.lh_Type = NT_MEMORY;
    ADDHEAD(&SysBase->MemList, &mh->mh_Node);
    NEWLIST(&SysBase->ResourceList);
    SysBase->ResourceList.lh_Type = NT_RESOURCE;
    NEWLIST(&SysBase->DeviceList);
    SysBase->DeviceList.lh_Type = NT_DEVICE;
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

    for(i=0; i<5; i++)
    {
	NEWLIST(&SysBase->SoftInts[i].sh_List);
	SysBase->SoftInts[i].sh_List.lh_Type = NT_INTERRUPT;
    }

    SysBase->SoftVer = VERSION_NUMBER;
    SysBase->ColdCapture = SysBase->CoolCapture
	= SysBase->WarmCapture = NULL;

    SysBase->SysStkUpper = (APTR)0xFFFFFFFF;
    SysBase->SysStkLower = (APTR)0x00000000;

    SysBase->MaxLocMem = (ULONG)mh->mh_Upper;

    SysBase->Quantum = 4;

    SysBase->TaskTrapCode = AROS_SLIB_ENTRY(TrapHandler,Exec);
    SysBase->TaskExceptCode = NULL;
    SysBase->TaskExitCode = AROS_SLIB_ENTRY(TaskFinaliser,Exec);
    SysBase->TaskSigAlloc = 0xFFFF;
    SysBase->TaskTrapAlloc = 0;

    SysBase->DebugAROSBase = PrepareAROSSupportBase();

    return SysBase;
}
