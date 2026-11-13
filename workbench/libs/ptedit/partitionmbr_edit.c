/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: MBR partition table handler, write support
*/

#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <proto/utility.h>

#include "partition_types.h"
#include "ptedit_intern.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

void MBRPutLE32(void *p, ULONG v)
{
    UBYTE *b = p;

    b[0] = v;
    b[1] = v >> 8;
    b[2] = v >> 16;
    b[3] = v >> 24;
}

#define PartitionMBRFreeHandle(base, ph) PartitionFreeHandle(base, ph, sizeof(struct MBRData))

static LONG PartitionMBRWritePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
    /* FIXME: readBlock(0) and synchronize data */

    /* root->data = mbr is up to date */
    if (writeBlock(PartitionBase, root, 0, root->table->data))
        return 1;
    return 0;
}

static LONG PartitionMBRCreatePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
struct MBR *mbr;

    mbr = AllocMem(ph->de.de_SizeBlock<<2, MEMF_PUBLIC);
    if (mbr)
    {
        if (readBlock(PartitionBase, ph, 0, mbr) == 0)
        {
            ph->table->data = mbr;

            SetMem(mbr->pcpt, 0, sizeof(mbr->pcpt));
            mbr->magic = AROS_WORD2LE(0xAA55);

            NEWLIST(&ph->table->list);
            return 0;
        }
        FreeMem(mbr, ph->de.de_SizeBlock<<2);
    }
    return 1;
}

/* Store a CHS address as head, sector (with cylinder bits 8-9) and cylinder.
   Addresses beyond cylinder 1023 get the conventional 0xFE/0xFF/0xFF. */
static __attribute__((noinline)) void PartitionMBRSetCHS(struct PartitionHandle *root, UBYTE *chs, ULONG sector)
{
    ULONG track = sector / root->de.de_BlocksPerTrack;
    ULONG cyl = track / root->de.de_Surfaces;

    if (cyl < 1024)
    {
        chs[0] = track % root->de.de_Surfaces;
        chs[1] = ((sector % root->de.de_BlocksPerTrack) + 1) | ((cyl & 0x300) >> 2);
        chs[2] = cyl & 0xFF;
    }
    else
    {
        chs[0] = 0xFE;
        chs[1] = 0xFF;
        chs[2] = 0xFF;
    }
}

void PartitionMBRSetGeometry
    (
        struct PartitionHandle *root,
        struct PCPartitionTable *entry,
        ULONG sector,
        ULONG count,
        ULONG relative_sector
    )
{
    /* Store LBA start block and block count */
    MBRPutLE32(&entry->first_sector, sector - relative_sector);
    MBRPutLE32(&entry->count_sector, count);

    /* CHS addresses of the first and last block */
    PartitionMBRSetCHS(root, &entry->start_head, sector);
    PartitionMBRSetCHS(root, &entry->end_head, sector + count - 1);
}

static void PartitionMBRSetDosEnvec
    (
        struct PartitionHandle *root,
        struct PCPartitionTable *entry,
        struct DosEnvec *de
    )
{
ULONG sector, count;

    sector = PartitionDEStart(de);
    count = PartitionDECount(de);
    PartitionMBRSetGeometry(root, entry, sector, count, 0);
}

static struct PartitionHandle *PartitionMBRAddPartition(struct Library *PartitionBase, struct PartitionHandle *root, const struct TagItem *taglist)
{
    struct TagItem *tag;

    tag = FindTagItem(PT_DOSENVEC, taglist);

