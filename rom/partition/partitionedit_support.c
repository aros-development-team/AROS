/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: partitionedit.library support functions
*/

#include <proto/exec.h>

#include "partitionedit_intern.h"

static const struct PTEditFunctionTable * const PartitionEditSupport[] =
{
    &PartitionEditGPT,
    &PartitionEditMBR,
    &PartitionEditEBR,
    &PartitionEditRDB,
    NULL
};

/* Find the write side of a partition table type */
const struct PTEditFunctionTable *PartitionEditFind(ULONG type)
{
    const struct PTEditFunctionTable * const *pst;

    for (pst = PartitionEditSupport; *pst; pst++)
    {
        if ((*pst)->type == type)
            return *pst;
    }
    return NULL;
}

LONG PartitionWriteData(struct PartitionHandle *ph, APTR buf, ULONG size, UQUAD block)
{
    return PartitionDoIO(ph, buf, size, block, ph->bd->cmdwrite);
}

/* write a single block within partition ph */
LONG PartitionWriteBlock(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG block, void *mem)
{
    return PartitionWriteData(ph, mem, ph->de.de_SizeBlock << 2, block);
}
