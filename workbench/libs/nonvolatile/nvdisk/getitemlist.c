/*
    (C) 2000 AROS - The Amiga Research OS
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


AROS_LH1(struct MinList *, GetItemList,

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

******************************************************************************/

{
    AROS_LIBFUNC_INIT    

    BPTR            lock;
    BPTR            oldCDir;
    struct MinList *minList = (struct MinList *)AllocVec(sizeof(struct MinList),
							 MEMF_CLEAR);
    struct FileInfoBlock *fib;

    if(minList == NULL)
	return NULL;

    fib = AllocDosObject(DOS_FIB, NULL);

    if(fib == NULL)
	return NULL;

    oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
    
    NEWLIST((struct List *)minList);

    lock = Lock(appName, SHARED_LOCK);

    if(lock != NULL)
    {
	if(Examine(lock, fib))
	{
	    while(ExNext(lock, fib))
	    {
	        // Data is stored as _files_
		if(fib->fib_DirEntryType < 0)
		{
		    /* Maximum filename length = 31 */
		    struct NVEntry *entry = AllocVec(sizeof(struct NVEntry)+32,
						     MEMF_CLEAR);
		    
		    if(entry == NULL)
		    {
			FreeAll(minList, nvdBase);
			minList = NULL;
			break;
		    }

		    entry->nve_Name = (STRPTR)(((char *)entry) + sizeof(struct NVEntry));
		    strncpy(entry->nve_Name, fib->fib_FileName, 32);
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

    return minList;

    AROS_LIBFUNC_EXIT
} /* GetItemList */


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
