/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

#include <aros/shcommands.h>

AROS_SH5(Path, 41.1,
AROS_SHA(STRPTR *, ,PATH,/M,NULL),
AROS_SHA(BOOL, ,ADD,/S,FALSE),
AROS_SHA(BOOL, ,SHOW,/S,TRUE),
AROS_SHA(BOOL, ,RESET,/S,FALSE),
AROS_SHA(BOOL, ,QUIET,/S,FALSE))
{
    AROS_SHCOMMAND_INIT

    UBYTE Buffer[4096];
    IPTR parg[1];
    STRPTR *names=SHArg(PATH);
    BPTR *cur, *next;
    struct CommandLineInterface *cli;

    (void)Path_version;

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
	    if(!SHArg(QUIET))
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

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
