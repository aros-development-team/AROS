#ifndef PARTITION_SUPPORT_H
#define PARTITION_SUPPORT_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <libraries/partition.h>
#include <utility/tagitem.h>
#include <proto/partition.h>

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
    LONG        (*setPartitionAttrs)       (struct Library *, struct PartitionHandle *, const struct TagItem *);
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
extern const struct PTFunctionTable PartitionGPT;
extern const struct FSFunctionTable FilesystemRDB;

LONG PartitionGetGeometry(struct Library *, struct IOExtTD *, struct DriveGeometry *);
void PartitionNsdCheck(struct Library *, struct PartitionHandle *);
ULONG getStartBlock(struct PartitionHandle *);
LONG deviceError(LONG err);

void initPartitionHandle(struct PartitionHandle *root, struct PartitionHandle *ph, ULONG first_sector, ULONG count_sector);
void setDosType(struct DosEnvec *de, ULONG type);

unsigned int Crc32_ComputeBuf(unsigned int inCrc32, const void *buf, unsigned int bufLen);

/* read a single block within partition ph */
static inline LONG readBlock(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG block, void *mem)
{
    return ReadPartitionDataQ(ph, mem, ph->de.de_SizeBlock << 2, block);
}

/* write a single block within partition ph */
static inline LONG PartitionWriteBlock(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG block, void *mem)
{
    return WritePartitionDataQ(ph, mem, ph->de.de_SizeBlock << 2, block);
}

#define getGeometry PartitionGetGeometry
#define writeBlock  PartitionWriteBlock

#endif /* PARTITION_SUPPORT_H */
