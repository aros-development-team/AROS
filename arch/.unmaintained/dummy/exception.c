/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <machine.h>

void exception(struct ExecBase *SysBase)
{
	struct Task *me=SysBase->ThisTask;
	ULONG sig;
	APTR data;
	BYTE idn;
	me->tc_Flags&=~TF_EXCEPT;
	idn=SysBase->IDNestCnt;
	SysBase->IDNestCnt=0;
	for(;;)
	{
		sig=me->tc_SigExcept&me->tc_SigRecvd;
		if(!sig)
			break;
		me->tc_SigExcept^=sig;
		me->tc_SigRecvd ^=sig;
		data=me->tc_ExceptData;
		Enable();
		sig=((ULONG(*)(ULONG,APTR,struct ExecBase*))me->tc_ExceptCode)
			(sig,data,SysBase);
		Disable();
		me->tc_SigExcept|=sig;
	}
	SysBase->IDNestCnt=idn;
}
