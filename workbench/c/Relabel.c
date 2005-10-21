/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************


    NAME

        Relabel

    SYNOPSIS

        DRIVE/A, NAME/A

    LOCATION

        Workbench:C

    FUNCTION

        Rename a volume

    INPUTS

        DRIVE   --  The volume to rename
	NAME    --  The new name

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

static const char version[] = "$VER: Relabel 41.1 (02.06.2000)\n";

enum { ARG_DRIVE = 0, ARG_NAME };

int __nocommandline;

int main(void)
{
    int  retval = RETURN_FAIL;
    IPTR args[] = { (IPTR) NULL, (IPTR) NULL };
    struct RDArgs *rda;
    
    rda = ReadArgs("DRIVE/A,NAME/A", args, NULL);

    if(rda != NULL)
    {
	if(strchr((STRPTR)args[ARG_NAME], ':') == NULL)
	{
		ULONG len = strlen((STRPTR)args[ARG_DRIVE]);
		UBYTE tmp[len + 1];
		struct DosList *dl;

		if (!len || ((STRPTR)args[ARG_DRIVE])[len - 1] != ':')
			goto invalid;

		len--;
		memcpy(tmp, (STRPTR)args[ARG_DRIVE], len);
		tmp[len] = '\0';

		dl = LockDosList(LDF_READ | LDF_DEVICES | LDF_VOLUMES);
		dl = FindDosEntry(dl, tmp, LDF_DEVICES | LDF_VOLUMES);
		UnLockDosList(LDF_READ | LDF_DEVICES | LDF_VOLUMES);

		if (dl)
		{
			if (Relabel((STRPTR)args[ARG_DRIVE], (STRPTR)args[ARG_NAME]))
			{
				retval = RETURN_OK;
			}
			else
			{
				PrintFault(IoErr(), NULL);
			}
		}
		else
		{
invalid:
			PutStr("Invalid device or volume name\n");
		}

	}
	else
	{
	    PutStr("':' is not a valid character in a volume name.\n");
	    retval = RETURN_FAIL;
	}
    }
    else
    {
	PrintFault(IoErr(), "Relabel");
	retval = RETURN_FAIL;
    }
    
    FreeArgs(rda);

    return retval;
}
