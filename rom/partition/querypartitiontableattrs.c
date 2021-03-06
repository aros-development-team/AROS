/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "partition_support.h"
#include "platform.h"

/* By default we have nothing here */
static const struct PartitionAttribute defaultPartitionTableAttrs[] =
{
    {TAG_DONE, 0}
};

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(const struct PartitionAttribute *, QueryPartitionTableAttrs,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, table, A1),

/*  LOCATION */
        struct Library *, PartitionBase, 17, Partition)

/*  FUNCTION
        Query partition table attributes.

    INPUTS
        ph - PartitionHandle of a partition table

    RESULT
        List of NULL-terminated ULONGs with attributes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (table->table)
    {
        struct PTFunctionTable *handler = table->table->handler;

        return handler->partitionTableAttrs;
    }

    return defaultPartitionTableAttrs;

    AROS_LIBFUNC_EXIT
}
