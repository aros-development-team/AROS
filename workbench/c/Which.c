/*
    (C) 1998-99 AROS - The Amiga Research OS
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

   HISTORY  980902 SDuvan  implemented */

/* NOTES: * Currently doesn't locate directories -- easy to fix in
            WriteifOK()

	  * Executable files in AROS currently haven't got the
            e-flag set, which makes Which unusable for now in
	    emulated mode.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <stdio.h>

#define  ARG_COUNT  4    /* Number of ReadArgs() arguments */
#define  BUFSIZE    1024 /* Maximum length of a complete pathname --
			    used in NameFromLock() */

/* NOTE: For now, compatibility to the Amiga which command is kept, but
         I think that the restriction to only executable files should be
         removed, especially considering soft links and such. */


BOOL FindResidentCmds(STRPTR name, BOOL dores);
BOOL FindCmdinPath(STRPTR name, struct FileInfoBlock *fib, BOOL doit);
BOOL FindCmdinC(STRPTR name, struct FileInfoBlock *fib, BOOL doit, BOOL doall);
BOOL WriteifOK(STRPTR name, struct FileInfoBlock *fib);


int main(int argc, char **argv)
{
    IPTR args[ARG_COUNT] = {0, 0, 0, 0}; /* Filled by ReadArgs() call */
    struct RDArgs *rda;           /* ReadArgs standard struct */
    BOOL   found;                 /* Indicates whether we've found a file
				     or not -- used for ALL ReadArgs() tag. */
    int    error = RETURN_OK;     /* Error value to return */
    struct FileInfoBlock *fib;    /* Used in Examine(). Allocated at top level
				     to skip multiple calls to
				     AllocDosObject() / FreeDosObject */
    
    if((rda = ReadArgs("FILE/A,NORES/S,RES/S,ALL/S", args, NULL)) != NULL)
    {
	fib = AllocDosObject(DOS_FIB, NULL);
	
	/* Is it in the resident segment list? */
	found = FindResidentCmds((STRPTR)args[0], !((BOOL)args[1]));
	/* Is it in the current dir? */
	found |= FindCmdinPath((STRPTR)args[0], fib,
			       ((BOOL)args[3] | !found) & !((BOOL)args[2]));
	/* Is it in some C: assign? */
	found |= FindCmdinC((STRPTR)args[0], fib,
			    ((BOOL)args[3] | !found) & !((BOOL)args[2]),
			    (BOOL)args[3]);
	
	FreeDosObject(DOS_FIB, fib);
	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(),"Which");
	error = RETURN_FAIL;
    }
    
    return error;
}


BOOL FindCmdinC(STRPTR name, struct FileInfoBlock *fib, BOOL doit,
		BOOL doall)
{
    BOOL            found = FALSE;    /* Object found? */
    struct DevProc *dp = NULL, *dp2;  /* For GetDeviceProc() call */
    struct MsgPort *oldfst;           /* Temporary holder of old FileSysTask */
    BPTR            olddir;           /* Temporary holder of old current dir */
    
    /* If FilePart(name) is not name itself, it can't be in the C: directory;
       or rather, it isn't in the C: directory or we found it in
       FindCmdinPath(). */
    if(doit == FALSE || (FilePart(name) != name))
	return FALSE;
    
    olddir = CurrentDir(NULL); /* Just to save the old current dir. */
    oldfst = GetFileSysTask();
    
    while(((dp2 = GetDeviceProc("C:", dp)) != NULL) && (!found || doall))
    {
	SetFileSysTask(dp2->dvp_Port);
	CurrentDir(dp2->dvp_Lock);
	found |= WriteifOK(name, fib);
	
	/* Is this a multi assign? */
	if(!(dp2->dvp_Flags & DVPF_ASSIGN))
	    break;
	
	dp = dp2;
    }
    
    SetFileSysTask(oldfst);
    CurrentDir(olddir);
    FreeDeviceProc(dp);
    
    return found;
}


BOOL FindCmdinPath(STRPTR name, struct FileInfoBlock *fib, BOOL doit)
{
    if(doit == FALSE)
	return FALSE;
    
    return WriteifOK(name, fib);
}


BOOL WriteifOK(STRPTR name, struct FileInfoBlock *fib)
{
    UBYTE *buf = AllocMem(BUFSIZE, MEMF_PUBLIC);   /* NameFromLock() buffer */
    BPTR   lock;                     /* Lock on 'name' */
    BOOL   found = FALSE;            /* For return value purposes */
    
    if(buf == NULL)
	return FALSE;

    lock = Lock(name, SHARED_LOCK);
    
    if(lock != NULL)
    {
	if(Examine(lock, fib) == DOSTRUE)
	{
	    NameFromLock(lock, buf, BUFSIZE - 1);
	    
	    /* File or directory? */
	    if(fib->fib_DirEntryType < 0)
	    {
		/* FIBF_EXECUTE is active low! */
		if(!(fib->fib_Protection & FIBF_EXECUTE))
		{
		    printf("%s\n", buf);
		    found = TRUE;
		}
	    }
	    else
	    {
		printf("%s\n", buf);
		found = TRUE;
	    }
	}
	
	UnLock(lock);
    }

    FreeMem(buf, BUFSIZE);
    return found;
}


BOOL FindResidentCmds(STRPTR name, BOOL dores)
{
    BOOL   found = FALSE;          /* For return value purposes */
    struct Segment *seg;           /* Holder of segment if 'name' is a
				      resident command */
    
    if(dores == FALSE)
	return FALSE;
    
    /* Look in both system and normal list. Or rather, in the normal list
       ONLY if it wasn't found in the system list. This is what the Amiga
       Which (apparently) does thus not giving the whole picture if you
       have 'cmd' resident while 'cmd' is an internal command also. However,
       if this is the case, you may never access it as the system list is
       searched first by the Shell? */
    if((seg = FindSegment(name, NULL, TRUE)) == NULL)
    {
	seg = FindSegment(name, NULL, FALSE);
    }
    
    if(seg != NULL)
    {
	found = TRUE;
	
	if(seg->seg_UC == CMD_INTERNAL)
	    printf("INTERNAL %s\n", name);
	else if(seg->seg_UC == CMD_DISABLED)
	{
	    printf("INTERNAL %s ;(DISABLED)\n", name);
	}
	else
	    printf("RES %s\n", name);
    }
    
    return found;
}
