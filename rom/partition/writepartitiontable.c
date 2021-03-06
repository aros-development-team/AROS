/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "partition_intern.h"
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(LONG, WritePartitionTable,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
        struct Library *, PartitionBase, 9, Partition)

/*  FUNCTION
        Write a partition table; writing this partition table doesn't affect
        subpartition tables.

    INPUTS
        root - partition table to write

    RESULT
        0 for success; an error code otherwise

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (root->table)
    {
        struct PTFunctionTable *handler = root->table->handler;

        if (handler->writePartitionTable)
            return handler->writePartitionTable(PartitionBase, root);
    }

    /* Can't write (no method) */
    return ERROR_ACTION_NOT_KNOWN;

    AROS_LIBFUNC_EXIT
}
