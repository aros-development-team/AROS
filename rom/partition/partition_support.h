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

struct FSFunctionTable;

struct FileSysHandle
{
    struct Node ln;
    const struct FSFunctionTable *handler;
    BOOL boot;
    /* Handler's private data follows */
};

struct FSFunctionTable
{
    BPTR (*loadFileSystem)   (struct PartitionBase_intern *, struct FileSysHandle *);
    LONG (*getFileSystemAttr)(struct Library *, struct FileSysHandle *, const struct TagItem *);
    void (*freeFileSystem)   (struct FileSysHandle *);
};

struct PTFunctionTable
{
    ULONG       type; /* Partition Table Type */
    CONST_STRPTR name;
    LONG        (*checkPartitionTable)     (struct Library *, struct PartitionHandle *);
    LONG        (*openPartitionTable)      (struct Library *, struct PartitionHandle *);
    void        (*closePartitionTable)     (struct Library *, struct PartitionHandle *);
    LONG        (*writePartitionTable)     (struct Library *, struct PartitionHandle *);
    LONG        (*createPartitionTable)    (struct Library *, struct PartitionHandle *);
    struct PartitionHandle *(*addPartition)(struct Library *, struct PartitionHandle *, struct TagItem *);
    void        (*deletePartition)         (struct Library *, struct PartitionHandle *);
    LONG        (*getPartitionTableAttr)   (struct Library *, struct PartitionHandle *, struct TagItem *);
    LONG        (*setPartitionTableAttrs)  (struct Library *, struct PartitionHandle *, struct TagItem *);
    LONG        (*getPartitionAttr)        (struct Library *, struct PartitionHandle *, struct TagItem *);
    LONG        (*setPartitionAttrs)       (struct Library *, struct PartitionHandle *, struct TagItem *);
    const struct PartitionAttribute *partitionTableAttrs;
    const struct PartitionAttribute *partitionAttrs;
    ULONG    	(*destroyPartitionTable) (struct Library *, struct PartitionHandle *);
    struct Node *(*findFileSystem)	 (struct Library *, struct PartitionHandle *, struct TagItem *);
};

struct BootFileSystem
{
    struct Node ln;
    struct FileSysHandle *handle;
};

extern const struct PTFunctionTable * const PartitionSupport[];

extern const struct PTFunctionTable PartitionEBR;
extern const struct PTFunctionTable PartitionMBR;
extern const struct PTFunctionTable PartitionRDB;
extern const struct FSFunctionTable FilesystemRDB;

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
