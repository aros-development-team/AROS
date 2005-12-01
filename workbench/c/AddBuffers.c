/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AddBuffers CLI command
    Lang: English
*/


/******************************************************************************

    NAME

        AddBuffers (drive) [(N)]

    SYNOPSIS

        DRIVE/A, BUFFERS/N

    LOCATION

        Workbench:C

    FUNCTION

        Add buffers to the list of available buffers for a specific
	drive. Adding buffers speeds disk access but has the drawback
	of using up system memory (512 bytes per buffer). Specifying
	a negative number subtracts buffers from the drive.
	    If only the DRIVE argument is specified, the number of 
	buffers for that drive are displayed without changing the buffer
	allocation.

    INPUTS

        DRIVE    --  the drive to alter the buffer allocation of
	BUFFERS  --  the number of buffers to add (or subtract in case of
	             a negative number) to a drive.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>


static const char version[] = "$VER: AddBuffers 41.1 (18.2.1997)\n";

#define ARG_TEMPLATE "DRIVE/A,BUFFERS/N"

enum
{
    ARG_DRIVE = 0,
    ARG_BUFFERS,
    NOOFARGS
};


int __nocommandline = 1;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR)NULL, (IPTR)0 };
    struct RDArgs *rda;

    int    result;
    int    error = RETURN_OK;
    ULONG  buffers = 0;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if (rda != NULL)
    {
	STRPTR  drive = (STRPTR)args[ARG_DRIVE];
	ULONG  *bufsptr = (ULONG *)args[ARG_BUFFERS];

	if (bufsptr != NULL)
	{
	    buffers = *bufsptr;
	}

	result = AddBuffers(drive, buffers);

	if (result == -1)
	{
	    Printf("%s has %ld buffers\n", drive, IoErr());
	}
	else if (result > 0)
	{
	    Printf("%s has %ld buffers\n", drive, (LONG)result);
	}
	else
	{
	    PrintFault(IoErr(), "AddBuffers");
	    error = RETURN_FAIL;
	}
	
	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), "AddBuffers");
	error = RETURN_FAIL;
    }
    
    return error;
}
