/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <clib/dos_protos.h>

int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    IPTR args[6]={ 0, 0, 0, 0, 0, 0 };

    rda=ReadArgs("PATH/M,ADD/S,SHOW/S,RESET/S,REMOVE/S,QUIET/S",args,NULL);
    if(rda!=NULL)
    {
	STRPTR *names=(STRPTR *)args[0];
	BPTR *cur, *next;
	struct CommandLineInterface *cli;
	cli=Cli();
	cur=(BPTR *)BADDR(cli->cli_CommandDir);
	while(cur)
	{
	    next=(BPTR *)cur[0];
	    UnLock(cur[1]);
	    FreeVec(cur);
	    cur=next;
	}
	cur=&cli->cli_CommandDir;
	while(*names!=NULL)
	{
	    next=(BPTR *)AllocVec(2*sizeof(BPTR),MEMF_ANY);
	    next[1]=Lock(*names,SHARED_LOCK);
	    if(!next[1])
	    {
		FreeVec(next);
		break;
	    }
	    cur[0]=MKBADDR(next);
	    cur=next;
	    if(!args[5])
		VPrintf("%s added.\n",(ULONG *)names);
	    names++;
	}
	cur[0]=0;
	FreeArgs(rda);
    }
    return 0;
}
