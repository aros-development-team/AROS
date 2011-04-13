#ifndef PARTITION_SUPPORT_H
#define PARTITION_SUPPORT_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <exec/types.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>

#include "partition_intern.h"

struct FileSysHandle
{
    struct Node ln;
    struct PartitionHandle *part;
    /* Handler's private data follows */
};

struct PTFunctionTable
{
    ULONG       type; /* Partition Table Type */
    CONST_STRPTR  name;
    LONG        (*checkPartitionTable)  (struct Library *, struct PartitionHandle *);
    LONG        (*openPartitionTable)   (struct Library *, struct PartitionHandle *);
    void        (*closePartitionTable)  (struct Library *, struct PartitionHandle *);
    LONG        (*writePartitionTable)  (struct Library *, struct PartitionHandle *);
    LONG        (*createPartitionTable) (struct Library *, struct PartitionHandle *);
    struct PartitionHandle *(*addPartition)(struct Library *, struct PartitionHandle *, struct TagItem *);
    void        (*deletePartition)      (struct Library *, struct PartitionHandle *);
    LONG        (*getPartitionTableAttrs)(struct Library *, struct PartitionHandle *, struct TagItem *);
    LONG        (*setPartitionTableAttrs)(struct Library *, struct PartitionHandle *, struct TagItem *);
    LONG        (*getPartitionAttrs)        (struct Library *, struct PartitionHandle *, struct TagItem *);
    LONG        (*setPartitionAttrs)        (struct Library *, struct PartitionHandle *, struct TagItem *);
    struct PartitionAttribute * (*queryPartitionTableAttrs)(struct Library *);
    struct PartitionAttribute * (*queryPartitionAttrs)  (struct Library *);
    ULONG    	(*destroyPartitionTable) (struct Library *, struct PartitionHandle *);
    struct Node *(*findFileSystem)	 (struct Library *, struct PartitionHandle *, struct TagItem *);
    BPTR	(*loadFileSystem)	 (struct PartitionBase_intern *, struct FileSysHandle *);
};

extern const struct PTFunctionTable * const PartitionSupport[];

extern const struct PTFunctionTable PartitionEBR;
extern const struct PTFunctionTable PartitionMBR;
extern const struct PTFunctionTable PartitionRDB;

LONG PartitionGetGeometry(struct Library *, struct IOExtTD *, struct DriveGeometry *);
void PartitionNsdCheck(struct Library *, struct PartitionHandle *);
ULONG getStartBlock(struct PartitionHandle *);
LONG readBlock(struct Library *, struct PartitionHandle *, ULONG, void *);
LONG PartitionWriteBlock(struct Library *, struct PartitionHandle *, ULONG, void *);
struct TagItem *findTagItem(ULONG tag, struct TagItem *);
void fillMem(BYTE *, LONG, BYTE);

#define getGeometry PartitionGetGeometry
#define writeBlock  PartitionWriteBlock

#endif /* PARTITION_SUPPORT_H */
