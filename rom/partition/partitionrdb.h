#ifndef PARTITIONRDB_H
#define PARTITIONRDB_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <devices/hardblocks.h>
#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_support.h"

#define IDNAME_BOOT 0x424f4f54

/*
 * RDBData is table->data. Each record below is AllocMem()'d so that its
 * last member holds a whole device block, and is freed by partition.library's
 * ClosePartitionTable(). Partition handles come from PartitionRDBAllocHandle().
 */
struct RDBData {
    UBYTE rdbblock; /* the block rdb was read from */
    LONG readerror; /* partial discovery must never be rewritten */
    BOOL newtable;  /* invalidate an earlier header only on initial save */
    struct List badblocklist;
    struct List fsheaderlist;
    struct FileSysNode *driveinit;
    struct FileSysNode *bootcode;
    struct RigidDiskBlock rdb; /* last: allocation includes the rest of the sector */
};

struct BadBlockNode {
    struct Node ln;
    struct BadBlockBlock bbb;
};

struct FileSysNode
{
    struct FileSysHandle h;

    struct LoadSegBlock *filesystem; /* the FS in LSEG blocks */
    ULONG fsblocks;                  /* nr of LSEG blocks for FS */
    ULONG blockbytes;                /* size of each stored LSEG block */
    ULONG segmentid;                 /* LSEG, or BOOT for the OS4 boot chain */
    ULONG filesize;                  /* sum of the encoded LSEG payload sizes */
    char name[85];                  /* terminated copy of the on-disk name */
    struct FileSysHeaderBlock fhb; /* last: allocation includes the rest of the sector */
};

#define PartitionRDBUpdateGeometry PartitionUpdateGeometry

/* Shared by partition.library and ptedit.library (partitionrdb_common.c) */
ULONG calcChkSum(ULONG *ptr, ULONG size);
struct LoadSegBlock *PartitionRDBFileSysBlock(struct FileSysNode *fn, ULONG block);
struct PartitionHandle *PartitionRDBAllocHandle(struct PartitionHandle *root);
void PartitionRDBFreeHandleMem(struct PartitionHandle *ph);
void PartitionRDBFreeHandle(struct Library *PartitionBase, struct PartitionHandle *ph);

#endif /* PARTITIONRDB_H */
