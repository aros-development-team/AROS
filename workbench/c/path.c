/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

static const char version[] = "$VER: path 41.1 (14.3.1997)\n";

UBYTE Buffer[4096];

int __nocommandline;

int main (void)
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
