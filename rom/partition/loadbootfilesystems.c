/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "partition_support.h"
#include "fsloader.h"

/*****************************************************************************

    NAME */
	AROS_LH0(LONG, LoadBootFileSystems,

/*  SYNOPSIS */

/*  LOCATION */
	struct Library *, PartitionBase, 24, Partition)

/*  FUNCTION
	Perform a deferred loading of boot filesystems.

    INPUTS
	None.

    RESULT
    	Zero if everything went okay or DOS error code in case of failure.

    NOTES
    	This function is actually private, it's called by dos.library during
    	initialization process. There's no sense to call it from within user
    	software.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct BootFileSystem *bfs, *bfs2;
    struct DosLibrary *DOSBase;
    ULONG lasterr = 0;

    PBASE(PartitionBase)->pb_DOSBase = OpenLibrary("dos.library", 36);
    DOSBase = (struct DosLibrary *)PBASE(PartitionBase)->pb_DOSBase;
    /* We should really have dos.library online now */
    D(bug("[LoadBootPartitions] DOSBase 0x%p\n", DOSBase));
    if (!DOSBase)
    	return ERROR_INVALID_RESIDENT_LIBRARY;

    ObtainSemaphore(&PBASE(PartitionBase)->bootSem);

    ForeachNodeSafe(&PBASE(PartitionBase)->bootList, bfs, bfs2)
    {
    	ULONG res;
	/*
	 * Unfortunately we have no way to process errors here.
	 * Well, let's hope that everything will be okay.
	 */
	D(bug("[LoadBootPartitions] Loading %s...\n", bfs->ln.ln_Name));

	res = AddFS(PartitionBase, bfs->handle);
	if (res)
	{
	    lasterr = res;
	}
	else
	{
	    /* A filesystem is loaded, remove it from the queue and free associated data. */
	    Remove(&bfs->ln);
	    bfs->handle->handler->freeFileSystem(bfs->handle);
	    FreeMem(bfs, sizeof(struct BootFileSystem));
	}
    }

    ReleaseSemaphore(&PBASE(PartitionBase)->bootSem);

    /*
     * We don't hold dos.library here because it may want to be expunged
     * (see dos_init.c and cliinit.c).
     */
    CloseLibrary(&DOSBase->dl_lib);
    return lasterr;

    AROS_LIBFUNC_EXIT
}