    if (tag)
    {
        struct PCPartitionTable *entry;
        struct PartitionHandle *ph;
        struct DosEnvec *de = (struct DosEnvec *)tag->ti_Data;
        WORD pos = -1, i;

        tag = FindTagItem(PT_POSITION, taglist);
        if (tag != NULL)
            pos = tag->ti_Data;
        else
        {
            // Find an unused slot
            for (i = 0; i < MBR_MAX_PARTITIONS && pos == -1; i++)
            {
                entry = &((struct MBR *)root->table->data)->pcpt[i];
                if (entry->type == 0)
                    pos = i;
            }
        }

        if (pos != -1)
        {
            entry = &((struct MBR *)root->table->data)->pcpt[pos];
            tag = FindTagItem(PT_ACTIVE, taglist);
            if (tag)
                entry->status = tag->ti_Data ? 0x80 : 0;
            else
                entry->status = 0;
            tag = FindTagItem(PT_TYPE, taglist);
            if (tag)
            {
            struct PartitionType *ptype = (struct PartitionType *)tag->ti_Data;

                entry->type = ptype->id[0];
            }
            else
                entry->type = 0;
            PartitionMBRSetDosEnvec(root, entry, de);
            ph = PartitionMBRNewHandle(PartitionBase, root, pos, entry);
            if (ph != NULL)
                Enqueue(&root->table->list, &ph->ln);
            else
                SetMem(entry, 0, sizeof(struct PCPartitionTable));

            return ph;
        }
    }
    return NULL;
}

static void PartitionMBRDeletePartition(struct Library *PartitionBase, struct PartitionHandle *ph)
{
    struct MBRData *data = (struct MBRData *)ph->data;

    SetMem(data->entry, 0, sizeof(struct PCPartitionTable));

    Remove(&ph->ln);
    PartitionMBRFreeHandle(PartitionBase, ph);
}

static LONG PartitionMBRSetPartitionAttrs(struct Library *PartitionBase, struct PartitionHandle *ph, const struct TagItem *taglist)
{
    struct MBRData *data = (struct MBRData *)ph->data;
    struct TagItem *tag;

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case PT_DOSENVEC:
            CopyMem((struct DosEnvec *)tag->ti_Data, &ph->de, sizeof(struct DosEnvec));
            PartitionMBRSetDosEnvec(ph->root, data->entry, (struct DosEnvec *)tag->ti_Data);
            break;

        case PT_TYPE:
            data->entry->type = PTYPE(tag->ti_Data)->id[0];
            /* Update DOSType according to a new type ID */
            setDosType(&ph->de, MBR_FindDosType(data->entry->type));
            break;

        case PT_POSITION:
            if (tag->ti_Data != data->position)
            {
            struct PartitionHandle *node;
            struct PCPartitionTable *entry;

                node = (struct PartitionHandle *)ph->root->table->list.lh_Head;
                while (node->ln.ln_Succ)
                {
                    if (tag->ti_Data == ((struct MBRData *)node->data)->position)
                        goto posbreak;
                    node = (struct PartitionHandle *)node->ln.ln_Succ;
                }
                data->position = tag->ti_Data;
                entry = &((struct MBR *)ph->root->table->data)->pcpt[data->position];
                CopyMem(data->entry, entry, sizeof(struct PCPartitionTable));
                SetMem(data->entry, 0, sizeof(struct PCPartitionTable));
                data->entry = entry;
                ph->ln.ln_Pri = MBR_MAX_PARTITIONS-1-data->position;
                Remove(&ph->ln);
                Enqueue(&ph->root->table->list, &ph->ln);
posbreak:
            ;
            }
            break;

        case PT_ACTIVE:
            if (tag->ti_Data)
                data->entry->status |= 0x80;
            else
                data->entry->status &= ~0x80;
            break;
        }
    }
    return 0;
}

static ULONG PartitionMBRDestroyPartitionTable(struct Library *PartitionBase,
    struct PartitionHandle *root)
{
    struct MBR *mbr = root->table->data;

    SetMem(mbr->pcpt, 0, sizeof(mbr->pcpt));
    /* deleting the magic value will invalidate the
     * partition table so it cannot be opened again
     */
    mbr->magic = 0;
    if (writeBlock(PartitionBase, root, 0, root->table->data))
        return 1;
    return 0;
}

const struct PTEditFunctionTable PtEditMBR =
{
    PHPTT_MBR,
    PartitionMBRWritePartitionTable,
    PartitionMBRCreatePartitionTable,
    PartitionMBRAddPartition,
    PartitionMBRDeletePartition,
    NULL,
    PartitionMBRSetPartitionAttrs,
    PartitionMBRDestroyPartitionTable
};
