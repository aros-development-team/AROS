/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Cd CLI command
    Lang: English
*/
/*****************************************************************************

    NAME

        CD

    SYNOPSIS

        DIR

    LOCATION

        Workbench:C/
	   
    FUNCTION

	Without argument it shows the name of the current directory.
	With argument it changes the current directory.
	
    INPUTS

	DIR -- path to change to current directory

    RESULT

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <aros/shcommands.h>

AROS_SH1(CD, 41.1,
AROS_SHA(STRPTR, ,DIR, ,NULL))
{
    AROS_SHCOMMAND_INIT

    BPTR dir,newdir;
    STRPTR buf;
    ULONG i;
    struct FileInfoBlock *fib;
    LONG error = 0;


    if (SHArg(DIR))
    {
	dir = Lock(SHArg(DIR), SHARED_LOCK);

	if (dir)
        {
	    fib = AllocDosObject(DOS_FIB, NULL);

	    if (fib != NULL)
	    {
		if (Examine(dir, fib))
		{
		    if (fib->fib_DirEntryType > 0)
		    {
			newdir = dir;
			dir = CurrentDir(newdir);

			for (i = 256;;i += 256)
			{
			    buf = AllocVec(i, MEMF_ANY);

			    if (buf == NULL)
			    {
				SetIoErr(ERROR_NO_FREE_STORE);
				error = RETURN_ERROR;
				break;
			    }

			    if (NameFromLock(newdir, buf, i))
			    {
				SetCurrentDirName(buf);
				FreeVec(buf);
			        break;
			    }

			    FreeVec(buf);

			    if (IoErr() != ERROR_LINE_TOO_LONG)
			    {
				error = RETURN_ERROR;
				break;
			    }
			}
		    }
		    else
		    {
			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			error = RETURN_ERROR;
		    }
		}
		else
		{
		    error = RETURN_ERROR;
		}

		FreeDosObject(DOS_FIB, fib);
	    }
	    else
	    {
		SetIoErr(ERROR_NO_FREE_STORE);
		error = RETURN_ERROR;
	    }

	    UnLock(dir);
	}
	else
	{
	    error = RETURN_ERROR;
	}
    }
    else
    {
	dir = CurrentDir(NULL);

	for(i = 256;;i += 256)
	{
	    buf = AllocVec(i, MEMF_ANY);

	    if (buf == NULL)
	    {
		SetIoErr(ERROR_NO_FREE_STORE);
		error = RETURN_ERROR;
		break;
	    }

	    if (NameFromLock(dir, buf, i))
	    {
		if (FPuts(Output(), buf) < 0 || FPuts(Output(), "\n") < 0)
		{
		    error = RETURN_ERROR;
		}
		
		FreeVec(buf);
		break;
	    }
	    
	    FreeVec(buf);

	    if (IoErr() != ERROR_LINE_TOO_LONG)
	    {
		error = RETURN_ERROR;
		break;
	    }
	}

	CurrentDir(dir);
    }
    
    if (error)
    {
	PrintFault(IoErr(), "CD");
    }

    return error;

    AROS_SHCOMMAND_EXIT
}
