/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(void, DeletePartition,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, ph, A1),

/*  LOCATION */
        struct Library *, PartitionBase, 12, Partition)

/*  FUNCTION
        Delete a partition along with its subpartitions.

    INPUTS
        ph - PartitionHandle to delete

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (ph->root)
    {
    struct PTFunctionTable *handler = ph->root->table->handler;

        if (handler->deletePartition)
            handler->deletePartition(PartitionBase, ph);
    }
    AROS_LIBFUNC_EXIT
}
