/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
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

    HISTORY

    01.05.2000  SDuvan  implemented

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
    int  retval = RETURN_OK;
    IPTR args[] = { NULL, NULL };
    struct RDArgs *rda;
    
    rda = ReadArgs("DRIVE/A,NAME/A", args, NULL);

    if(rda != NULL)
    {
	if(strchr((STRPTR)args[ARG_NAME], ':') == NULL)
	{
	    if(Relabel((STRPTR)args[ARG_DRIVE], (STRPTR)args[ARG_NAME]) ==
	       DOSFALSE)
	    {
		PrintFault(IoErr(), "Relabel");
		retval = RETURN_FAIL;
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
