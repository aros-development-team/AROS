/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#include <asm/registers.h>

#define DEBUG 1
#include <aros/debug.h>

#define CANNOT_LOAD_SHELL	"Unable to load C:shell\n"
#define CANNOT_OPEN_CON		"Cannot open boot console\n"

extern void hidd_demo(struct ExecBase * SysBase);

int main(struct ExecBase * SysBase, struct Library * DOSBase)
{
	LONG            rc = RETURN_FAIL;


	BPTR sseq = Open("S:Startup-Sequence", FMF_READ);
	BPTR cis = Open("CON:20/20///Boot Shell/AUTO", FMF_READ);
	// Necessary! Don't remove because otherwise it carshes!
	hidd_demo(SysBase);

	if (cis) {
		struct TagItem tags[] =
		{
			{ SYS_Asynch,      TRUE       },
			{ SYS_Background,  FALSE      },
			{ SYS_Input,       (IPTR)cis  },
			{ SYS_Output,      (IPTR)NULL },
			{ SYS_Error,       (IPTR)NULL },
			{ SYS_ScriptInput, (IPTR)sseq },
			{ TAG_DONE,       0           }
		};

		rc = SystemTagList("", tags);
		if (rc != -1) {
			cis  = NULL;
			sseq = NULL;
		} else
			rc = RETURN_FAIL;
	} else {
		PutStr(CANNOT_OPEN_CON);
	}
	Close(cis);
	Close(sseq);
	

#if 0
D(bug("Entering endless loop here in %s\n",__FUNCTION__));
while (1) {
	struct Task * me = FindTask(NULL), * t;
	int i = 0;
	D(bug("IPR: 0x%x, IMR: 0x%x, SR=0x%x\n",RREG_L(IPR),RREG_L(IMR),SetSR(0x0,0x0)));
	Forbid();
	D(bug("This task %p has priority: %d, name: %s\n",
	      me,
	      me->tc_Node.ln_Pri,
	      me->tc_Node.ln_Name));
	D(bug("IDNestCount: %d\n",SysBase->IDNestCnt));
	while (i++ < 3) {
		t = (struct Task *)SysBase->TaskReady.lh_Head;
		while (NULL != t->tc_Node.ln_Succ) {
			D(bug("ready task %p with priority: %d, name: %s\n",
			      t,
			      t->tc_Node.ln_Pri,
			      t->tc_Node.ln_Name));
			t = (struct Task *)t->tc_Node.ln_Succ;
		}
		t = (struct Task *)SysBase->TaskWait.lh_Head;
		while (NULL != t->tc_Node.ln_Succ) {
			D(bug("waiting task %p with priority: %d, name: %s\n",
			      t,
			      t->tc_Node.ln_Pri,
			      t->tc_Node.ln_Name));
			t = (struct Task *)t->tc_Node.ln_Succ;
		}
	}
	Permit();
	Delay(5000);
}
#endif
	return rc;
}
