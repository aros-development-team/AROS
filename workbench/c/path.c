/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:22:37  ldp
    Include proto instead of clib

    Revision 1.6  1996/10/04 17:09:44  digulla
    More readable way to access arguments

    Revision 1.5  1996/10/04 14:35:41  digulla
    Make "path x: ADD" work as expected. "path x:" works now, too

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
#include <proto/dos.h>

UBYTE Buffer[4096];

int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    IPTR args[6]={ 0, 0, 1, 0, 0, 0 };
#define ARG_Path	((STRPTR *)args[0])
#define ARG_Add 	((BOOL)args[1])
#define ARG_Show	((BOOL)args[2])
#define ARG_Reset	((BOOL)args[3])
#define ARG_Remove	((BOOL)args[4])
#define ARG_Quiet	((BOOL)args[5])

    IPTR parg[1];

    rda=ReadArgs("PATH/M,ADD/S,SHOW/S,RESET/S,REMOVE/S,QUIET/S",args,NULL);
    if(rda!=NULL)
    {
	STRPTR *names=ARG_Path;
	BPTR *cur, *next;
	struct CommandLineInterface *cli;
	cli=Cli();
	if (*names)
	{
	    /* Search last entry */
	    cur=&cli->cli_CommandDir;
	    while (cur[0])
		cur=(BPTR *)BADDR(cur[0]);

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
		if(!ARG_Quiet)
		    VPrintf("%s added.\n",(ULONG *)names);
		names++;
	    }
	    cur[0] = 0;
	}
	else
	{
	    BPTR l;

	    l = Lock ("", SHARED_LOCK);
	    if (l)
	    {
		NameFromLock (l, Buffer, sizeof (Buffer));
		parg[0] = (IPTR) Buffer;
		VPrintf ("Current Directory: %s\n", parg);
		UnLock (l);
	    }

	    cur=(BPTR *)BADDR(cli->cli_CommandDir);
	    while(cur)
	    {
		NameFromLock (cur[1], Buffer, sizeof (Buffer));
		parg[0] = (IPTR) Buffer;
		VPrintf ("%s\n", parg);
		cur=(BPTR *)BADDR(cur[0]);
	    }
	    VPrintf ("C:\n", NULL);
	}

	FreeArgs(rda);
    }
    return 0;
}
