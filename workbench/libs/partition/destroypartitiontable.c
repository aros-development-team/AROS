/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

*/
#include <exec/memory.h>
#include <proto/exec.h>
#include "partition_support.h"
#include "platform.h"

#ifdef __AMIGAOS__
extern void Partition_ClosePartitionTable(struct PartitionHandle *, struct Library *);
#endif

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

   AROS_LH1(LONG, DestroyPartitionTable,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
   struct Library *, PartitionBase, 19, Partition)

/*  FUNCTION
	Destroy a partition table by immediatly overwriting table data on disk.

    INPUTS
	root - partition table to destroy

    RESULT
	0 on success; an error code otherwise

    NOTES
	After calling this function the state of the PartitionHandle will be the
	same as before calling OpenPartitionTable(). Therefore do not reference
	any children PartitionHandles anymore.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	25-08-02    first version

*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	LONG retval=1;

	if (root->table)
	{
	struct PTFunctionTable *handler = root->table->handler;

		if (handler->DestroyPartitionTable)
		{
			retval = handler->DestroyPartitionTable(PartitionBase, root);
			if (retval == 0)
				Partition_ClosePartitionTable(root, PartitionBase);
		}
	}
	return retval;
	AROS_LIBFUNC_EXIT
}
