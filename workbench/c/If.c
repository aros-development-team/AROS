/*
    (C) 2000 AROS - The Amiga Research OS
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

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <dos_commanderrors.h>

int main(int argc, char **argv)
{
    BOOL result = FALSE;

    IPTR args[] = { FALSE,
		    FALSE,
		    FALSE,
		    FALSE,
		    NULL,	/* Multiple */
		    NULL,
		    NULL,
		    NULL,
		    FALSE,
		    NULL };

    struct RDArgs *rda;
    struct UtilityBase *UtilityBase;


    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 41);

    if(UtilityBase == NULL)
	return RETURN_FAIL;
    
    rda = ReadArgs("NOT/S,WARN/S,ERROR/S,FAIL/S,,EQ/K,GT/K,GE/K,VAL/S,EXISTS/K",
		   (IPTR *)&args, NULL);
    
    
    if(rda != NULL)
    {
	struct CommandLineInterface *cli = Cli();
	
	if((cli != NULL) && (cli->cli_CurrentInput != cli->cli_StandardInput))
	{
	    STRPTR *argArray  = (STRPTR *)args[4];
	    STRPTR *argArray2 = argArray;
	    
	    if(args[4])		/* Multiple arguments... */
	    {
		int i = 0;
		
		while(argArray2++ != NULL)
		    i++;
		
		if(i != 2)	/* ...there must be exactly two of them. */
		{
		    FreeArgs(rda);
		    PrintFault(ERROR_NUMBER_OF_ARGUMENTS, "If");
		    return RETURN_ERROR;
		}
	    }
	    
	    if(args[1])	/* WARN */
	    {
		if(cli->cli_ReturnCode >= RETURN_WARN)
		    result = TRUE;
	    } else if(args[2])	/* ERROR */
	    {
		if(cli->cli_ReturnCode >= RETURN_ERROR)
		    result = TRUE;
	    } else if(args[3])	/* FAIL */
	    {
		if(cli->cli_ReturnCode >= RETURN_FAIL)
		    result = TRUE;
	    } else if(args[5] || args[6] || args[7])	/* EQ, GT, GE */
	    {
		if(args[8])
		{
		    LONG val1, val2;
		    
		    StrToLong(argArray[0], (LONG *)&val1);
		    StrToLong(argArray[1], (LONG *)&val2);
		    
		    if(args[5] && (val1 == val2))
			result = TRUE;
		    
		    if(args[6] && (val1 > val2))
			result = TRUE;
		    
		    if(args[7] && (val1 >= val2))
			result = TRUE;
		}
		else
		{
		    LONG res = Stricmp(argArray[0], argArray[1]);
		    
		    result = (args[5] && (res == 0)) ||
		             (args[6] && (res >  0)) ||
			     (args[7] && (res >= 0));
		}
	    }
	    else if(args[9])		/* EXISTS */
	    {
		BPTR lock = Lock((STRPTR)args[9], SHARED_LOCK);

		if(lock != NULL)
		    result = TRUE;
		
		UnLock(lock);
	    }
	    
	    if(args[0])		       /* NOT */
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
		    do {
			a = FGetC(Input());
		    } while (a != '\n' && a != ENDSTREAMCH);
		}

		if(!found)
		    PrintFault(ERROR_NO_MATCHING_ELSEENDIF, "If");
	    }
	}
	else
	{
	    Flush(Output());
	    PrintFault(ERROR_SCRIPT_ONLY, "If");
	}
    }

    FreeArgs(rda);

    CloseLibrary((struct Library *)UtilityBase);

    return RETURN_OK;
}
