/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <aros/libcall.h>
#include <clib/exec_protos.h>
#include <exec/execbase.h>
#include <stdio.h>

ULONG s;

/* Function prototype needed on Linux-M68K*/
AROS_LD2(ULONG,handler,
    AROS_LDA(ULONG,signals,D0),
    AROS_LDA(APTR,exceptData,A1),
    struct ExecBase *,SysBase,0,Test);

AROS_LH2(ULONG,handler,
    AROS_LHA(ULONG,signals,D0),
    AROS_LHA(APTR,exceptData,A1),
    struct ExecBase *,SysBase,0,Test)
{
    AROS_LIBFUNC_INIT
    exceptData=0;
    s=SetSignal(0,0);
    return signals;
    AROS_LIBFUNC_EXIT
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
	    SysBase->ThisTask->tc_ExceptCode=&AROS_SLIB_ENTRY(handler,Test);
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
