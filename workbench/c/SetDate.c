/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SetDate CLI command
    Lang: English
*/

/*************************************************************************

    NAME

        SetDate

    FORMAT

        SetDate (file | pattern) [(weekday)] [(date)] [(time)] [ALL]

    SYNOPSIS

        FILE/A,WEEKDAY,DATE,TIME,ALL/S

    LOCATION

        C:

    FUNCTION

        Changes the date and time of the creation or last change of a file or
        directory. With option ALL, it also changes the date and time of all
        files and subdirectories within directories matching the specified
        pattern. If either the date or time is unspecified, the current date
        or time is used.

    INPUTS

        FILE     --  File (or pattern) to change the date of.

    WEEKDAY  --  Specification of the day of the date. This is locale
                 sensitive, and you may use standard keywords such as
             'Tomorrow' and 'Yesterday' (in the language used, of
             course).

    DATE     --  A date in the format DD-MMM-YY.
                 MMM is either the number or the first 3 letters of the
                 month in English.

    TIME     --  Time string in the format HH:MM:SS or HH:MM.

    ALL      --  Recurse through subdirectories.

    RESULT

        Standard DOS return codes

    NOTES

    EXAMPLE

        SetDate #? ALL

    Sets the date for all files and directories in the current directory
    and its subdirectories to the current date.

    BUGS

    

    SEE ALSO

        Date

    INTERNALS

*************************************************************************/


#include <proto/exec.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <dos/rdargs.h>
#include <dos/dosasl.h>

#define MAX_PATH_LEN    512

const TEXT version[] = "$VER: SetDate 1.0 (10.11.2011)\n";

enum { ARG_FILE = 0, ARG_WEEKDAY, ARG_DATE, ARG_TIME, ARG_ALL };

int __nocommandline;

int main(void)
{
    struct AnchorPath *aPath;
    struct RDArgs  *rda;
    IPTR            args[5] = { (IPTR)NULL, (IPTR)NULL, (IPTR)NULL, (IPTR)NULL, (IPTR)FALSE };
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
    dt.dat_Format  = FORMAT_DOS;
    dt.dat_StrDate = (TEXT *)args[ARG_DATE];
    dt.dat_StrTime = (TEXT *)args[ARG_TIME];

    /* Change the defaults according to the user's specifications */
    if(StrToDate(&dt))
    {
        dt.dat_StrDate = (TEXT *)args[ARG_WEEKDAY];

        if(!StrToDate(&dt))
        {
            timeError = TRUE;
        }
    }
    else
    {
        timeError = TRUE;
    }
    
    if(timeError)
    {
        PutStr("SetDate: Illegal DATE or TIME string\n");
        return RETURN_FAIL;
    }

    aPath = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN,
                 MEMF_ANY | MEMF_CLEAR);

    if (aPath != NULL)
    {
        aPath->ap_Flags     = (BOOL)args[ARG_ALL] ? (APF_DODIR | APF_DOWILD) : APF_DOWILD;
        aPath->ap_BreakBits = SIGBREAKF_CTRL_C;
        aPath->ap_Strlen    = MAX_PATH_LEN;

        /* Save the current dir */
        oldCurDir = CurrentDir(BNULL);
        CurrentDir(oldCurDir);

        error = MatchFirst((STRPTR)args[ARG_FILE], aPath);

        while(error == 0)
        {
            CurrentDir(aPath->ap_Current->an_Lock);

            //VPrintf("%s\n", (IPTR *)&aPath->ap_Info.fib_FileName);
            if (  ((&aPath->ap_Info)->fib_DirEntryType >= 0)
               && !(aPath->ap_Flags & APF_DIDDIR) 
               && ((BOOL)args[ARG_ALL]))
            {
            	aPath->ap_Flags |= APF_DODIR;
            }
            SetFileDate(aPath->ap_Info.fib_FileName, &dt.dat_Stamp);

            error = MatchNext(aPath);
        }
        
        MatchEnd(aPath);

        /* Restore the current dir */
        CurrentDir(oldCurDir);
        
        FreeArgs(rda);
        FreeVec(aPath);
    }
    else
    {
        retval = RETURN_FAIL;
    }
    
    if(error != ERROR_NO_MORE_ENTRIES)
    {
        if(error == ERROR_BREAK)
        {
            retval = RETURN_WARN;
        }
        else
        {
            retval = RETURN_FAIL;
        }
        
        PrintFault(IoErr(), "SetDate");
    }
    
    return retval;
}    
