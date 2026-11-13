/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

*/

#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <proto/utility.h>

#include "partition_types.h"
#include "partition_support.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

static LONG PartitionEBRCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
union {
    struct MBR mbr;
    UBYTE space[4096];
} sector;

struct PartitionType type;
struct TagItem tags[] = {{PT_TYPE, (IPTR)&type}, {TAG_DONE, 0}};

    if ((root->de.de_SizeBlock << 2) > sizeof(sector))
        return 0;

    if (readBlock(PartitionBase, root, 0, &sector.mbr) == 0)
    {
        if (AROS_LE2WORD(sector.mbr.magic) == 0xAA55)
        {
            if (root->root != NULL)
            {
                GetPartitionAttrs(root, tags);
                if (
                        (root->root->table->type == PHPTT_MBR) &&
                        (type.id[0] == MBRT_EXTENDED
                        || type.id[0] == MBRT_EXTENDED2)
                    )
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static LONG PartitionEBROpenPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
LONG error = 0;
struct PartitionHandle *ph;
struct MBR *ebr;
UBYTE i;
ULONG block_no = 0;
BOOL atEnd = FALSE;

    ebr = AllocMem(root->de.de_SizeBlock<<2, MEMF_PUBLIC);
    if (ebr != NULL)
    {
        NEWLIST(&root->table->list);
        for (i = 0; !atEnd && error == 0; i++)
        {
            if (readBlock(PartitionBase, root, block_no, ebr) == 0)
            {
                if (AROS_LE2WORD(ebr->magic) == 0xAA55)
                {
                    /* Create handle for current EBR's logical partition */

                    if (MBRGetLE32(&ebr->pcpt[0].count_sector) != 0)
                    {
                        ph = PartitionEBRNewHandle(PartitionBase, root,
                            ebr->pcpt[0].type,
                            block_no + MBRGetLE32(&ebr->pcpt[0].first_sector),
                            MBRGetLE32(&ebr->pcpt[0].count_sector),
                            block_no);
                        if (ph != NULL)
                        {
                            Enqueue(&root->table->list, &ph->ln);
                        }
                        else
                            error = 1;
                    }

                    /* Get location of next EBR in chain */

                    block_no = MBRGetLE32(&ebr->pcpt[1].first_sector);
                    if (block_no == 0)
                        atEnd = TRUE;
                }
                else
                    error = 1;
            }
            else
                error = 1;
        }
        FreeMem(ebr, root->de.de_SizeBlock<<2);
    }
    else
        error = 1;
    return error;
}

static void PartitionEBRClosePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;

    PartitionFreeHandles(PartitionBase, &root->table->list, sizeof(struct EBRData));
}

static LONG PartitionEBRGetPartitionTableAttr(struct Library *PartitionBase,
    struct PartitionHandle *root, const struct TagItem *tag)
{
    switch (tag->ti_Tag)
    {
    case PTT_MAXLEADIN:
        *((LONG *)tag->ti_Data) = root->de.de_BlocksPerTrack;
        return TRUE;
    }

    return 0;
}

static LONG PartitionEBRGetPartitionAttr(struct Library *PartitionBase,
    struct PartitionHandle *ph, const struct TagItem *tag)
{
    struct EBRData *data = (struct EBRData *)ph->data;

    switch (tag->ti_Tag)
    {
    case PT_TYPE:
        PTYPE(tag->ti_Data)->id[0]  = data->type;
        PTYPE(tag->ti_Data)->id_len = 1;
        return TRUE;

    case PT_STARTBLOCK:
        *((UQUAD *)tag->ti_Data) = data->block_no;
        return TRUE;

    case PT_ENDBLOCK:
        *((UQUAD *)tag->ti_Data) = data->block_no + data->block_count - 1;
        return TRUE;
    }

    return 0;
}

static const struct PartitionAttribute PartitionEBRPartitionTableAttrs[]=
{
    {PTT_TYPE,           PLAM_READ},
    {PTT_RESERVED,       PLAM_READ},
    {PTT_MAXLEADIN,      PLAM_READ},
    {TAG_DONE,           0}
};

static const struct PartitionAttribute PartitionEBRPartitionAttrs[]=
{
    {PT_GEOMETRY,  PLAM_READ},
    {PT_TYPE,      PLAM_READ | PLAM_WRITE},
    {PT_DOSENVEC,  PLAM_READ | PLAM_WRITE},
    {PT_POSITION,  PLAM_READ},
    {PT_LEADIN,    PLAM_READ},
    {PT_STARTBLOCK, PLAM_READ},
    {PT_ENDBLOCK,   PLAM_READ},
    {TAG_DONE, 0}
};

const struct PTFunctionTable PartitionEBR =
{
    PHPTT_EBR,
    "PC-EBR",
    PartitionEBRCheckPartitionTable,
    PartitionEBROpenPartitionTable,
    PartitionEBRClosePartitionTable,
    PartitionEBRGetPartitionTableAttr,
    PartitionEBRGetPartitionAttr,
    PartitionEBRPartitionTableAttrs,
    PartitionEBRPartitionAttrs,
    NULL
};
