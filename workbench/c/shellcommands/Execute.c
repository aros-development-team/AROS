/*
    (C) 1995-2001 AROS - The Amiga Research OS
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

#include "shcommands.h"

AROS_SH1(Execute, 41.1,
AROS_SHA(STRPTR, ,NAME,/A,NULL))
{
    AROS_SHCOMMAND_INIT

    BPTR shell;
    STRPTR s1, s2, s3, buf;
    LONG error=0;

    s1=s2=SHArg(NAME);

    while(*s2++);

    buf=(STRPTR)AllocVec(6+2*(s2-s1),MEMF_ANY);
    if(buf!=NULL)
    {
	CopyMem("FROM ",buf,8);
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

    if(error)
	PrintFault(IoErr(), NULL);

    return error;

    AROS_SHCOMMAND_EXIT
}
