/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find the whereabouts of an executable file or directory
    Lang: English
*/

/******************************************************************************

    NAME

        Which

    SYNOPSIS

        FILE/A, NORES/S, RES/S, ALL/S

    LOCATION

        Workbench:C

    FUNCTION

        Find and print the location of a specific program or directory.
	Resident programs are marked as RESIDENT if they are not
	interal resident in which case they are marked as INTERNAL.

	Which searches the resident list, the current directory,
        the command paths and the C: assign. If the item was not
	found the condition flag is set to WARN but no error is
	printed.

    INPUTS

        FILE   --  the command/directory to search for
        NORES  --  don't include resident programs in the search
	RES    --  consider resident programs only
	ALL    --  find all locations of the FILE. This may cause the
                   printing of the same location several times, for
		   instance if the current directory is C: and the
		   FILE was found in C:

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

        Executable files in AROS currently haven't got the e-flag set,
	which makes Which unusable for now in emulated mode.

    HISTORY

        09.02.1998  SDuvan  --  implemented
        11.11.2000  SDuvan  --  rewrote most of the code and added 
	                        correct path support

******************************************************************************/
 

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <stdio.h>

#define  ARG_COUNT  4    /* Number of ReadArgs() arguments */

/* NOTE: For now, compatibility to the Amiga Which command is kept, but
         I think that the restriction to only executable files should be
         removed, especially considering soft links and such. */


/*
 * Check the resident list for the command 'name'.
 */
BOOL FindResidentCommand(STRPTR name);


/*
 * Check the paths for the command 'name'.
 */
BOOL FindCommandinPath(STRPTR name, BOOL checkAll, struct FileInfoBlock *fib);


/*
 * Check the C: multiassign for the command 'name'.
 */
BOOL FindCommandinC(STRPTR name, BOOL checkAll, struct FileInfoBlock *fib);


/*
 * Look in the current directory for the command 'name'.
 */
BOOL CheckDirectory(STRPTR name, struct FileInfoBlock *fib);


/*
 * Get a string that specifies the full path to a file or NULL if there
 * was not enough memory to allocate the string. This string should be
 * freed using FreeVec().
 */
STRPTR GetFullPath(BPTR lock);


int __nocommandline;

int main(void)
{
    /* Array filled by ReadArgs() call */
    IPTR           args[ARG_COUNT] = {0, 0, 0, 0};

    struct RDArgs *rda;           /* ReadArgs standard struct */
    BOOL   found = FALSE;         /* Indicates whether we've found a file
				     or not -- used for ALL ReadArgs() tag. */
    int    error = RETURN_WARN;     /* Error value to return */

    struct FileInfoBlock *fib;    /* Used in Examine(). Allocated at top level
				     to skip multiple calls to
				     AllocDosObject() / FreeDosObject */
    
    if((rda = ReadArgs("FILE/A,NORES/S,RES/S,ALL/S", args, NULL)) != NULL)
    {
	BOOL    noRes    = (BOOL)args[1];   /* Don't check resident commands */
	BOOL    resOnly  = (BOOL)args[2];   /* Check resident commands only */
	BOOL    checkAll = (BOOL)args[3];   /* Check for multiple occurances */

	STRPTR  commandName = (STRPTR)args[0];     /* Command to look for */

	fib = AllocDosObject(DOS_FIB, NULL);
	
	if(fib != NULL)
	{
	    if(!noRes)
	    {
		/* Check resident lists */
		found |= FindResidentCommand(commandName);
		kprintf("Resident list\n");
	    }

	    if(!found && !resOnly)
	    {
		/* Check all available paths */
		found |= FindCommandinPath(commandName, checkAll, fib);
		kprintf("Path\n");
	    }

	    if(!found && !resOnly)
	    {
		/* Check C: multiassign */
		found |= FindCommandinC(commandName, checkAll, fib);
		kprintf("C:\n");
	    }
	    if (found)
	    {
		error = RETURN_OK;
	    }
	
	    FreeDosObject(DOS_FIB, fib);
	}

	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), "Which");
	error = RETURN_FAIL;
    }
    
    return error;
}


/* NOTE: The filesystemtask stuff is only necessary for this to work
         correctly on AmigaOS. AROS doesn't use this concept. */
