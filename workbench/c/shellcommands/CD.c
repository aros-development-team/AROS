/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CD CLI command
    Lang: English
*/
/*****************************************************************************

    NAME

        CD

    SYNOPSIS

        DIR

    LOCATION

        C:
	   
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

static STRPTR GetLockName(BPTR lock, struct ExecBase *SysBase,
    struct DosLibrary *DOSBase);

AROS_SH1(CD, 41.2,
    AROS_SHA(STRPTR, , DIR, , NULL))
{
    AROS_SHCOMMAND_INIT

    BPTR dir,newdir;
    STRPTR buf;
    struct FileInfoBlock *fib;
    LONG return_code = 0, error = 0;

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

			buf = GetLockName(newdir, SysBase, DOSBase);
		        if (buf != NULL)
		        {
			    SetCurrentDirName(buf);
			    FreeVec(buf);
		        }
		    }
		    else
			error = ERROR_OBJECT_WRONG_TYPE;
		}
		else
		    error = IoErr();

		FreeDosObject(DOS_FIB, fib);
	    }
	    else
		error = IoErr();

	    UnLock(dir);
	}
	else
	    error = IoErr();
    }
    else
    {
	dir = CurrentDir(BNULL);

        buf = GetLockName(dir, SysBase, DOSBase);
        if (buf != NULL)
        {
            if (FPuts(Output(), buf) < 0 || FPuts(Output(), "\n") < 0)
                error = IoErr();
        }
        else
            error = IoErr();
	FreeVec(buf);
	CurrentDir(dir);
    }

    if (error != 0)
    {
        PrintFault(error, "CD");
        return_code = RETURN_ERROR;
    }

    return return_code;

    AROS_SHCOMMAND_EXIT
}


static STRPTR GetLockName(BPTR lock, struct ExecBase *SysBase, struct DosLibrary *DOSBase)
{
    LONG error = 0;
    STRPTR buf = NULL;
    ULONG i;

    for(i = 256; buf == NULL && error == 0; i += 256)
    {
        buf = AllocVec(i, MEMF_PUBLIC);
        if (buf != NULL)
        {
            if (!NameFromLock(lock, buf, i))
            {
                error = IoErr();
                if (error == ERROR_LINE_TOO_LONG)
                {
                    error = 0;
                    FreeVec(buf);
                    buf = NULL;
                }
            }
        }
        else
            error = IoErr();
    }

    if (error != 0)
        SetIoErr(error);
    return buf;
}
