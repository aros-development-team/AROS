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

   AROS_LH2(LONG, GetPartitionTableAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),
   AROS_LHA(struct TagItem *,    taglist,       A2),

/*  LOCATION */
   struct Library *, PartitionBase, 13, Partition)

/*  FUNCTION
    get attributes of a partition table

    INPUTS
    ph      - PartitionHandle of the partition table
    taglist - list of attributes; unknown tags are ignored
        PTT_TYPE     - ULONG *           ; get partition table type
        PTT_MAXLEADIN     - LONG *
        PTT_RESERVED - ULONG *           ; get number of reserved blocks

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

        if (handler->getPartitionTableAttrs)
            return handler->getPartitionTableAttrs(PartitionBase, root, taglist);
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
