/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <exec/execbase.h>
#include <clib/exec_protos.h>

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
