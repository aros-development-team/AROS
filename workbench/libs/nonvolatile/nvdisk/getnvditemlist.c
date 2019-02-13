/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "nvdisk_intern.h"
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <libraries/nonvolatile.h>

#include <string.h>

void FreeAll(struct MinList *ml, struct Library *nvdBase);


AROS_LH1(struct MinList *, GetNVDItemList,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName, A0),

/*  LOCATION */

	struct Library *, nvdBase, 10, NVDisk)

/*  FUNCTION

    Get a list of the data stored in the nonvolatile memory by the
    'appName' application.

    INPUTS

    appName  --  the application the data of which to query about

    RESULT

    A list of the data items saved by application 'appName'.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    November 2000,  SDuvan  --  implemented

******************************************************************************/

     /* The size of a combined NVEntry and the name associated with it;
        32 is due to the maximum length of the 'itemName' string being 31 */
#define  NV_NODESIZE  (sizeof(struct NVEntry) + 32)

{
    AROS_LIBFUNC_INIT    

    BPTR            lock;
    BPTR            oldCDir;
    struct MinList *minList = (struct MinList *)AllocVec(sizeof(struct MinList),
							 MEMF_CLEAR);
    ULONG                 length;
    struct FileInfoBlock *fib;
    char                 *entries;

    if(minList == NULL)
	return NULL;

    fib = AllocDosObject(DOS_FIB, NULL);

    if(fib == NULL)
    {
	FreeVec(minList);
	return NULL;
    }

    oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
    
    NEWLIST((struct List *)minList);

    lock = Lock(appName, SHARED_LOCK);

    if(lock != BNULL)
    {
	if(Examine(lock, fib))
	{
	    while(ExNext(lock, fib))
	    {
	        // Data is stored as _files_
		if(fib->fib_DirEntryType < 0)
		{
		    struct NVEntry *entry = AllocVec(NV_NODESIZE,
						     MEMF_CLEAR);
		    
		    if(entry == NULL)
		    {
			FreeAll(minList, nvdBase);
			minList = NULL;
			break;
		    }

		    entry->nve_Name = (STRPTR)(((char *)entry) + sizeof(struct NVEntry));
			CopyMem(fib->fib_FileName, entry->nve_Name, 32);
			entry->nve_Name[31] = '0';
		    entry->nve_Size = fib->fib_Size;
		    entry->nve_Protection = fib->fib_Protection;
		    AddTail((struct List *)minList, (struct Node *)entry);
		}
	    }
	}
	
	UnLock(lock);
    }
    
    FreeDosObject(DOS_FIB, fib);
    
    CurrentDir(oldCDir);

    ListLength(minList, length);
    entries = AllocVec(sizeof(struct MinList) + length*NV_NODESIZE,
		       MEMF_ANY);


    /* We store the whole list in one memory block to make it possible to
       free the memory by calling nonvolatile.library/FreeNVData() */
    if(entries != NULL)
    {
	struct Node *node;	/* Temporary variable */
	ULONG        offset = sizeof(struct MinList); /* Offset into the memory
							 allocated */
	NEWLIST((struct List *)entries);

	for(node = GetHead((struct List *)minList); node != NULL;
	    node = GetSucc(node))
	{
	    /* 1. Copy the NVEntry plus string
	       2. Add the memory as a node in the list
	       3. Set the name to point to the copied memory */

	    CopyMem(node, entries + offset, NV_NODESIZE);
	    AddTail((struct List *)entries, (struct Node *)(entries + offset));
	    SetNodeName(entries + offset, entries + offset +
			sizeof(struct NVEntry));
	    offset += NV_NODESIZE;
	}
    }
    else
    {
	FreeAll(minList, nvdBase);
	return NULL;
    }

    FreeAll(minList, nvdBase);
    return (struct MinList *)entries;

    AROS_LIBFUNC_EXIT
} /* GetItemList */


/* Free all the nodes in a list together with the list structure itself. */
void FreeAll(struct MinList *ml, struct Library *nvdBase)
{
    struct Node *node;

    while((node = GetHead((struct List *)ml)) != NULL)
    {
	Remove(node);
	FreeVec(node);
    }

    FreeVec(ml);
}

#undef  NV_NODESIZE

