/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/

/*****************************************************************************

    NAME */
   AROS_LH1(ULONG *, QueryPartitionTableAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, table,       A1),

/*  LOCATION */
   struct Library *, PartitionBase, 17, Partition)

/*  FUNCTION
	query partition table attributes

    INPUTS
	ph      - PartitionHandle of a partition table

    RESULT
	list of NULL-terminated ULONGs with attributes

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

	if (table->table)
	{
	struct PTFunctionTable *handler = table->table->handler;

		if (handler->QueryPartitionTableAttrs)
			return handler->QueryPartitionTableAttrs(PartitionBase);
	}
	return 0;
	AROS_LIBFUNC_EXIT
}
