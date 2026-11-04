/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include "ptedit_intern.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH1(LONG, DestroyPartitionTable,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, root,       A1),

/*  LOCATION */
        struct Library *, PtEditBase, 11, PtEdit)

/*  FUNCTION
        Destroy a partition table by immediately overwriting table data on
        disk.

    INPUTS
        root - partition table to destroy

    RESULT
        0 on success; an error code otherwise

    NOTES
        After calling this function the state of the PartitionHandle will be
        the same as before calling OpenPartitionTable(). Therefore do not
        reference any child PartitionHandles any more.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG retval=1;

    if (root->table)
    {
    const struct PTEditFunctionTable *handler = PtEditFind(root->table->type);
    struct Library *PartitionBase = PEBASE(PtEditBase)->pe_PartitionBase;

        if (handler && handler->destroyPartitionTable)
        {
            retval = handler->destroyPartitionTable(PartitionBase, root);
            if (retval == 0)
                ClosePartitionTable(root);
        }
    }
    return retval;
    AROS_LIBFUNC_EXIT
}
