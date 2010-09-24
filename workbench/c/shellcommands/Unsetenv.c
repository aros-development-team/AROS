/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UnSetEnv CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        UnSetEnv

    SYNOPSIS

        NAME,SAVE/S

    LOCATION

        C:

    FUNCTION

        Unsets a global variable from the current shell. These variables can
        be accessed from any program executing at any time.

        These variables are usually not saved in the ENVARC: directory, hence they
        can only be used by programs during the current execution of the
        operating system. If set using SAVE argument, the variable is also saved
        in ENVARC: and can then be deleted there by UnSetEnv with the SAVE argument.

        If no parameter is specified, the current list of global variables
        is displayed.

    INPUTS

        NAME    - The name of the global variable to unset.

        SAVE    - Unset (delete) the variable also in ENVARC:

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        UnSetEnv EDITOR SAVE

            Any program that accesses the variable "EDITOR" (like More and
            MultiView) are able to find out the name of the text-editor the
            user would like to use, by examining the contents of the variable.
            This command would delete this variable in Ram (in ENV:) _and_ in
            ENVARC: (assuming it was already set there, for example by the use
            of the SAVE argument of SetEnv).

    BUGS

    SEE ALSO

        GetEnv, SetEnv

    INTERNALS

    HISTORY

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
#include <aros/shcommands.h>

AROS_SH2(UnSetEnv, 45.0,
AROS_SHA(STRPTR, , NAME,   , NULL),
AROS_SHA(IPTR  , , SAVE, /S, NULL))
{
    AROS_SHCOMMAND_INIT

    int     Return_Value;
    BOOL    Success;
    BPTR    lock;
    TEXT    progname[108];

    GetProgramName(progname, sizeof(progname));
    Return_Value = RETURN_OK;

    if (SHArg(NAME) != NULL)
    {
        /* Delete the global variable from the list.
         */
        Success = DeleteVar(SHArg(NAME),
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
