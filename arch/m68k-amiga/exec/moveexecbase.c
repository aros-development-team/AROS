/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Additional ExecBase manipulation code
    Lang:
*/

#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/execbase.h>

#include <proto/exec.h>

#include "exec_intern.h"

static void reloclist(struct List *l)
{
	struct Node *n;

	if (l->lh_Head->ln_Succ == NULL) {
		NEWLIST(l);
		return;
	}

	n = l->lh_Head;
	n->ln_Pred = (struct Node*)&l->lh_Head; 

	n = l->lh_TailPred;
	n->ln_Succ = (struct Node*)&l->lh_Tail;
}

/* Move execbase to better location, used by m68k-amiga port to move exec from
 * chip or slow ram to real fast ram if autoconfig detected any real fast boards
 * RTF_SINGLETASK run level.
 * Note that oldSysBase is NOT location where to copy from but location of SysBase
 * before reset (from where to copy reset proof pointers)
 */
struct ExecBase *PrepareExecBaseMove(struct ExecBase *oldSysBase)
{
    ULONG totalsize, i;
    struct ExecBase *oldsb = SysBase, *newsb;
	
    APTR ColdCapture = NULL, CoolCapture = NULL, WarmCapture = NULL;
    APTR KickMemPtr = NULL, KickTagPtr = NULL, KickCheckSum = NULL;

	if (oldSysBase) {
		ColdCapture = oldSysBase->ColdCapture;
    		CoolCapture = oldSysBase->CoolCapture;
    		WarmCapture = oldSysBase->WarmCapture;
    		KickMemPtr = oldSysBase->KickMemPtr; 
    		KickTagPtr = oldSysBase->KickTagPtr;
    		KickCheckSum = oldSysBase->KickCheckSum;
    	}

	Remove((struct Node*)oldsb);

	totalsize = oldsb->LibNode.lib_NegSize + oldsb->LibNode.lib_PosSize;
	newsb = (struct ExecBase *)((UBYTE *)AllocMem(totalsize, MEMF_KICK) + oldsb->LibNode.lib_NegSize);
	CopyMemQuick((UBYTE*)oldsb - oldsb->LibNode.lib_NegSize, (UBYTE*)newsb - oldsb->LibNode.lib_NegSize, totalsize);

	reloclist(&newsb->LibList);
	AddTail(&newsb->LibList, (struct Node*)newsb);

	reloclist(&newsb->MemList);
	reloclist(&newsb->ResourceList);
	reloclist(&newsb->DeviceList);
	reloclist(&newsb->IntrList);
	reloclist(&newsb->PortList);
	reloclist(&newsb->TaskReady);
	reloclist(&newsb->TaskWait);
	reloclist(&newsb->SemaphoreList);
	reloclist((struct List*)&newsb->ex_MemHandlers);
	reloclist(&newsb->TaskReady);
	for (i = 0; i < 5; i++) {
		reloclist(&newsb->SoftInts[i].sh_List);
    	}
	reloclist(&PrivExecBase(newsb)->ResetHandlers);
	reloclist((struct List*)&PrivExecBase(newsb)->AllocMemList);

	InitSemaphore(&PrivExecBase(newsb)->MemListSem);
	InitSemaphore(&PrivExecBase(newsb)->LowMemSem);

	SysBase = newsb;
	FreeMem((UBYTE*)oldsb - oldsb->LibNode.lib_NegSize, totalsize);

	if (oldSysBase) {
    		SysBase->ColdCapture = ColdCapture;
    		SysBase->CoolCapture = CoolCapture;
    		SysBase->WarmCapture = WarmCapture;
    		SysBase->KickMemPtr = KickMemPtr;
    		SysBase->KickTagPtr = KickTagPtr;
    		SysBase->KickCheckSum = KickCheckSum;
    	}

	SetSysBaseChkSum();

	return SysBase;
}
