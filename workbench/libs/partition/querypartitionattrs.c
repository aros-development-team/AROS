/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

   AROS_LH1(struct PartitionAttribute *, QueryPartitionAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, table,       A1),

/*  LOCATION */
   struct Library *, PartitionBase, 18, Partition)

/*  FUNCTION
    query partition attributes

    INPUTS
    ph      - PartitionHandle to the table

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

        if (handler->queryPartitionAttrs)
            return handler->queryPartitionAttrs(PartitionBase);
    }
    return 0;
    AROS_LIBFUNC_EXIT
}
