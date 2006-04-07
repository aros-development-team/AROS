/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

   AROS_LH1(void, DeletePartition,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, ph,       A1),

/*  LOCATION */
   struct Library *, PartitionBase, 12, Partition)

/*  FUNCTION
    delete a partition with it's subpartitions

    INPUTS
    ph - PartitionHandle to delete

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

    if (ph->root)
    {
    struct PTFunctionTable *handler = ph->root->table->handler;

        if (handler->deletePartition)
            handler->deletePartition(PartitionBase, ph);
    }
    AROS_LIBFUNC_EXIT
}
