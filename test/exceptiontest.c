/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1998/04/13 22:50:02  hkiel
    Include <proto/exec.h>

    Revision 1.4  1996/10/24 15:51:34  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/10/23 14:06:40  aros
    Use AROS_LHA() instead of AROS_LA()

    Use AROS_SLIB_ENTRY() to generate the name of a symbol in AROS_LH*()

    Revision 1.2  1996/08/01 17:41:39  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <aros/libcall.h>
#include <clib/exec_protos.h>
#include <exec/execbase.h>
#include <stdio.h>

ULONG s;

AROS_LH2(ULONG,handler,
    AROS_LHA(ULONG,signals,D0),
    AROS_LHA(APTR,exceptData,A1),
    struct ExecBase *,SysBase,,)
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
	    SysBase->ThisTask->tc_ExceptCode=&AROS_SLIB_ENTRY(handler,);
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
