/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Avail CLI command
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

int main (int argc, char ** argv)
{
    IPTR args[4]={ 0, 0, 0, 0 };
    struct RDArgs *rda;
    LONG error=0;

    rda=ReadArgs("CHIP/S,FAST/S,TOTAL/S,FLUSH/S",args,NULL);
    if(rda!=NULL)
    {
	ULONG chip[4], fast[4], total[4];
	if(args[0]+args[1]+args[2]>1)
	{
	    FPuts(Output(),"Only one of CHIP, FAST or TOTAL allowed\n");
	    FreeArgs(rda);
	    return RETURN_FAIL;
	}else if(args[0])
	{
	    if(args[3])
		FreeVec(AllocVec(~0ul/2,MEMF_CHIP));
	    chip[0]=AvailMem(MEMF_CHIP);
	    if(VPrintf("%ld\n",chip)<0)
		error=RETURN_ERROR;
	}else if(args[1])
	{
	    if(args[3])
		FreeVec(AllocVec(~0ul/2,MEMF_FAST));
	    fast[0]=AvailMem(MEMF_FAST);
	    if(VPrintf("%ld\n",fast)<0)
		error=RETURN_ERROR;
	}else if(args[2])
	{
	    if(args[3])
		FreeVec(AllocVec(~0ul/2,MEMF_ANY));
	    total[0]=AvailMem(MEMF_ANY);
	    if(VPrintf("%ld\n",total)<0)
		error=RETURN_ERROR;
	}else
	{
	    Forbid();
	    if(args[3])
		FreeVec(AllocVec(~0ul/2,MEMF_ANY));
	    chip[0]=AvailMem(MEMF_CHIP);
	    chip[2]=AvailMem(MEMF_CHIP|MEMF_TOTAL);
	    chip[3]=AvailMem(MEMF_CHIP|MEMF_LARGEST);
	    chip[1]=chip[2]-chip[0];
	    fast[0]=AvailMem(MEMF_FAST);
	    fast[2]=AvailMem(MEMF_FAST|MEMF_TOTAL);
	    fast[3]=AvailMem(MEMF_FAST|MEMF_LARGEST);
	    fast[1]=fast[2]-fast[0];
	    total[0]=AvailMem(MEMF_ANY);
	    total[2]=AvailMem(MEMF_ANY|MEMF_TOTAL);
	    total[3]=AvailMem(MEMF_ANY|MEMF_LARGEST);
	    total[1]=total[2]-total[0];
	    Permit();

	    if(PutStr("Type  Available    In-Use   Maximum   Largest\n")<0||
	       VPrintf("chip %10.ld%10.ld%10.ld%10.ld\n",chip)<0||
	       VPrintf("fast %10.ld%10.ld%10.ld%10.ld\n",fast)<0||
	       VPrintf("total%10.ld%10.ld%10.ld%10.ld\n",total)<0)
		error=RETURN_ERROR;
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Avail");
    return error;
}
