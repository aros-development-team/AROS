#ifndef PARTITION_SUPPORT_H
#define PARTITION_SUPPORT_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
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
    LONG        (*getPartitionTableAttr)   (struct Library *, struct PartitionHandle *, const struct TagItem *);
    LONG        (*getPartitionAttr)        (struct Library *, struct PartitionHandle *, const struct TagItem *);
    const struct PartitionAttribute *partitionTableAttrs;
    const struct PartitionAttribute *partitionAttrs;
    struct Node *(*findFileSystem)	 (struct Library *, struct PartitionHandle *, const struct TagItem *);
    /* Write support lives in ptedit.library, see ptedit_intern.h */
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

#define DE_NUMBUFFERS_MIN 20
#define DE_NUMBUFFERS_MAX 1024
ULONG PartitionDefaultNumBuffers(void);
void PartitionNsdCheck(struct Library *, struct PartitionHandle *);
ULONG getStartBlock(struct PartitionHandle *);
LONG deviceError(LONG err);

void initPartitionHandle(struct PartitionHandle *root, struct PartitionHandle *ph, ULONG first_sector, ULONG count_sector);
void setDosType(struct DosEnvec *de, ULONG type);
void PartitionUpdateGeometry(struct PartitionHandle *ph);
void PartitionFreeHandle(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG datasize);
void PartitionFreeHandles(struct Library *PartitionBase, struct List *list, ULONG datasize);
LONG PartitionDoIO(struct PartitionHandle *ph, APTR buf, ULONG size, UQUAD block, UWORD cmd);
ULONG PartitionDEStart(const struct DosEnvec *de);
ULONG PartitionDECount(const struct DosEnvec *de);
UQUAD PartitionMulQ(ULONG a, ULONG b, ULONG c);

unsigned int Crc32_ComputeBuf(unsigned int inCrc32, const void *buf, unsigned int bufLen);

/* read a single block within partition ph */
LONG readBlock(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG block, void *mem);

#define getGeometry PartitionGetGeometry

#endif /* PARTITION_SUPPORT_H */
