/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:

    BUGS:
          Doesn't check whether an entry is already in the Path.
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <dos_commanderrors.h>

#include <aros/shcommands.h>

AROS_SH7(Path, 45.2,
AROS_SHA(STRPTR *, ,PATH,/M,NULL),
AROS_SHA(BOOL, ,ADD,/S,TRUE),
AROS_SHA(BOOL, ,SHOW,/S,FALSE),
AROS_SHA(BOOL, ,RESET,/S,FALSE),
AROS_SHA(BOOL, ,REMOVE,/S,FALSE),
AROS_SHA(BOOL, ,QUIET,/S,FALSE),
AROS_SHA(BOOL, ,HEAD,/S,FALSE))
{
    AROS_SHCOMMAND_INIT

    typedef struct
    {
        BPTR next;
        BPTR lock;
    } PathEntry;

    STRPTR *names=SHArg(PATH);
    PathEntry *cur;
    struct CommandLineInterface *cli = Cli();


    #define PE(x) ((PathEntry *)(BADDR(x)))

    if(!cli)
    {
        PrintFault(ERROR_SCRIPT_ONLY, "Path");

        return RETURN_ERROR;
    }

    cur = (PathEntry *)&cli->cli_CommandDir;

    if (names && *names)
    {
        BPTR next;

        if (!SHArg(HEAD) && !SHArg(RESET))
        {
       	    /* Search last entry */
            while (cur->next)
                cur = PE(cur->next);
        }

        next = cur->next;

        for (; *names; names++, cur = PE(cur->next))
	{
	    if
            (
                !(cur->next = MKBADDR(AllocVec(sizeof(PathEntry), MEMF_ANY)))
		||
                !(PE(cur->next)->lock = Lock(*names, SHARED_LOCK))
            )
            {
	        if (!PE(cur->next)->lock)
                    PrintFault(IoErr(), *names);

		FreeVec(PE(cur->next));

                break;
	    }
	}

        cur->next = next;
    }
    else
    {
        SHArg(SHOW) = TRUE && !SHArg(RESET);
    }

    if (SHArg(RESET))
    {
        while (cur->next)
        {
	    BPTR next = PE(cur->next)->next;

            UnLock(PE(cur->next)->lock);
            FreeVec(PE(cur->next));
            cur->next = next;
        }
    }

    if (SHArg(SHOW))
    {
        UBYTE Buffer[2048];
        IPTR parg[1];

        PutStr("Current Directory\n");

	for
        (
            cur = PE(cli->cli_CommandDir);
            cur;
	    cur = PE(cur->next)
        )
	{
	    NameFromLock (cur->lock, Buffer, sizeof (Buffer));

            parg[0] = (IPTR) Buffer;
	    VPrintf("%s\n", parg);
	}

        PutStr("C:\n");
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
