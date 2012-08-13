/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Setenv CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Setenv

    SYNOPSIS

        NAME,SAVE/S,STRING/F

    LOCATION

        C:

    FUNCTION

        Sets a global variable from the current shell. These variables can
        be accessed from any program executing at any time.

        These variables are usually not saved in the ENVARC: directory, hence they
        can only be used by programs during the current execution of the
        operating system. When using SAVE argument, the variable is also saved
	in ENVARC:

        If no parameters are specified, the current list of global variables
        are displayed.

    INPUTS

        NAME    - The name of the global variable to set.

        SAVE    - Save the variable also in ENVARC:

        STRING  - The value of the global variable NAME.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Setenv EDITOR Ed

            Any program that accesses the variable "EDITOR" will be able to
            find out the name of the text-editor the user would like to use,
            by examining the contents of the variable.

    BUGS

    SEE ALSO

        Getenv, Unsetenv

    INTERNALS

    HISTORY

        30-Jul-1997     laguest     Initial inclusion into the AROS tree
        13-Aug-1997     srittau     Minor changes

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/memory.h>
#include <exec/types.h>

#include <utility/tagitem.h>
#include <stdio.h>

#include <aros/shcommands.h>

AROS_SH3(Setenv, 45.0,
AROS_SHA(STRPTR, ,NAME,     , NULL),
AROS_SHA(IPTR  , ,SAVE,   /S, NULL),
AROS_SHA(STRPTR, ,STRING, /F, NULL))
{
    AROS_SHCOMMAND_INIT

    int     Return_Value;
    BOOL    Success;
    BPTR    lock;
    TEXT    progname[108];
    

    GetProgramName(progname, sizeof(progname));
    Return_Value = RETURN_OK;
    
    if (SHArg(NAME) != NULL || SHArg(STRING) != NULL)
    {
        /* Make sure we get to here is either arguments are
         * provided on the command line.
         */
        if (SHArg(NAME) != NULL && SHArg(STRING) != NULL)
        {
            /* Add the new global variable to the list.
             */
	     Success = SetVar(SHArg(NAME),
	     	    	      SHArg(STRING),
			      -1,
			      SHArg(SAVE) ? GVF_GLOBAL_ONLY | GVF_SAVE_VAR : GVF_GLOBAL_ONLY);

    	     if (Success == FALSE)
	     {
	         UBYTE buf[FAULT_MAX+128];

	         Fault(-141, NULL, buf, sizeof(buf));
	         Printf(buf, SHArg(NAME));	                	
	     	 PrintFault(IoErr(), progname);

	     	 Return_Value = RETURN_ERROR;
	     }
        }
    }
    else
    {
        /* Display a list of global variables.
         */
	 
	lock = Lock("ENV:", ACCESS_READ);
	if (lock)
	{
	    struct FileInfoBlock *FIB = AllocDosObject(DOS_FIB, NULL);
   	
	    if (FIB)
	    {
		if(Examine(lock, FIB))
		{		
	            while(ExNext(lock, FIB))
	            {
	                /* don't show dirs */
		        if (FIB->fib_DirEntryType < 0)
	        	{	                    	
	                    PutStr(FIB->fib_FileName);
	                    PutStr("\n");
	                } 	                    				    	    
		    } 
		}
		FreeDosObject(DOS_FIB, FIB);
    	    }
	    UnLock(lock);
	}
	else
	{
	    PrintFault(IoErr(), progname);
	    
	    Return_Value = RETURN_FAIL;
	}
    }

    return Return_Value;

    AROS_SHCOMMAND_EXIT
} /* main */
