/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: querypartitionattrs.c 38180 2011-04-12 12:32:14Z sonic $

*/

#include <exec/memory.h>
#include <proto/exec.h>

#include "partition_support.h"
#include "platform.h"
#include "fsloader.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

    AROS_LH1(LONG, AddBootFileSystem,

/*  SYNOPSIS */
    AROS_LHA(struct Node *, handle, A1),

/*  LOCATION */
    struct Library *, PartitionBase, 22, Partition)

/*  FUNCTION
    Adds the specified filesystem to the system list of bootable filesystems
    (actually FileSystem.resource).

    INPUTS
    handle - Filesystem handle obtained by FindFileSystemA()

    RESULT
    Zero if everything went okay or common dos.library-compliant error code.

    NOTES
    This function can be called during system startup before dos.library is
    available. In this case filesystem loading will be delayed until dos.library
    started up. Delayed loading will be handled automatically without any caller's
    intervention.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* If dos.library is available, load the filesystem immediately */
    if (((struct PartitionBase_intern *)PartitionBase)->pb_DOSBase)
    	return AddFS(PartitionBase, (struct FileSysHandle *)handle);    

    /* Otherwise we need to queue it to the FSLoader hook (if not already done) */
    if (!((struct FileSysHandle *)handle)->boot)
    {
    	struct BootFileSystem *bfs = AllocMem(sizeof(struct BootFileSystem), MEMF_ANY);

    	if (!bfs)
    	    return ERROR_NO_FREE_STORE;

	bfs->ln.ln_Name = handle->ln_Name;
	bfs->ln.ln_Pri  = handle->ln_Pri;
	bfs->handle = (struct FileSysHandle *)handle;

	/* This will prevent ClosePartitionTable() from deallocating the handle */
	((struct FileSysHandle *)handle)->boot = TRUE;

	Enqueue(&((struct PartitionBase_intern *)PartitionBase)->bootList, &bfs->ln);
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
