/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include "partition_support.h"
#include "partition_intern.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(void, ClosePartitionTable,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
        struct Library *, PartitionBase, 8, Partition)

/*  FUNCTION
        Close a partition table (and discard all changes). All partitions
        and subpartitions in root->list will be removed recursively.

    INPUTS
        root - partition table to close

    RESULT

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

        if (handler->closePartitionTable)
            handler->closePartitionTable(PartitionBase, root);
        FreeMem(root->table, sizeof(struct PartitionTableHandler));
        root->table = 0;
    }

    AROS_LIBFUNC_EXIT
}
