/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1997/01/27 00:22:37  ldp
    Include proto instead of clib

    Revision 1.5  1996/10/24 15:32:25  aros
    Fixed some warnings about int/ptr conversions

    Revision 1.4  1996/09/17 16:43:01  digulla
    Use general startup code

    Revision 1.3  1996/09/13 17:52:11  digulla
    Use IPTR

    Revision 1.2  1996/08/01 17:40:45  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

int main (int argc, char ** argv)
{
    STRPTR args[2]={ "CON:", "S:Shell-Startup" };
    struct RDArgs *rda;
    BPTR lock, in, out, shell;
    STRPTR s1, s2, s3, buf;
    struct Process *process;
    LONG error=RETURN_ERROR;

    rda=ReadArgs("WINDOW,FROM",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	s1=s2=(STRPTR)args[1];
	while(*s2++)
	    ;
	buf=(STRPTR)AllocVec(6+2*(s2-s1),MEMF_ANY);
	if(buf!=NULL)
	{
	    CopyMem("FROM ",buf,5);
	    s3=buf+5;
	    s2=s1;
	    *s3++='\"';
	    while(*s1)
	    {
		if(*s1=='*'||*s1=='\"'||*s1=='\n')
		    *s3++='*';
		if(*s1=='\n')
		    *s3++='n';
		else
		    *s3++=*s1;
		s1++;
	    }
	    *s3++='\"';
	    *s3=0;

	    shell=LoadSeg("c:shell");
	    if(shell)
	    {
		out=Open(args[0],MODE_READWRITE);
		if(out)
		{
		    /* Clone output filehandle */
		    lock=DupLockFromFH(out);
		    if(lock)
		    {
			in=OpenFromLock(lock);
			if(!in)
			    UnLock(lock);
		    }else
			in=0;
		    if(in)
		    {
			struct TagItem tags[]=
			{
			    { NP_Arguments, 0 },
			    { NP_Input, 0 },
			    { NP_Output, 0 },
			    { NP_Error, 0 },
			    { NP_Seglist, 0 },
			    { NP_Cli, 1 },
			    { TAG_END, 0 }
			};
			tags[0].ti_Data=(IPTR)buf;
			tags[1].ti_Data=(IPTR)in;
			tags[2].ti_Data=(IPTR)out;
			tags[4].ti_Data=(IPTR)shell;
			process=CreateNewProc(tags);
			if(process!=NULL)
			{
			    out=in=shell=0;
			    error=0;
			}
			Close(in);
		    }
		    Close(out);
		}
		UnLoadSeg(shell);
	    }
	    FreeVec(buf);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"NewShell");
    return error;
}
