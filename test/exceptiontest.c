/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:39  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <aros/libcall.h>
#include <clib/exec_protos.h>
#include <exec/execbase.h>
#include <stdio.h>

ULONG s;

__AROS_LH2(ULONG,handler,__AROS_LA(ULONG,signals,D0),__AROS_LA(APTR,exceptData,A1)
,struct ExecBase *,SysBase,,)
{
    __AROS_FUNC_INIT
    exceptData=0;
    s=SetSignal(0,0);
    return signals;
    __AROS_FUNC_EXIT
}

int main(void)
{
    int s1,s2;
    APTR oldexc;

    s1=AllocSignal(-1);
    if(s1>=0)
    {
	printf("sig1: %d\n",s1);
	s2=AllocSignal(-1);
	if(s2>=0)
	{
	    printf("sig2: %d\n",s2);
	    oldexc=SysBase->ThisTask->tc_ExceptCode;
	    SysBase->ThisTask->tc_ExceptCode=&_handler;
	    SetExcept(1<<s2,1<<s2);
	    Signal(SysBase->ThisTask,(1<<s2)|(1<<s1));
	    SetExcept(0,1<<s2);
	    SysBase->ThisTask->tc_ExceptCode=oldexc;
	    printf("got: %08lx\n",s);
	    FreeSignal(s2);
	}
	FreeSignal(s1);
    }
    return 0;
}
