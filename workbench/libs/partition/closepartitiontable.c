/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/

/*****************************************************************************

    NAME */
   AROS_LH1(void, ClosePartitionTable,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
   struct Library *, PartitionBase, 8, Partition)

/*  FUNCTION
	close a partition table (and discard all changes)
	all partitions and subpartitions in root->list will be removed recursivly

    INPUTS
	root - partition table to close

    RESULT

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

	if (root->table)
	{
	struct PTFunctionTable *handler = root->table->handler;

		if (handler->ClosePartitionTable)
			handler->ClosePartitionTable(PartitionBase, root);
		FreeMem(root->table, sizeof(struct PartitionTableHandler));
		root->table = 0;
	}

	AROS_LIBFUNC_EXIT
}