BOOL FindCommandinC(STRPTR name, BOOL checkAll, struct FileInfoBlock *fib)
{
    BOOL            found = FALSE;    /* Object found? */
    struct DevProc *dp = NULL, *dp2;  /* For GetDeviceProc() call */
    BPTR            oldCurDir;        /* Temporary holder of old current dir */
    // struct MsgPort *oldFST;        /* Temporary holder of old FileSysTask */
    
    /* If FilePart(name) is not name itself, it can't be in the C: directory;
       or rather, it isn't in the C: directory or we found it in
       FindCommandinPath(). */
    if(FilePart(name) != name)
	return FALSE;
    
    oldCurDir = CurrentDir(NULL);     /* Just to save the old current dir... */
    // oldFST    = GetFileSysTask();  /* ... and the filesystem task */
    
    while(((dp2 = GetDeviceProc("C:", dp)) != NULL) && (!found || checkAll))
    {
	// SetFileSysTask(dp2->dvp_Port);
	CurrentDir(dp2->dvp_Lock);
	found |= CheckDirectory(name, fib);
	
	/* Is this a multi assign? */
	if(!(dp2->dvp_Flags & DVPF_ASSIGN))
	    break;
	
	dp = dp2;
    }
    
    // SetFileSysTask(oldFST);
    CurrentDir(oldCurDir);
    FreeDeviceProc(dp);
    
    return found;
}


BOOL FindCommandinPath(STRPTR name, BOOL checkAll, struct FileInfoBlock *fib)
{
    BOOL  found;                /* Have we found the 'file' yet? */
    BPTR  oldCurDir;		/* Space to store the current dir */
    BPTR *paths;                /* Loop variable */

    struct CommandLineInterface *cli = Cli();
    
    /* Can this happen at all? */
    if(cli == NULL)
	return FALSE;

    /* Check the current directory */
    kprintf("Calling CheckDirectory()\n");
    found = CheckDirectory(name, fib);

    oldCurDir = CurrentDir(NULL);

    /* Check all paths */
    paths = (BPTR *)BADDR(cli->cli_CommandDir);

    while((!found || checkAll) && paths != NULL)
    {
	CurrentDir(paths[1]);

	kprintf("Calling CheckDirectory()\n");
	found |= CheckDirectory(name, fib);
	
	paths = (BPTR *)BADDR(paths[0]);    /* Go on with the next path */
    }

    CurrentDir(oldCurDir);

    return found;
}


BOOL CheckDirectory(STRPTR name, struct FileInfoBlock *fib)
{
    BPTR    lock;                     /* Lock on 'name' */
    BOOL    found = FALSE;            /* For return value purposes */
    STRPTR  pathName;

    lock = Lock(name, SHARED_LOCK);

    kprintf("Locked command %s\n", name);
    
    if(lock != NULL)
    {
	kprintf("Calling Examine()\n");

	if(Examine(lock, fib) == DOSTRUE)
	{
	    kprintf("Calling GetFullPath()\n");

	    pathName = GetFullPath(lock);

	    if(pathName != NULL)
	    {	    
		/* File or directory? */
		if(fib->fib_DirEntryType < 0)
		{
		    /* FIBF_EXECUTE is active low! */
		    if(!(fib->fib_Protection & FIBF_EXECUTE))
		    {
			Printf("%s\n", pathName);
			found = TRUE;
		    }
		}
		else
		{
		    /* Directories are always printed */
		    Printf("%s\n", pathName);
		    found = TRUE;
		}

		FreeVec(pathName); /* Free memory holding the full path name */
	    }
	}
	
	UnLock(lock);
    }

    return found;
}


STRPTR GetFullPath(BPTR lock)
{
    UBYTE  *buf;           /* Pointer to the memory allocated for the string */
    ULONG   size;          /* Holder of the (growing) size of the string */

    for(size = 512; ; size += 512)
    {
	buf = AllocVec(size, MEMF_ANY);

	if(buf == NULL)
	    break;

	if(NameFromLock(lock, buf, size))
	{
	    return (STRPTR)buf;
	}

	FreeVec(buf);
    }

    return NULL;
}


BOOL FindResidentCommand(STRPTR name)
{
    BOOL   found = FALSE;          /* For return value purposes */
    struct Segment *seg;           /* Holder of segment if 'name' is a
				      resident command */
        
    /* Look in both system and normal list. Or rather, in the normal list
       ONLY if it wasn't found in the system list. This is what the Amiga
       Which does thus not giving the whole picture if you have 'cmd' 
       resident while 'cmd' is an internal command also. However, if this
       is the case, you may never access it as the system list is searched
        first by the Shell? */
    if((seg = FindSegment(name, NULL, TRUE)) == NULL)
    {
	seg = FindSegment(name, NULL, FALSE);
    }
    
    if(seg != NULL)
    {
	found = TRUE;
	
	if(seg->seg_UC == CMD_INTERNAL)
	{
	    Printf("INTERNAL %s\n", name);
	}
	else if(seg->seg_UC == CMD_DISABLED)
	{
	    Printf("INTERNAL %s ;(DISABLED)\n", name);
	}
	else
	    Printf("RES %s\n", name);
    }
    
    return found;
}
