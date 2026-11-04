/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include "partitionedit_intern.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

       AROS_LH2(LONG, SetPartitionAttrs,

/*  SYNOPSIS */
       AROS_LHA(struct PartitionHandle *, ph,       A1),
       AROS_LHA(const struct TagItem *,   taglist,  A2),

/*  LOCATION */
       struct Library *, PartitionEditBase, 10, PartitionEdit)

/*  FUNCTION
        Set attributes of a partition

    INPUTS
        ph      - PartitionHandle
        taglist - list of attributes; unknown tags are ignored

    TAGS
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (ph->root)
    {
    const struct PTEditFunctionTable *handler = PartitionEditFind(ph->root->table->type);

        if (handler && handler->setPartitionAttrs)
            return handler->setPartitionAttrs(PEBASE(PartitionEditBase)->pe_PartitionBase, ph, taglist);
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
