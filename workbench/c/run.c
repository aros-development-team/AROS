/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: run 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR in, out, shell;
    STRPTR s1, s2, buf;
    struct Process *process;
    ULONG num;
    LONG error=0;

    rda=ReadArgs("COMMAND/A/F",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	error=RETURN_ERROR;
	s1=s2=(STRPTR)args[0];
	while(*s2++)
	    ;
	buf=(STRPTR)AllocVec(8+s2-s1,MEMF_ANY);
	if(buf!=NULL)
	{
	    CopyMem("COMMAND ",buf,8);
	    CopyMem(s1,buf+8,s2-s1);
	    shell=LoadSeg("c:shell");
	    if(shell)
	    {
		in=Open("CONSOLE:",MODE_OLDFILE);
		if(in)
		{
		    out=Open("CONSOLE:",MODE_NEWFILE);
		    if(out)
		    {
			struct TagItem tags[]=
			{
			    { NP_Name, (IPTR)"Background task" },
			    { NP_Arguments, 0 },
			    { NP_Input, 0 },
			    { NP_Output, 0 },
			    { NP_Error, 0 },
			    { NP_Seglist, 0 },
			    { NP_Cli, 1 },
			    { TAG_END, 0 }
			};
			tags[1].ti_Data=(IPTR)buf;
			tags[2].ti_Data=(IPTR)in;
			tags[3].ti_Data=(IPTR)out;
			tags[5].ti_Data=(IPTR)shell;
			Forbid();
			process=CreateNewProc(tags);
			if(process!=NULL)
			{
			    num=process->pr_TaskNum;
			    out=in=shell=0;
			    error=0;
			}
			Permit();
			Close(out);
			if(process&&VPrintf("[CLI %ld]\n",&num)<0)
			   error=RETURN_ERROR;
		    }
		    Close(in);
		}
		UnLoadSeg(shell);
	    }
	    FreeVec(buf);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Run");
    return error;
}
