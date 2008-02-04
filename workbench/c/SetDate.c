/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SetDate CLI command
    Lang: English
*/

/******************
    NAME

        SetDate

    FORMAT

        SetDate (file | pattern) [(weekday)] [(date)] [(time)] [ALL]

    SYNOPSIS

        FILE/A,WEEKDAY,DATE,TIME,ALL/S

    LOCATION

        Sys:c

    FUNCTION

        Changes the the date and time of the creation or last change of a
        file or directory. With option ALL, it changes the date and time of
        all files and directories (and files and subdirectories to those)
	matching the specified pattern.
            You may use the output from Date as input to SetDate.
    
    INPUTS

        FILE     --  File (or pattern) to change the date of.

	WEEKDAY  --  Specification of the day of the date. This is locale
	             sensitive, and you may use standard keywords as
		     'Tomorrow' and 'Yesterday' (in the language used, of
		     course).

	DATE     --  A date described according to the locale specification
	             of the currently used language.

	TIME     --  Time string in localized format.

	ALL      --  Recurse through subdirectories.

    RESULT

        Standard DOS return codes

    NOTES

    EXAMPLE

        SetDate #? `Date` ALL

	Sets the date for all files and directories in the current directory
	and its subdirectories to the current date.

    BUGS

    SEE ALSO

        Date

    INTERNALS

    HISTORY

        26.12.99  SDuvan   implemented
*/

#include <dos/datetime.h>
#include <proto/dos.h>
#include <dos/rdargs.h>
#include <dos/dosasl.h>

enum { ARG_FILE = 0, ARG_WEEKDAY, ARG_DATE, ARG_TIME, ARG_ALL };

int __nocommandline;

int main(void)
{
    struct AnchorPath aPath;
    struct RDArgs  *rda;
    IPTR            args[5] = { NULL, NULL, NULL, NULL, FALSE };
    struct DateTime dt;
    LONG            error = 0;
    BPTR            oldCurDir;
    LONG            retval = RETURN_OK;
    BOOL            timeError = FALSE; /* Error in time/date specification? */

    rda = ReadArgs("FILE/A,WEEKDAY,DATE,TIME,ALL/S", args, NULL);

    if(rda == NULL)
    {
	PrintFault(IoErr(), "SetDate");
	return RETURN_FAIL;
    }

    /* Use the current time as default (if no DATE, TIME or WEEKDAY is
       defined) */
    DateStamp(&dt.dat_Stamp);

    dt.dat_Flags   = DTF_FUTURE;
    dt.dat_Format  = FORMAT_DEF;
    dt.dat_StrDate = (UBYTE *)args[ARG_DATE];
    dt.dat_StrTime = (UBYTE *)args[ARG_TIME];

    /* Change the defaults according to the user's specifications */
    if(StrToDate(&dt) == DOSTRUE)
    {
	dt.dat_StrDate = (UBYTE *)args[ARG_WEEKDAY];

	if(StrToDate(&dt) == DOSFALSE)
	    timeError = TRUE;
    }
    else
	timeError = TRUE;
   
    if(timeError)
    {
	PutStr("SetDate: Illegal DATE or TIME string\n");
	return RETURN_FAIL;
    }


    aPath.ap_Flags = (BOOL)args[ARG_ALL] ? APF_DOWILD : 0;
    aPath.ap_BreakBits = SIGBREAKF_CTRL_C;
    aPath.ap_Strlen = 0;

    /* Save the current dir */
    oldCurDir = CurrentDir(NULL);
    CurrentDir(oldCurDir);

    error = MatchFirst((STRPTR)args[ARG_FILE], &aPath);

    while(error == 0)
    {
	CurrentDir(aPath.ap_Current->an_Lock);

	// VPrintf("%s", (IPTR *)&aPath.ap_Info.fib_FileName);
	
	SetFileDate(aPath.ap_Info.fib_FileName, &dt.dat_Stamp);

	error = MatchNext(&aPath);
    }
    
    MatchEnd(&aPath);

    /* Restore the current dir */
    CurrentDir(oldCurDir);
    
    FreeArgs(rda);

    if(error != ERROR_NO_MORE_ENTRIES)
    {
	if(error == ERROR_BREAK)
	    retval = RETURN_WARN;
	else
	    retval = RETURN_FAIL;

	PrintFault(IoErr(), "SetDate");
    }

    return retval;
}	
