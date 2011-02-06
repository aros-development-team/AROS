/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: initkicktags.c

    Desc: Handle CoolCapture and KickTags (reset proof residents)
    Lang: english
*/

#include <aros/debug.h>
#include "exec_intern.h"

void InitKickTags(void)
{
    ULONG chk = (ULONG)SysBase->KickCheckSum;
    ULONG chkold = SumKickData();
    struct MemList *ml = (struct MemList*)SysBase->KickMemPtr;

    D(bug("coolcapture=%p kickmemptr=%p kicktagptr=%p kickchecksum=%08x\n",
	SysBase->CoolCapture, SysBase->KickMemPtr, SysBase->KickTagPtr, chk));

    if (SysBase->CoolCapture) {
	AROS_UFC1(void, SysBase->CoolCapture,
            AROS_UFCA(struct Library *, (struct Library *)SysBase, A6));
    }
	
    if (chkold != chk) {
    	D(bug("Kicktag checksum mismatch %08x!=%08x\n", chkold, chk));
    	SysBase->KickMemPtr = NULL;
    	SysBase->KickTagPtr = NULL;
    	SysBase->KickCheckSum = 0;
    	return;
    }
    	
    while (ml) { /* single linked! */
    	UWORD i;
    	for (i = 0; i < ml->ml_NumEntries; i++) {
    	    D(bug("KickMem at %x len %d\n", ml->ml_ME[i].me_Un.meu_Addr, ml->ml_ME[i].me_Length));
    	    if (!AllocAbs(ml->ml_ME[i].me_Length, ml->ml_ME[i].me_Un.meu_Addr)) {
		D(bug("KickMem allocation failed\n"));
		/* Should we free already allocated KickMem lists? */
 	    	return;
 	    }
    	}
    	ml = (struct MemList*)ml->ml_Node.ln_Succ;
    }

    if (SysBase->KickTagPtr) {
	D(bug("Initializing Residents\n"));
	InitResidentList(SysBase->KickTagPtr, ~0, 0);
	D(bug("Residents initialized\n"));
    }
}
    
