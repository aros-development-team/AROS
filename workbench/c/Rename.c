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

enum
{
    ARG_FROM = 0,
    ARG_TO,
    ARG_QUIET,
    NOOFARGS
};


#define MAX_PATH_LEN	512

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

    return retval;
}


int doRename(STRPTR *from, STRPTR to, BOOL quiet)
{
    struct AnchorPath *ap;

    char    pathName[MAX_PATH_LEN];
    STRPTR  fileStart;
    BOOL    destIsDir = FALSE;
    BOOL    destExists = FALSE;
    LONG    match;
    BPTR    lock;
    ULONG   i;



    /* First we check if the destination is a directory */
    lock = Lock(to, SHARED_LOCK);

    if(lock != NULL)
    {
 	struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

	destExists = TRUE;

	if(fib == NULL)
	{
	    UnLock(lock);
	    PrintFault(IoErr(), "Rename");

	    return RETURN_FAIL;
	}

	if(Examine(lock, fib))
	{
	    if(fib->fib_DirEntryType >= 0)
	    {
		destIsDir = TRUE;
	    }
	}
	else
	{
	    UnLock(lock);
	    PrintFault(IoErr(), "Rename");

	    return RETURN_FAIL;
	}

	UnLock(lock);
    }

    
    /* If the source is a pattern or multiple defined files, the destination
       'to' have to be a directory. */
    for(i = 0; from[i] != NULL; i++);
    
    if(i > 1 || ParsePattern(from[0], pathName, sizeof(pathName)) == 1)
    {
	if(!destExists)
	{
	    Printf("Destination directory \"%s\" does not exist.\n", to);
	    
	    return RETURN_FAIL;
	}	

	if(!destIsDir)
	{
	    Printf("Destination \"%s\" is not a directory.\n", to);

	    return RETURN_FAIL;
	}
    }
    else
    {
	if(destExists)
	{
	    Printf("Can't rename %s as %s because", from[0], to);
	    PrintFault(ERROR_OBJECT_EXISTS, "");

	    return RETURN_FAIL;
	}
    }

    /* Now either only one specific file should be renamed or the
       destination is really a directory. We can use the same routine
       for both cases! */
    
    ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath),
				       MEMF_ANY | MEMF_CLEAR);
    
    if(ap == NULL)
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	PrintFault(IoErr(), "Rename");
	
	return RETURN_FAIL;
    }
    
    strncpy(pathName, to, MAX_PATH_LEN);
    
    fileStart = pathName + strlen(pathName);
    
    for(i = 0; from[i] != NULL; i++)
    {
	for(match = MatchFirst(from[i], ap); match == 0; match = MatchNext(ap))
	{
	    BOOL ok;
	    BPTR olddir;
	    
	    /* Check for identical 'from' and 'to'? */
		
	    if(destIsDir)
	    {
		/* Clear former filename */
		*fileStart = 0;
		AddPart(pathName, ap->ap_Info.fib_FileName, MAX_PATH_LEN);
	    }
	    
	    if(!quiet)
	    {
		Printf("Renaming %s as %s\n", ap->ap_Info.fib_FileName, pathName);
	    }
	    
	    olddir = CurrentDir(ap->ap_Current->an_Lock);	    
	    ok = Rename(ap->ap_Info.fib_FileName, pathName);
	    CurrentDir(olddir);
	    
	    if(!ok)
	    {
		MatchEnd(ap);
		FreeVec(ap);
		PrintFault(IoErr(), "Rename");
		
		return RETURN_FAIL;
	    }
	}
	
	MatchEnd(ap);
    }
    
    FreeVec(ap);
    
    return RETURN_OK;
}
