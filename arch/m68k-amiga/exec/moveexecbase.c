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
    ULONG totalsize, i, oldIntFlags;
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

	/* A little discussion on why MEMF_KICK is used.
	 *
	 * MEMF_CHIP is the 'core' memory on an Amiga, at address 0.
	 *    Always there, but super slow.
	 * MEMF_LOCAL is all of MEMF_CHIP, plus any non-AutoConfig expansion
	 *   (ie A500/600 Ranger at 0xc00000, A3000 and A4000 motherboard RAMs)
	 * MEMF_KICK is all of MEMF_LOCAL, plus any AutoConfig expansions
	 *   (ie Zorro II and Zorro III memory cards)
	 * MEMF_FAST *may* include other memories, which could be set up
	 *   by the SysBase->KickTags list, or CLI utilities
	 *
	 * arch/m68k-amiga/exec/boot.c has been specially prepared to handle
	 * MEMF_LOCAL or MEMF_KICK SysBase, so MEMF_KICK is the fastest
	 * superset.
	 */
	newsb = (struct ExecBase *)((UBYTE *)AllocMem(totalsize, MEMF_KICK) + oldsb->LibNode.lib_NegSize);
	CopyMem((UBYTE*)oldsb - oldsb->LibNode.lib_NegSize, (UBYTE*)newsb - oldsb->LibNode.lib_NegSize, totalsize);

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
	reloclist((struct List*)&PrivExecBase(newsb)->TaskStorageSlots);

	InitSemaphore(&PrivExecBase(newsb)->MemListSem);
	InitSemaphore(&PrivExecBase(newsb)->LowMemSem);

	SysBase = newsb;

	/* We need to temporarily disable MungWall, because
	 * it wasn't on when we were allocated.
	 */
	oldIntFlags = PrivExecBase(oldsb)->IntFlags;
	PrivExecBase(SysBase)->IntFlags &=  ~EXECF_MungWall;
	FreeMem((UBYTE*)oldsb - oldsb->LibNode.lib_NegSize, totalsize);
	PrivExecBase(SysBase)->IntFlags = oldIntFlags;

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
