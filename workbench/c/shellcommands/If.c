/*
    (C) 2000-2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************


    NAME

        If

    SYNOPSIS

        NOT/S,WARN/S,ERROR/S,FAIL/S,,EQ/K,GT/K,GE/K,VAL/S,EXISTS/K

    LOCATION

        Workbench:C

    FUNCTION

        Carry out all the commands in a block if a given conditional is true.
	(A block is a run of command lines ended with an Else or EndIf
	command.) For every If command there must be a corresponding EndIf.
	If the condition is false, command execution will skip to the
	corresponding Else of EndIf command.

    INPUTS

        NOT               --  Negates the value of the condition

	WARN              --  True if the previous return code was greater
	                      than or equal to 5.
	ERROR             --  True if the previous return code was greater
	                      than or equal to 10.
        FAIL              --  True if the previous return code was greater
	                      than or equal to 20.

	EQ, GE, GT        --  True if the first value is equal, greater than
	                      or equal respectively greater than the second.

        VAL               --  Indicate that the comparison should treat the
	                      strings as numerical values.

        EXISTS  <string>  --  True if the file or directory <string> exists.


    RESULT

    NOTES

        ERROR and FAIL will only be appropriate if the fail level of the
	script is set via FailAt (the standard fail level is 10 and if any
	return code exceeds or equals this value, the script will be aborted).

    EXAMPLE

        If 500 GT 200 VAL
	    echo "500 is greater than 200"
	Else
	    If EXISTS S:User-Startup
	        echo "User-Startup script found in S:"
		Execute S:User-Startup
	    EndIf
	EndIf

    BUGS

    SEE ALSO

        Else, EndIf, FailAt

    INTERNALS

    HISTORY

    10.01.2000  SDuvan   implemented

******************************************************************************/

#define  DEBUG  0
#include <aros/debug.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <dos_commanderrors.h>

#include <aros/shcommands.h>

AROS_SH10(If, 41.1,
AROS_SHA(BOOL, ,NOT,/S, FALSE),
AROS_SHA(BOOL, ,WARN,/S,FALSE),
AROS_SHA(BOOL, ,ERROR,/S, FALSE),
AROS_SHA(BOOL, ,FAIL,/S,FALSE),
AROS_SHA(STRPTR *, , , ,NULL),
AROS_SHA(STRPTR, ,EQ,/K,NULL),
AROS_SHA(STRPTR, ,GT,/K,NULL),
AROS_SHA(STRPTR, ,GE,/K,NULL),
AROS_SHA(BOOL, ,VAL,/S,FALSE),
AROS_SHA(STRPTR, ,EXISTS,/K,NULL))
{
    AROS_SHCOMMAND_INIT

    BOOL result = FALSE;

    struct UtilityBase *UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39);
    struct CommandLineInterface *cli = Cli();

    if (!UtilityBase)
        return RETURN_FAIL;


    if((cli != NULL) && (cli->cli_CurrentInput != cli->cli_StandardInput))
    {
	D(bug("Current input = %p, Standard input = %p\n",
	      cli->cli_CurrentInput, cli->cli_StandardInput));

	{

	    STRPTR *argArray  = SHArg( );
	    STRPTR *argArray2 = argArray;

	    if(SHArg( ))		/* Multiple arguments... */
	    {
		int i = 0;

		while(argArray2++ != NULL)
		    i++;

		if(i != 2)	/* ...there must be exactly two of them. */
		{
		    PrintFault(ERROR_TOO_MANY_ARGS, "If");
		    CloseLibrary((struct Library *)UtilityBase);
		    return RETURN_ERROR;
		}
	    }

	    if(SHArg(WARN))
	    {
		if(cli->cli_ReturnCode >= RETURN_WARN)
		    result = TRUE;
	    }
	    else if(SHArg(ERROR))
	    {
		if(cli->cli_ReturnCode >= RETURN_ERROR)
		    result = TRUE;
	    }
	    else if(SHArg(FAIL))
	    {
		if(cli->cli_ReturnCode >= RETURN_FAIL)
		    result = TRUE;
	    }
	    else if(SHArg(EQ) || SHArg(GT) || SHArg(GE))	/* EQ, GT, GE */
	    {
		if(SHArg(VAL))
		{
		    LONG val1, val2;

		    StrToLong(argArray[0], (LONG *)&val1);
		    StrToLong(argArray[1], (LONG *)&val2);

		    if(SHArg(EQ) && (val1 == val2))
			result = TRUE;

		    if(SHArg(GT) && (val1 > val2))
			result = TRUE;

		    if(SHArg(GE) && (val1 >= val2))
			result = TRUE;
		}
		else
		{
		    LONG res = Stricmp(argArray[0], argArray[1]);

		    result = (SHArg(EQ) && (res == 0)) ||
		             (SHArg(GT) && (res >  0)) ||
			     (SHArg(GE) && (res >= 0));
		}
	    }
	    else if(SHArg(EXISTS))
	    {
		BPTR lock = Lock(SHArg(EXISTS), SHARED_LOCK);

		if(lock != NULL)
		    result = TRUE;

		UnLock(lock);
	    }

	    if(SHArg(NOT))		       /* NOT */
		result = !result;


	    /* We have determined the result -- now we've got to act on it. */

	    if(!result)
	    {
		char a;
		char buffer[256];
		int  level = 1;	    /* If block level */
		BOOL found = FALSE; /* Have we found a matching Else or
				       EndIF? */

		SelectInput(cli->cli_CurrentInput);

		while(!found)
		{
		    LONG status;

		    status = ReadItem(buffer, sizeof(buffer), NULL);

		    if(status == ITEM_ERROR)
			break;

		    if(status == ITEM_NOTHING)
		    {
			if(FGetC(Input()) == ENDSTREAMCH)
			    break;
		    }

		    switch(FindArg("IF,ELSE,ENDIF", buffer))
		    {
		    case 0:
			level++;
			//			printf("Found If\n");
			break;

		    case 1:
			if(level == 0)
			    found = TRUE;
			break;

		    case 2:
			level--;

			if(level == 0)
			    found = TRUE;
			break;
		    }

		    /* Take care of long lines */
		    do
		    {
			a = FGetC(Input());
		    } while (a != '\n' && a != ENDSTREAMCH);
		}

		if(!found)
		    PrintFault(ERROR_NO_MATCHING_ELSEENDIF, "If");
	    }
	}
    }
    else
    {
	Flush(Output());
	PrintFault(ERROR_SCRIPT_ONLY, "If");
    }

    CloseLibrary((struct Library *)UtilityBase);

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
