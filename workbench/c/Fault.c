/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Fault - Display an informative message about an error number.
    Lang: english
*/

/**************************************************************************

    NAME
	Fault

    FORMAT
	Fault <error number>

    SYNOPSIS
	NUMBERS/N/M

    LOCATION
	INTERNAL

    FUNCTION
	Fault prints the message corresponding with the error number
	supplied. Any number of error numbers can be given at once,
	but they must be separated by spaces.

    EXAMPLE

	1.SYS:> Fault 205
	Fault 205: object not found

	    This tells you that the error code 205 means that a disk
	    object could not be found.

**************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <stdio.h> /* for sprintf() */
#include <string.h>

#include <aros/debug.h>

#define ARG_TEMPLATE	"/N/M"
#define ARG_CODE	0
#define TOTAL_ARGS	1
#define DEBUG 1

static const char version[] = "$VER: Fault 41.1 (3.1.1998)";
static const char exthelp[] =
    "Fault : Display the meaning of a DOS error code\n"
    "\tNUMBERS/N/M      The error numbers you wish to query\n";

int main(int argc, char **argv)
{
    struct RDArgs *rd, *rda;
    IPTR args[TOTAL_ARGS] = { NULL };
    int error = 0;

    rda = AllocDosObject(DOS_RDARGS, NULL);
    if( rda != NULL )
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;

	rd = ReadArgs(ARG_TEMPLATE, (LONG *)args, rda);
	if( rd != NULL )
	{
    	    UBYTE buffer[128];
	    BPTR outStream = Output();
	    ULONG **theNum = (ULONG **)args[ARG_CODE], n;

	    /* Print the first part of the buffer */
	    CopyMem(buffer, "Fault ", 6);

	    D(bug("theNum = %p\n", theNum));
	    while( *theNum != NULL )
	    {
		D(bug("*theNum = %p", *theNum));
		D(bug(", **theNum = %p\n", **theNum));

		n = sprintf(&buffer[6], "%ld: ", **theNum);
		Fault(**theNum, NULL, &buffer[7+n], 128 - 7 - n);
		
		FPuts(outStream, buffer);
		theNum++;
	    }

	    FreeArgs(rd);
	} /* ReadArgs() ok */
	else
    	    error = IoErr();

	FreeDosObject(DOS_RDARGS, rda);
    }
    else
	error = IoErr();

    if( error != 0 )
    {
	PrintFault(error, "Fault");
	return RETURN_FAIL;
    }
    SetIoErr(0);
    return 0;
}
