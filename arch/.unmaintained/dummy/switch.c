/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <machine.h>

void _switch(struct ExecBase *SysBase)
{
	struct Task *me=SysBase->ThisTask;
	
	if(me->tc_State==TS_RUN&&!(me->tc_Flags&TF_EXCEPT))
	{
		me->tc_State=TS_READY;
		Enqueue(&SysBase->TaskReady,&me->tc_Node);
	}
	Dispatch();
}
