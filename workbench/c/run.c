/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Run

    SYNOPSIS

        COMMAND/F

    LOCATION

        Workbench:C

    FUNCTION

        Run a program, that is start a program as a background process.
        That means it doesn't take over the parent shell.

    INPUTS

        COMMAND  --  the program to run together with its arguments

    RESULT

    NOTES

        To make it possible to close the current shell, redirect the output
        using
 
             Run >NIL: program arguments

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define  DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: run 41.2 (5.1.2001)\n";

#define  ARG_TEMPLATE  "COMMAND/F"

enum
{
    ARG_COMMAND = 0,
    NOOFARGS
};


int main(int argc, char **argv)
{
    struct RDArgs  *rda;

    IPTR args[NOOFARGS] = { NULL };

    LONG    error = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if (rda != NULL)
    {
	LONG res;

	struct TagItem tags[] =
	{
	    { SYS_Input   , Open("CONSOLE:", MODE_OLDFILE) },
	    { SYS_Output  , Open("CONSOLE:", MODE_NEWFILE) },
	    { SYS_Asynch  , TRUE },
	    { NP_StackSize, Cli()->cli_DefaultStack },
	    { TAG_DONE    , 0 }
	};
	
	res = SystemTagList((STRPTR)args[ARG_COMMAND], tags);
	
	if (res == -1)
	{
	    error = RETURN_FAIL;
	}

	FreeArgs(rda);
    }
    else
    {
	error = RETURN_FAIL;
    }

    if(error != RETURN_OK)
    {
	PrintFault(IoErr(), "Run");
    }

    return error;
}

