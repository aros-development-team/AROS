/*
    (C) 1998-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Find the whereabouts of an executable file or directory
    Lang: English
*/


/* FUNCTION

   Format:    Which (command) [NORES] [RES] [ALL]
   Template:  FILE/A, NORES/S, RES/S, ALL/S

   Which finds a specific program or directory and prints its
   location if found. Resident programs are marked as RESIDENT
   if they are not internal in which case they are marked as
   INTERNAL.
       Which searches the resident list, the current directory
   the command paths and the C: directory/directories. If the
   item wasn't found, Which sets the condition flag to WARN (5)
   but does not print any error message.
       The option NORES will make Which not search through the
   resident list; specifying RES will only scan the resident list.
   The ALL switch corresponds to printing all occurencies of an
   item. This may lead to multiple listings of the same program,
   for instance if the current directory is C:.

   HISTORY  980902 SDuvan  implemented
            001111 SDuvan  rewrote most of the code and added 
                           correct path support */

/* NOTES: * Executable files in AROS currently haven't got the
            e-flag set, which makes Which unusable for now in
	    emulated mode.
 */

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


int main(int argc, char **argv)
{
    /* Array filled by ReadArgs() call */
    IPTR           args[ARG_COUNT] = {0, 0, 0, 0};

    struct RDArgs *rda;           /* ReadArgs standard struct */
    BOOL   found = FALSE;         /* Indicates whether we've found a file
				     or not -- used for ALL ReadArgs() tag. */
    int    error = RETURN_OK;     /* Error value to return */

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
    struct MsgPort *oldFST;           /* Temporary holder of old FileSysTask */
    BPTR            oldCurDir;        /* Temporary holder of old current dir */
    
    /* If FilePart(name) is not name itself, it can't be in the C: directory;
       or rather, it isn't in the C: directory or we found it in
       FindCommandinPath(). */
    if(FilePart(name) != name)
	return FALSE;
    
    oldCurDir = CurrentDir(NULL);        /* Just to save the old current dir... */
    //    oldFST    = GetFileSysTask();        /* ... and the filesystem task */
    
    while(((dp2 = GetDeviceProc("C:", dp)) != NULL) && (!found || checkAll))
    {
	//	SetFileSysTask(dp2->dvp_Port);
	CurrentDir(dp2->dvp_Lock);
	found |= CheckDirectory(name, fib);
	
	/* Is this a multi assign? */
	if(!(dp2->dvp_Flags & DVPF_ASSIGN))
	    break;
	
	dp = dp2;
    }
    
    //    SetFileSysTask(oldFST);
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
			printf("%s\n", pathName);
			found = TRUE;
		    }
		}
		else
		{
		    /* Directories are always printed */
		    printf("%s\n", pathName);
		    found = TRUE;
		}

		FreeVec(pathName);		/* Free memory holding the full path name */
	    }
	}
	
	UnLock(lock);
    }

    return found;
}


STRPTR GetFullPath(BPTR lock)
{
    UBYTE  *buf;             /* Pointer to the memory allocated for the string */
    ULONG   size;            /* Holder of the (growing) size of the string */

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
	    printf("INTERNAL %s\n", name);
	}
	else if(seg->seg_UC == CMD_DISABLED)
	{
	    printf("INTERNAL %s ;(DISABLED)\n", name);
	}
	else
	    printf("RES %s\n", name);
    }
    
    return found;
}

