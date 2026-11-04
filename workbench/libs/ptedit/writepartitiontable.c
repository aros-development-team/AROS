/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include "ptedit_intern.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(LONG, WritePartitionTable,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
        struct Library *, PtEditBase, 5, PtEdit)

/*  FUNCTION
        Write a partition table; writing this partition table doesn't affect
        subpartition tables.

    INPUTS
        root - partition table to write

    RESULT
        0 for success; an error code otherwise

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
        const struct PTEditFunctionTable *handler = PtEditFind(root->table->type);

        if (handler && handler->writePartitionTable)
            return handler->writePartitionTable(PEBASE(PtEditBase)->pe_PartitionBase, root);
    }

    /* Can't write (no method) */
    return ERROR_ACTION_NOT_KNOWN;

    AROS_LIBFUNC_EXIT
}
