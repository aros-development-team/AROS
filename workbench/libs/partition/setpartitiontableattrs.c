/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include "debug.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

   AROS_LH2(LONG, SetPartitionTableAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),
   AROS_LHA(struct TagItem *,    taglist,       A2),

/*  LOCATION */
   struct Library *, PartitionBase, 14, Partition)

/*  FUNCTION
    set attributes of a partition table

    INPUTS
    ph      - PartitionHandle of the partition table
    taglist - list of attributes; unknown tags are ignored

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

        if (handler->setPartitionTableAttrs)
            return handler->setPartitionTableAttrs(PartitionBase, root, taglist);
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
