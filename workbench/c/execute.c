/*
    (C) 1995-96 AROS - The Amiga Research OS
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

static const char version[] = "$VER: execute 41.1 (14.3.1997)\n";

int main(int argc, char **argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    BPTR shell;
    STRPTR s1, s2, s3, buf;
    LONG error=0;

    rda = ReadArgs("FILE/A",(IPTR *)args,NULL);

    if(rda != NULL)
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
	    else
                error = RETURN_FAIL;

	    FreeVec(buf);
	}
	else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            error = RETURN_FAIL;
        }

	FreeArgs(rda);
    }
    else
	error = RETURN_FAIL;

    if(error)
	PrintFault(IoErr(), NULL);
    return error;
}
