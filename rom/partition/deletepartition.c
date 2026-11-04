/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include "partitionedit_intern.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(void, DeletePartition,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, ph, A1),

/*  LOCATION */
        struct Library *, PartitionEditBase, 8, PartitionEdit)

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
    const struct PTEditFunctionTable *handler = PartitionEditFind(ph->root->table->type);

        if (handler && handler->deletePartition)
            handler->deletePartition(PEBASE(PartitionEditBase)->pe_PartitionBase, ph);
    }
    AROS_LIBFUNC_EXIT
}
