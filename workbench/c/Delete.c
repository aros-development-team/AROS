/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Delete CLI Command.
*/

/*****************************************************************************

    NAME

    Delete

    SYNOPSIS
    
    Delete { (name | pattern) } [ALL] [QUIET] [FORCE]

    LOCATION

	Workbench/c

    FUNCTION

    Deletes files and directories. You may delete several files and directories
    by listing them separately or by using wildcards. To abort a multiple
    delete, press CTRL-C. Delete will notify the user of which files it
    weren't able to delete.
        Delete cannot delete directories which are not empty unless the
    ALL option is used. To suppresss file and directory names from being
    printed while deleted use the QUIET option. If the 'd' protection bit
    is cleared for a file or directory, it may not be deleted unless the
    FORCE option is used.

    INPUTS

    FILE/M/A  --  files or directories to delete (may contain patterns)
    ALL/S     --  recursively delete dirctories
    QUIET/S   --  don't print which files/directories were deleted
    FORCE/S   --  delete files/directories even if they are protected from
                  deletion

    RESULT

    NOTES

    EXAMPLE

    Delete RAM:T/#? ALL FORCE

    Deletes all directories and files recursively in the directory RAM:T
    even if they are protected from deletion.


    BUGS

    SEE ALSO

    INTERNALS

    Shows a good usage of the pattern matching capabilities.

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>


#define ARG_TEMPLATE    "FILE/M/A,ALL/S,QUIET/S,FORCE/S"

enum 
{
    ARG_FILE = 0,
    ARG_ALL,
    ARG_QUIET,
    ARG_FORCE,
    NOOFARGS
};


/* Maximum file path length */
#define MAX_PATH_LEN    512

static const char version[] = "$VER: Delete 41.2 (6.1.2000)\n";
static char cmdname[] = "Delete";


int doDelete(struct AnchorPath *ap, STRPTR *files, BOOL all, BOOL quiet,
	     BOOL force);

int __nocommandline;

int main(void)
{
    struct RDArgs      *rda;
    struct AnchorPath  *ap;
    IPTR                args[NOOFARGS] = { (IPTR) NULL, FALSE, FALSE, FALSE };
    int	                retval         = RETURN_OK;

    ap = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN,
		  MEMF_ANY | MEMF_CLEAR);

    if (ap != NULL)
    {
	ap->ap_Strlen = MAX_PATH_LEN;
	
	rda = ReadArgs(ARG_TEMPLATE, args, NULL);

	if (rda != NULL)
	{
	    /* Convert arguments into (less complex) variables */
	    STRPTR *files = (STRPTR *)args[ARG_FILE];
	    BOOL    all = (BOOL)args[ARG_ALL];
	    BOOL    quiet = (BOOL)args[ARG_QUIET];
	    BOOL    force = (BOOL)args[ARG_FORCE];

	    retval = doDelete(ap, files, all, quiet, force);
	    
	    FreeArgs(rda);
	}
	else
	{
	    PrintFault(IoErr(), cmdname);
	    retval = RETURN_FAIL;
	}	
    }
    else
    {
	retval = RETURN_FAIL;
    }
    
    FreeVec(ap);
    
    return retval;
} /* main */


#define isDirectory(fib) ((fib)->fib_DirEntryType >= 0)

#define isDeletable(fib) (!((fib)->fib_Protection & FIBF_DELETE))

int doDelete(struct AnchorPath *ap, STRPTR *files, BOOL all, BOOL quiet,
	     BOOL force)
{
    LONG  match;
    int   i;
    BOOL  matched = FALSE;

    for (i = 0; files[i] != NULL; i++)
    {
	/* Index for last character in the current file name (pattern name) */
	int lastIndex = strlen(files[i]) - 1;

	if (files[i][lastIndex] == ':')
	{
	    struct DosList *dl = LockDosList(LDF_ALL | LDF_READ);
	    
	    if (FindDosEntry(dl, (CONST_STRPTR)files[i], LDF_ALL | LDF_READ))
	    {
		MatchEnd(ap);
		UnLockDosList(LDF_ALL | LDF_READ);
		
		Printf("%s is a device and cannot be deleted\n", files[i]);
		
		return RETURN_FAIL;
	    }
	    UnLockDosList(LDF_ALL | LDF_READ);
	}

	for (match = MatchFirst(files[i], ap); match == 0;
	     match = MatchNext(ap))
	{
	    matched = TRUE;

	    if (CheckSignal(SIGBREAKF_CTRL_C))
	    {
		MatchEnd(ap);
		PrintFault(ERROR_BREAK,"");

		return  RETURN_ERROR;
	    }

	    /* If this is a directory, we enter it regardless if the ALL
	       switch is set. */
	    if (isDirectory(&ap->ap_Info))
	    {
		/* This is a directory. It's important to check if we just left
		   the directory or is about to enter it. This is because we
		   cannot delete a directory until we have deleted all the
		   files in it. */
		if (ap->ap_Flags & APF_DIDDIR)
		{
		    /* If we get here, we are in ALL mode and have deleted
		       all the files inside the dir (if none were protected
		       and such). */

		    ap->ap_Flags &= ~APF_DIDDIR;

		    /* Now go on and delete the directory */
		}
		else
		{
		    /* We stumbled upon a directory */

		    if(all)
		    {
			ap->ap_Flags |= APF_DODIR;

			/* We don't want to delete a directory before deleting
			   possible files inside it. Thus, match next file */
			continue;
		    }

		    /* If all is not set, DeleteFile() will return an error
		       in case the directory was not empty. */
		}
	    }

	    /* Check permissions */
	    if (!force)
	    {
		if (!isDeletable(&ap->ap_Info))
		{
		    Printf("%s  Not Deleted", ap->ap_Buf);
		    PrintFault(ERROR_DELETE_PROTECTED, "");

		    if(!all)
		    {
			MatchEnd(ap);

			return RETURN_FAIL;
		    }
		}
	    }

	    /* Try to delete the file or directory */
	    if (!DeleteFile(ap->ap_Buf))
	    {
		Printf("%s  Not Deleted", ap->ap_Buf);
		PrintFault(IoErr(), "");
		
		/* If ALL is given as a parameter, we continue */
		if (!all)
		{
		    MatchEnd(ap);
		    
		    return RETURN_FAIL;
		}
	    }
	    
	    if (!quiet)
	    {
		Printf("%s  Deleted\n", ap->ap_Buf);
	    }
	}
    }

    if (!matched)
    {
	PutStr("No file to delete\n");
    }

    MatchEnd(ap);

    return RETURN_OK;
}
