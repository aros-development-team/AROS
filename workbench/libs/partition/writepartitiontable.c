/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

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
    subpartition tables

    INPUTS
    root - partition table to write

    RESULT
    0 for success; an error code otherwise

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

        if (handler->writePartitionTable)
            return handler->writePartitionTable(PartitionBase, root);
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
