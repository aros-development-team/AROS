/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include <exec/memory.h>
#include <proto/exec.h>
#include "partition_intern.h"
#include "partition_support.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

   AROS_LH1(LONG, OpenPartitionTable,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
   struct Library *, PartitionBase, 7, Partition)

/*  FUNCTION
	open a partition table

    INPUTS
	root - root partition

    RESULT
	0 for success; an error code otherwise.
	On success root->list will be filled with a list of PartitionHandle's.
	if one partition contains more subpartitions call OpenPartitionTable()
	on the PartitionHandle recursivly.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	21-02-02    first version

*****************************************************************************/
{
	AROS_LIBFUNC_INIT

	struct PTFunctionTable **pst;

	pst = PartitionSupport;
	while (pst[0])
	{
		if (pst[0]->CheckPartitionTable(PartitionBase, root))
		{
			root->table = AllocMem
				(	
					sizeof(struct PartitionTableHandler),
					MEMF_PUBLIC | MEMF_CLEAR
				);
			if (root->table)
			{
			LONG retval;

				root->table->type = pst[0]->type;
				root->table->handler = *pst;
				retval = pst[0]->OpenPartitionTable(PartitionBase, root);
				if (retval!=0)
				{
					FreeMem(root->table, sizeof(struct PartitionTableHandler));
					root->table = 0;
				}
				return retval;
			}
		}
		pst++;
	}
	return 1;
	AROS_LIBFUNC_EXIT
}
