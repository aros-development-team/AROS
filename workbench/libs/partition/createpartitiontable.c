/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include <exec/memory.h>
#include <proto/exec.h>
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

   AROS_LH2(LONG, CreatePartitionTable,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),
   AROS_LHA(ULONG,                    type,       D1),

/*  LOCATION */
   struct Library *, PartitionBase, 10, Partition)

/*  FUNCTION
    Create a new partition table. 

    INPUTS
    root - partition to create table in
    type - the type of the partition table to create

    RESULT
    0 on success; an error code otherwise

    NOTES
    After calling this function the state of the PartitionHandle will be the
    same as when calling OpenPartitionTable(). Therefore before closing the
    PartitionHandle you should call ClosePartitionTable().
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
    21-02-02    first version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG retval=1;

    if (root->table == NULL)
    {
    struct PTFunctionTable **pst;

        pst = PartitionSupport;
        while (pst[0])
        {
            if (pst[0]->type == type)
            {
                if (pst[0]->createPartitionTable)
                {
                    root->table = AllocMem
                        (
                            sizeof(struct PartitionTableHandler),
                            MEMF_PUBLIC | MEMF_CLEAR
                        );
                    if (root->table)
                    {
                        root->table->type = type;
                        root->table->handler = *pst;
                        retval = pst[0]->createPartitionTable(PartitionBase, root);
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
