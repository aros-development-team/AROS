/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Rename CLI command.
*/

/*****************************************************************************

    NAME

	Rename

    SYNOPSIS

	Rename [{FROM}] <name> [TO|AS] <name> [QUIET]

	FROM/A/M,TO=AS/A,QUIET/S

    LOCATION

	Workbench/c

    FUNCTION

	Renames a directory or file. Rename can also act like the UNIX mv
	command, which moves a file or files to another location on disk.

    INPUTS

	FROM   --  The name(s) of the file(s) to rename or move. There may
		   be many files specified, this is used when moving files
		   into a new directory.

	TO|AS  --  The name which we wish to call the file.

	QUIET  --  Suppress any output from the command.

    RESULT

	Standard DOS error codes.

    NOTES

    EXAMPLE

	Rename letter1.doc letter2.doc letters

	    Moves letter1.doc and letter2.doc to the directory letters.

	Rename ram:a ram:b quiet
	Rename from ram:a to ram:b quiet
	Rename from=ram:a to=ram:b quiet

	    All versions, renames file "a" to "b" and does not output any
	    diagnostic information.

    BUGS

    SEE ALSO

    INTERNALS

	Rename() can only move a file to another directory, if and only if
	the to path has the from filename appended to the end.

	e.g.
	    Rename("ram:a", "ram:clipboards/a");
	not
	    Rename("ram:a", "ram:clipboards/");

******************************************************************************/

#define  DEBUG  0
#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <exec/types.h>

#include <stdio.h>
#include <string.h>

#define ARG_TEMPLATE	"FROM/A/M,TO=AS/A,QUIET/S"

#define APPNAME     	"Rename"

enum
{
    ARG_FROM = 0,
    ARG_TO,
    ARG_QUIET,
    NOOFARGS
};


#define MAX_PATH_LEN	2048

static const char version[] = "$VER: Rename 41.2 (23.11.2000)\n";


int doRename(STRPTR *from, STRPTR to, BOOL quiet);


int __nocommandline;

int main(void)
{
    struct RDArgs     *rda;

    IPTR  args[NOOFARGS] = { (IPTR) NULL, (IPTR) NULL, FALSE };

    int   retval = RETURN_FAIL;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    
    if(rda != NULL)
    {
	STRPTR *from  = (STRPTR *)args[ARG_FROM];
	STRPTR  to    = (STRPTR)args[ARG_TO];
	BOOL    quiet = (BOOL)args[ARG_QUIET];
	
	retval = doRename(from, to, quiet);
	
	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), APPNAME);
	retval = RETURN_ERROR;
    }

    return retval;
}


