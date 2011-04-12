/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

   AROS_LH2(LONG, SetPartitionAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, ph,       A1),
   AROS_LHA(struct TagItem *,    taglist,       A2),

/*  LOCATION */
   struct Library *, PartitionBase, 16, Partition)

/*  FUNCTION
    set attributes of a partition

    INPUTS
    ph      - PartitionHandle
    taglist - list of attributes; unknown tags are ignored
        PT_DOSENVEC - set new DosEnvec values
        PT_TYPE     - change partition type (MBR-PC)
        PT_POSITION - move partition to another position (MBR-PC)
        PT_ACTIVE   - set partition active
        PT_NAME     - change name of partition (max 31 Bytes + NULL-byte)

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

        if (handler->setPartitionAttrs)
            return handler->setPartitionAttrs(PartitionBase, ph, taglist);
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
