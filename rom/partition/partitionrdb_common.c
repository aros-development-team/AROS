/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: RDB helpers shared by partition.library and ptedit.library
*/

#include <proto/exec.h>
#include <proto/partition.h>

#include <exec/memory.h>

#include "partitionrdb.h"

struct LoadSegBlock *PartitionRDBFileSysBlock(struct FileSysNode *fn, ULONG block)
{
    return (struct LoadSegBlock *)((UBYTE *)fn->filesystem + (IPTR)block * fn->blockbytes);
}

ULONG calcChkSum(ULONG *ptr, ULONG size)
{
  ULONG i;
  ULONG sum=0;

  for (i=0;i<size;i++)
  {
    sum += AROS_BE2LONG(*ptr);
    ptr++;
  }
  return sum;
}

void PartitionRDBFreeHandleMem(struct PartitionHandle *ph)
{
    FreeMem(ph->data, ph->root->de.de_SizeBlock << 2);
    FreeVec(ph->ln.ln_Name);
    FreeMem(ph, sizeof(struct PartitionHandle));
}

/* Handle, name and a zeroed copy of the partition block */
struct PartitionHandle *PartitionRDBAllocHandle(struct PartitionHandle *root)
{
    struct PartitionHandle *ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);

    if (ph)
    {
        ph->ln.ln_Name = AllocVec(32, MEMF_PUBLIC | MEMF_CLEAR);
        if (ph->ln.ln_Name)
        {
            ph->data = AllocMem(root->de.de_SizeBlock << 2, MEMF_PUBLIC | MEMF_CLEAR);
            if (ph->data)
            {
                ph->root = root;
                ph->bd = root->bd;
                return ph;
            }
            FreeVec(ph->ln.ln_Name);
        }
        FreeMem(ph, sizeof(struct PartitionHandle));
    }
    return NULL;
}

void PartitionRDBFreeHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    ClosePartitionTable(ph);
    Remove(&ph->ln);
    PartitionRDBFreeHandleMem(ph);
}
