/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include "ptedit_intern.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

        AROS_LH2(LONG, CreatePartitionTable,

/*  SYNOPSIS */
        AROS_LHA(struct PartitionHandle *, root,       A1),
        AROS_LHA(ULONG,                    type,       D1),

/*  LOCATION */
        struct Library *, PtEditBase, 6, PtEdit)

/*  FUNCTION
        Create a new partition table.

    INPUTS
        root - partition to create table in
        type - the type of the partition table to create

    RESULT
        0 on success; an error code otherwise

    NOTES
        After calling this function the state of the PartitionHandle will be
        the same as when calling OpenPartitionTable(). Therefore before
        closing the PartitionHandle you should call ClosePartitionTable().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG retval=1;

    if (root->table == NULL)
    {
        struct Library *PartitionBase = PEBASE(PtEditBase)->pe_PartitionBase;
        struct PartitionTableInfo **pst;

        /* The new table is read, closed and queried through partition.library */
        pst = ((struct PartitionBase *)PartitionBase)->tables;
        while (pst[0])
        {
            if (pst[0]->pti_Type == type)
            {
                const struct PTEditFunctionTable *handler = PtEditFind(type);

                if (handler && handler->createPartitionTable)
                {
                    root->table = AllocMem
                        (
                            sizeof(struct PartitionTableHandler),
                            MEMF_PUBLIC | MEMF_CLEAR
                        );
                    if (root->table)
                    {
                        root->table->type = type;
                        root->table->handler = (APTR)*pst;
                        retval = handler->createPartitionTable(PartitionBase, root);
                        if (retval != 0)
                        {
                            FreeMem(root->table, sizeof(struct PartitionTableHandler));
                            root->table = NULL;
                        }
                        return retval;
                    }
                }
                return 1;
            }
            pst++;
        }
    }
    return 1;
    AROS_LIBFUNC_EXIT
}
