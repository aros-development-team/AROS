#ifndef PARTITIONEDIT_INTERN_H
#define PARTITIONEDIT_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal definitions for partitionedit.library
*/

#include <exec/libraries.h>
#include <libraries/partition.h>

#include "partition_support.h"

/*
 * partitionedit.library creates, edits and writes the partition tables
 * opened by partition.library. Handles keep pointing at partition.library's
 * read-only PTFunctionTable, so closing and querying them works as usual;
 * the write side of each table type is looked up by its type.
 */

#define PARTITION_MIN_VERSION 4

struct PartitionEditBase_intern
{
    struct Library lib;
    struct Library *pe_PartitionBase;   /* partition.library */
};

#define PEBASE(x) ((struct PartitionEditBase_intern *)(x))

struct PTEditFunctionTable
{
    ULONG       type; /* Partition Table Type */
    LONG        (*writePartitionTable)     (struct Library *, struct PartitionHandle *);
    LONG        (*createPartitionTable)    (struct Library *, struct PartitionHandle *);
    struct PartitionHandle *(*addPartition)(struct Library *, struct PartitionHandle *, const struct TagItem *);
    void        (*deletePartition)         (struct Library *, struct PartitionHandle *);
    LONG        (*setPartitionTableAttrs)  (struct Library *, struct PartitionHandle *, const struct TagItem *);
    LONG        (*setPartitionAttrs)       (struct Library *, struct PartitionHandle *, const struct TagItem *);
    ULONG       (*destroyPartitionTable)   (struct Library *, struct PartitionHandle *);
};

extern const struct PTEditFunctionTable PartitionEditEBR;
extern const struct PTEditFunctionTable PartitionEditMBR;
extern const struct PTEditFunctionTable PartitionEditRDB;
extern const struct PTEditFunctionTable PartitionEditGPT;

const struct PTEditFunctionTable *PartitionEditFind(ULONG type);

/*
 * The handlers get partition.library's base as PartitionBase, so reads go
 * through it. Writes use PartitionWriteData(), the implementation of this
 * library's WritePartitionDataQ().
 */
LONG PartitionWriteData(struct PartitionHandle *ph, APTR buf, ULONG size, UQUAD block);
LONG PartitionWriteBlock(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG block, void *mem);

#define writeBlock PartitionWriteBlock

#endif /* PARTITIONEDIT_INTERN_H */
