/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include "ptedit_intern.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include "debug.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

        AROS_LH2(LONG, SetPartitionTableAttrs,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, root, A1),
        AROS_LHA(const struct TagItem *, taglist, A2),

/*  LOCATION */
        struct Library *, PtEditBase, 9, PtEdit)

/*  FUNCTION
        Set attributes of a partition table.

    INPUTS
        ph      - PartitionHandle of the partition table
        taglist - list of attributes; unknown tags are ignored

    RESULT

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

        if (handler && handler->setPartitionTableAttrs)
            return handler->setPartitionTableAttrs(PEBASE(PtEditBase)->pe_PartitionBase, root, taglist);
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
