/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:00  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <machine.h>

APTR sp;

void dispatch(struct ExecBase *SysBase)
{
	struct Task *me;
	struct List *list;
	me=SysBase->ThisTask;
	me->tc_SPReg=sp;
	if(me->tc_Flags&TF_SWITCH)
		me->tc_Switch();
	me->tc_IDNestCnt=SysBase->IDNestCnt;
	SysBase->IDNestCnt=-1;
	list=&SysBase->TaskReady;
	me=(struct Task *)list->lh_Head;
	list->lh_Head=me->tc_Node.ln_Succ;
	me->tc_Node.ln_Succ->ln_Pred=(struct Node *)list;
	SysBase->ThisTask=me;
	me->tc_State=TS_RUN;	
	SysBase->IDNestCnt=me->tc_IDNestCnt;
	if(me->tc_Flags&TF_LAUNCH)
		me->tc_Launch();
	sp=me->tc_SPReg;
	if(me->tc_Flags&TF_EXCEPT)
	{
		Disable();
		Exception();
		Enable();
	}
}
