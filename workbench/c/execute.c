/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/09/17 16:43:00  digulla
    Use general startup code

    Revision 1.3  1996/09/13 17:52:10  digulla
    Use IPTR

    Revision 1.2  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR shell;
    STRPTR s1, s2, s3, buf;
    LONG error=0;

    rda=ReadArgs("FILE/A",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	s1=s2=(STRPTR)args[0];
	while(*s2++)
	    ;
	buf=(STRPTR)AllocVec(9+2*(s2-s1),MEMF_ANY);
	if(buf!=NULL)
	{
	    CopyMem("COMMAND ",buf,8);
	    s3=buf+8;
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
		RunCommand(shell,4096,buf,s3-buf);
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
