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


#define ARG_TEMPLATE    "FILE/M/A,ALL/S,QUIET/S,FORCE/S,FOLLOWLINKS/S"

enum 
{
    ARG_FILE = 0,
    ARG_ALL,
    ARG_QUIET,
    ARG_FORCE,
    ARG_FOLLOWLINKS,
    NOOFARGS
};


/* Maximum file path length */
#define MAX_PATH_LEN    2048

static const char version[] = "$VER: Delete 41.2 (6.1.2000)\n";
static char cmdname[] = "Delete";


int doDelete(struct AnchorPath *ap, STRPTR *files, BOOL all, BOOL quiet,
	     BOOL force, BOOL forcelinks);

int __nocommandline;

int main(void)
{
    struct RDArgs      *rda;
    struct AnchorPath  *ap;
    IPTR                args[NOOFARGS] = { (IPTR) NULL, FALSE, FALSE, FALSE, FALSE };
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
	    BOOL    followlinks = (BOOL)args[ARG_FOLLOWLINKS];

	    retval = doDelete(ap, files, all, quiet, force, followlinks);
	    
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

static inline BOOL isDirectory(struct AnchorPath *ap, BOOL followflag)
{
    BOOL isdir;

    if (ap->ap_Info.fib_DirEntryType == ST_SOFTLINK)
    {
        /* Softlink to dir/file - Default don't enter it, unless
           flag is set and the destination is really softlink.

           We have all lost some files due to braindead behaviour
           of the original delete regarding softlinks. - Piru
        */
        isdir = FALSE;

        if (followflag)
        {
            /* Okay flag set, figure out if this is a softlink to directory
            */
            BPTR lock;

            lock = Lock(ap->ap_Buf, ACCESS_READ);
            if (lock)
            {
                struct FileInfoBlock *fib;

                fib = AllocDosObject(DOS_FIB, NULL);
                if (fib)
                {
                    if (Examine(lock, fib))
                    {
                        /* Just extra sanity check so it can't be softlink
                           anymore (weird, fucked up, fs?).
                        */
                        isdir = (fib->fib_DirEntryType >= 0 &&
                                 fib->fib_DirEntryType != ST_SOFTLINK);
                    }
                    FreeDosObject(DOS_FIB, fib);
                }
                UnLock(lock);
            }
        }
    }
    else if (ap->ap_Info.fib_DirEntryType == ST_LINKDIR)
    {
        /* Hardlink to directory - Only follow it if flag set.

           It is debatable whether hardlinks should be followed by
           default. IMHO not. - Piru
        */
        isdir = followflag;
    }
    else
    {
        isdir = ap->ap_Info.fib_DirEntryType >= 0;
    }
    return isdir;
}

#define isDeletable(fib) (!((fib)->fib_Protection & FIBF_DELETE))

int doDelete(struct AnchorPath *ap, STRPTR *files, BOOL all, BOOL quiet,
	     BOOL force, BOOL forcelinks)
{
    LONG  match;
    int   i;
    char  name[MAX_PATH_LEN];
    BOOL  isfile = TRUE;
    BOOL  deleteit = FALSE;
    BOOL  deletedfile = FALSE;
    BOOL  firstmatch = TRUE;
    STRPTR file;

    if (!files)
    {
        return RETURN_OK;
    }

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
	    firstmatch = FALSE;

	    if (CheckSignal(SIGBREAKF_CTRL_C))
	    {
		MatchEnd(ap);
		PrintFault(ERROR_BREAK,"");

		return  RETURN_ERROR;
	    }

            if (deleteit)
            {
                /* Try to delete the file or directory */
                if (!DeleteFile(name))
                {
                    LONG ioerr = IoErr();
                    Printf("%s  Not Deleted", (ULONG)name);
                    PrintFault(ioerr, "");
                }
                else if (!quiet)
                {
                    Printf("%s  Deleted\n", (ULONG)name);
                }

                deletedfile = TRUE;
                deleteit = FALSE;
            }

	    /* If this is a directory, we enter it regardless if the ALL
	       switch is set. */
	    if (isDirectory(ap, forcelinks))
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
            /* Mark the entry for deletion */
            deleteit = TRUE;

	    /* Check permissions */
            if (!isDeletable(&ap->ap_Info))
            {
                /* Consider delete protected file/dir 'deleted' */
                deletedfile = TRUE;

                if (force)
                    SetProtection(ap->ap_Buf, 0);
                else
                {
                    Printf("%s  Not Deleted", (ULONG)ap->ap_Buf);
                    PrintFault(ERROR_DELETE_PROTECTED, "");
                    deleteit = FALSE;
                }
            }
            isfile = !isDirectory(ap, forcelinks);
            strcpy(name, ap->ap_Buf);
	}
        MatchEnd(ap);
        if (deleteit)
        {
            /* Try to delete the file or directory */
            if (!DeleteFile(name))
            {
                LONG ioerr = IoErr();
                Printf("%s  Not Deleted", (ULONG)name);
                PrintFault(ioerr, "");
            }
            else if (!quiet)
            {
                Printf("%s  Deleted\n", (ULONG)name);
            }

            deletedfile = TRUE;
            deleteit = FALSE;
        }
    }

    if (firstmatch && match &&
        match != ERROR_OBJECT_NOT_FOUND &&
        match != ERROR_NO_MORE_ENTRIES)
    {
        PrintFault(match, NULL);
        return RETURN_WARN;
    }

    if (!deletedfile)
    {
        PutStr("No file to delete\n");
        return RETURN_WARN;
    }
    return RETURN_OK;
}
