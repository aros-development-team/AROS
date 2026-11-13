#ifndef PARTITIONMBR_H
#define PARTITIONMBR_H

/*
    Copyright © 2007-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_intern.h"

struct PCPartitionTable {
   UBYTE status;
   UBYTE start_head;
   UBYTE start_sector;
   UBYTE start_cylinder;
   UBYTE type;
   UBYTE end_head;
   UBYTE end_sector;
   UBYTE end_cylinder;
   ULONG first_sector;
   ULONG count_sector;
} __attribute__((packed));

struct MBR {
   BYTE boot_data[0x1BE];
   struct PCPartitionTable pcpt[4];
   UWORD magic;
} __attribute__((packed));

#define MBR_MAGIC 0xAA55
#define MBR_STATUS_VALID(status) ((!(status & 0x0F)) || (status & 0x80))

/* MBR: table->data is the MBR block (AllocMem()'d, one block).
 * Handles and their MBRData are AllocMem()'d separately. */
struct MBRData {
    struct PCPartitionTable *entry;
    UBYTE position;
};

/* EBR: no table data. Handles and their EBRData are AllocMem()'d separately. */
struct EBRData
{
    UBYTE type;
    ULONG ebr_block_no; /* Home block of the record itself */
    ULONG block_no;     /* Start partition block           */
    ULONG block_count;  /* Length of partition in blocks   */
};

/* Unaligned little-endian access to packed partition table fields */
ULONG MBRGetLE32(const void *p);
void MBRPutLE32(void *p, ULONG v);

LONG MBRCheckPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root, void *buffer);
struct PartitionHandle *PartitionMBRNewHandle(struct Library *PartitionBase, struct PartitionHandle *root,
    UBYTE position, struct PCPartitionTable *entry);
struct PartitionHandle *PartitionEBRNewHandle(struct Library *PartitionBase, struct PartitionHandle *root,
    UBYTE type, ULONG block_no, ULONG block_count, ULONG ebr_block_no);
void PartitionMBRSetGeometry
    (
        struct PartitionHandle *root,
        struct PCPartitionTable *entry,
        ULONG sector,
        ULONG count,
        ULONG relative_sector  
    );

#define MBR_MAX_PARTITIONS (4)
#define MBRT_EXTENDED      (0x05)
#define MBRT_EXTENDED2     (0x0f)


#endif /* PARTITIONMBR_H */