int doRename(STRPTR *from, STRPTR to, BOOL quiet)
{
#define ERROR(n) retval = (n); goto cleanup

	struct AnchorPath *ap;

	UBYTE   *pathName;
	STRPTR  fileStart;
	BOOL    destIsDir = FALSE;
	BOOL    destExists = FALSE;
	LONG    match;
	BPTR    tolock = NULL;
	ULONG   i;
	int     retval;
	LONG    ioerr = 0;
	UBYTE   itsWild;
	BOOL    isSingle;

	ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN + MAX_PATH_LEN,
	                                   MEMF_ANY | MEMF_CLEAR);
	if (!ap)
	{
		ioerr = IoErr();
		ERROR(RETURN_FAIL);
	}

	pathName = ((UBYTE *) (ap + 1)) + MAX_PATH_LEN;

	ap->ap_BreakBits = SIGBREAKF_CTRL_C;
	ap->ap_Flags     = APF_DOWILD;
	ap->ap_Strlen    = MAX_PATH_LEN;
	if (MatchFirst(from[0], ap) != 0)
	{
		ioerr = IoErr();
		if (ioerr == ERROR_OBJECT_NOT_FOUND)
		{
			Printf("Can't rename %s as %s because ", (ULONG)from[0], (ULONG)to);
		}
		ERROR(RETURN_FAIL);
	}
	itsWild = ap->ap_Flags & APF_ITSWILD;
	MatchEnd(ap);

	/* First we check if the destination is a directory */
	tolock = Lock(to, SHARED_LOCK);
	if (tolock)
	{
		struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
		LONG entrytype;

		destExists = TRUE;

		if (!fib)
		{
			PrintFault(IoErr(), APPNAME);

			ERROR(RETURN_FAIL);
		}

		i = Examine(tolock, fib);
		entrytype = fib->fib_EntryType;
		FreeDosObject(DOS_FIB, fib);

		if (i)
		{
			if (entrytype >= 0)
			{
				destIsDir = TRUE;
			}
		}
		else
		{
			PrintFault(IoErr(), APPNAME);

			ERROR(RETURN_FAIL);
		}
	}


	/* Check if dest is not a dir and src is pattern or multi */
	if (!destIsDir && (itsWild || from[1]))
	{
		Printf("Destination \"%s\" is not a directory.\n", (ULONG)to);
		ERROR(RETURN_FAIL);
	}


	/* Handle single file name change */
	isSingle =!destIsDir;
	/* 15th Jan 2004 bugfix, (destIsDir && ...) not (!destisDir && ...) ! - Piru */
	if (destIsDir && !from[1])
	{
		BPTR fromlock;

		fromlock = Lock(from[0], ACCESS_READ);
		if (fromlock)
		{
			isSingle = SameLock(fromlock, tolock) == LOCK_SAME;

			UnLock(fromlock);
		}

		/* 4th May 2003 bugfix: be quiet about single object renames - Piru */
		quiet = TRUE;
	}
	if (isSingle)
	{
		if (ParsePattern(from[0], pathName, MAX_PATH_LEN) > -1 &&
		    Rename(pathName, to))
		{
			ERROR(RETURN_OK);
		}

		ioerr = IoErr();
		Printf("Can't rename %s as %s because ", (ULONG)from[0], (ULONG)to);
		ERROR(RETURN_FAIL);
	}

	if (tolock)
	{
		if (!NameFromLock(tolock, pathName, MAX_PATH_LEN))
		{
			if (IoErr() == ERROR_LINE_TOO_LONG)
			{
				PrintFault(IoErr(), APPNAME);

				ERROR(RETURN_FAIL);
			}

			pathName[0] = '\0';
		}
	}

	if (!pathName[0])
	{
		stccpy(pathName, to, MAX_PATH_LEN);
	}


	/* Now either only one specific file should be renamed or the
	   destination is really a directory. We can use the same routine
	   for both cases! */

	fileStart = pathName + strlen(pathName);

	ap->ap_BreakBits = SIGBREAKF_CTRL_C;
	ap->ap_Strlen    = MAX_PATH_LEN;

	for (i = 0; from[i]; i++)
	{
		for (match = MatchFirst(from[i], ap); match == 0; match = MatchNext(ap))
		{
			/* Check for identical 'from' and 'to'? */

			if (destIsDir)
			{
				/* Clear former filename */
				*fileStart = '\0';
				if (!AddPart(pathName, FilePart(ap->ap_Buf), MAX_PATH_LEN))
				{
					MatchEnd(ap);

					PrintFault(ERROR_LINE_TOO_LONG, APPNAME);
					SetIoErr(ERROR_LINE_TOO_LONG);

					ERROR(RETURN_FAIL);
				}
			}

			if (!quiet)
			{
				Printf("Renaming %s as %s\n", (ULONG)ap->ap_Buf, (ULONG)pathName);
			}

			if (!Rename(ap->ap_Buf, pathName))
			{
				ioerr = IoErr();
				MatchEnd(ap);

				Printf("Can't rename %s as %s because ", (ULONG)ap->ap_Buf, (ULONG)pathName);
				ERROR(RETURN_FAIL);
			}
		}

		MatchEnd(ap);
	}

	if (ap->ap_FoundBreak & SIGBREAKF_CTRL_C)
	{
		PrintFault(ERROR_BREAK, NULL);

		retval = RETURN_WARN;
	}
	else
	{
		retval = RETURN_OK;
	}

cleanup:
	if (tolock)
	{
		UnLock(tolock);
	}

	if (ioerr)
	{
		//MatchEnd(ap);
		PrintFault(ioerr, NULL);
		SetIoErr(ioerr);
		//retval = ERROR_FAIL;
	}

	FreeVec(ap);

	return retval;
}
