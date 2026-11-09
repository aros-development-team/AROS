/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <proto/utility.h>

#include <linklibs/fatbpb.h>
#include "partition_types.h"
#include "partition_support.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

LONG MBRCheckPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root, void *buffer)
{
    struct MBR *mbr = buffer;
    LONG res = 0;

    if (readBlock(PartitionBase, root, 0, mbr) == 0 &&
        !FAT_IsBPBPlausible(buffer, root->de.de_SizeBlock << 2))
    {
        struct PCPartitionTable *pcpt = mbr->pcpt;

        /* Check status bytes of all partition slots and block signature */
        if ((AROS_LE2WORD(mbr->magic) == MBR_MAGIC) &&
            MBR_STATUS_VALID(pcpt[0].status) &&
            MBR_STATUS_VALID(pcpt[1].status) &&
            MBR_STATUS_VALID(pcpt[2].status) &&
            MBR_STATUS_VALID(pcpt[3].status))
            res = 1;
    }

    return res;
}

static LONG PartitionMBRCheckPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    LONG res;
    void *blk;

    /* MBR can be placed only in the root of the disk */
    if (root->root)
        return 0;

    blk = AllocMem(root->de.de_SizeBlock << 2, MEMF_ANY);
    if (!blk)
        return 0;

    res = MBRCheckPartitionTable(PartitionBase, root, blk);

    FreeMem(blk, root->de.de_SizeBlock << 2);
    return res;
}

static LONG PartitionMBROpenPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    struct PartitionHandle *ph;
    struct MBR *mbr;
    UBYTE i;

    mbr = AllocMem(root->de.de_SizeBlock<<2, MEMF_PUBLIC);
    if (mbr)
    {
        if (readBlock(PartitionBase, root, 0, mbr) == 0)
        {
            root->table->data = mbr;
            for (i=0;i<4;i++)
            {
                ph = PartitionMBRNewHandle(PartitionBase, root, i, &mbr->pcpt[i]);
                if (ph != NULL)
                    Enqueue(&root->table->list, &ph->ln);
            }
            return 0;
        }
        FreeMem(mbr, root->de.de_SizeBlock<<2);
    }
    return 1;
}

static void PartitionMBRClosePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;

    PartitionFreeHandles(PartitionBase, &root->table->list, sizeof(struct MBRData));
    FreeMem(root->table->data, root->de.de_SizeBlock<<2);
}

static LONG PartitionMBRGetPartitionTableAttr(struct Library *PartitionBase,
    struct PartitionHandle *root, const struct TagItem *tag)
{
    switch (tag->ti_Tag)
    {
    case PTT_RESERVED:
        *((LONG *)tag->ti_Data) = root->de.de_BlocksPerTrack; /* One track */
        return TRUE;

    case PTT_MAX_PARTITIONS:
        *((LONG *)tag->ti_Data) = MBR_MAX_PARTITIONS;
        return TRUE;
    }

    return 0;
}

static LONG PartitionMBRGetPartitionAttr(struct Library *PartitionBase,
    struct PartitionHandle *ph, const struct TagItem *tag)
{
    struct MBRData *data = (struct MBRData *)ph->data;

    switch (tag->ti_Tag)
    {
    case PT_TYPE:
        PTYPE(tag->ti_Data)->id[0] = (LONG)data->entry->type;
        PTYPE(tag->ti_Data)->id_len = 1;
        return TRUE;

    case PT_POSITION:
        *((LONG *)tag->ti_Data) = (LONG)data->position;
        return TRUE;

    case PT_ACTIVE:
        *((LONG *)tag->ti_Data) = data->entry->status & 0x80 ? 1 : 0;
        return TRUE;

    case PT_STARTBLOCK:
        *((UQUAD *)tag->ti_Data) = MBRGetLE32(&data->entry->first_sector);
        return TRUE;

    case PT_ENDBLOCK:
        *((UQUAD *)tag->ti_Data) = MBRGetLE32(&data->entry->first_sector) + MBRGetLE32(&data->entry->count_sector) - 1;
        return TRUE;
    }

    /* Everything else gets default values */
    return 0;
}

static const struct PartitionAttribute PartitionMBRPartitionTableAttrs[]=
{
    {PTT_TYPE,           PLAM_READ},
    {PTT_RESERVED,       PLAM_READ},
    {PTT_MAX_PARTITIONS, PLAM_READ},
    {TAG_DONE,           0}
};

static const struct PartitionAttribute PartitionMBRPartitionAttrs[]=
{
    {PT_GEOMETRY,  PLAM_READ},
    {PT_TYPE,      PLAM_READ | PLAM_WRITE},
    {PT_POSITION,  PLAM_READ | PLAM_WRITE},
    {PT_ACTIVE,    PLAM_READ | PLAM_WRITE},
    {PT_STARTBLOCK, PLAM_READ},
    {PT_ENDBLOCK,   PLAM_READ},
    {TAG_DONE, 0}
};

const struct PTFunctionTable PartitionMBR =
{
    PHPTT_MBR,
    "PC-MBR",
    PartitionMBRCheckPartitionTable,
    PartitionMBROpenPartitionTable,
    PartitionMBRClosePartitionTable,
    PartitionMBRGetPartitionTableAttr,
    PartitionMBRGetPartitionAttr,
    PartitionMBRPartitionTableAttrs,
    PartitionMBRPartitionAttrs,
    NULL
};
