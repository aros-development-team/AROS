/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: EBR partition table handler, write support
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

#define PartitionEBRFreeHandle(base, ph) PartitionFreeHandle(base, ph, sizeof(struct EBRData))

static LONG PartitionEBRWritePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
ULONG block_no, block_count;
struct PartitionHandle *ph, *next_ph, *tail;
struct EBRData *data, *next_data;
struct DosEnvec *de;
struct MBR *ebr;
LONG error = 0;

    ebr = AllocMem(root->de.de_SizeBlock << 2, MEMF_PUBLIC | MEMF_CLEAR);
    if (ebr != NULL)
    {
        ph = (struct PartitionHandle *)root->table->list.lh_Head;
        tail = (struct PartitionHandle *)&root->table->list.lh_Tail;

        if (ph == tail)
        {
            /* Write an empty EBR block for empty partition table */

            ebr->magic = AROS_WORD2LE(0xAA55);
            if (writeBlock(PartitionBase, root, 0, ebr))
                error = 1;

        }
        else
        {
            /* First EBR must be at extended partition's first block */
            data = (struct EBRData *)ph->data;
            data->ebr_block_no = 0;

            /* Write an EBR block for every partition in the list */

        while (ph != tail)
        {
            data = (struct EBRData *)ph->data;
            next_ph = (struct PartitionHandle *)ph->ln.ln_Succ;

            /* Write table entry for current EBR's logical partition */

            de = &ph->de;
            block_no = PartitionDEStart(de);
            block_count = PartitionDECount(de);

            PartitionMBRSetGeometry(root, &ebr->pcpt[0], block_no,
                block_count, data->ebr_block_no);
            ebr->pcpt[0].status = 0;
            ebr->pcpt[0].type = data->type;

            /* Write table entry that points to next EBR in chain */

            if (next_ph != tail)
            {
                de = &root->de;
                next_data = (struct EBRData *)next_ph->data;
                block_no = next_data->ebr_block_no;
                block_count = PartitionDECount(de) - block_no;
                ebr->pcpt[1].type = MBRT_EXTENDED;
            }
            else
            {
                block_no = 0;
                block_count = 0;
                ebr->pcpt[1].type = 0;
            }
            PartitionMBRSetGeometry(root, &ebr->pcpt[1], block_no, block_count, 0);
            ebr->pcpt[1].status = 0;

            /* Write EBR */

            ebr->magic = AROS_WORD2LE(0xAA55);
            if (writeBlock(PartitionBase, root, data->ebr_block_no, ebr))
                error = 1;

            ph = next_ph;
        }
        }

        FreeMem(ebr, root->de.de_SizeBlock << 2);
    }
    else
        error = 1;

    return error;
}

static LONG PartitionEBRCreatePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    NEWLIST(&ph->table->list);
    return 0;
}

static struct PartitionHandle *PartitionEBRAddPartition(struct Library *PartitionBase, struct PartitionHandle *root, const struct TagItem *taglist)
{
    struct TagItem *tag;
    ULONG block_no = 0, new_block_no, new_block_count, ebr_track_no, ebr_block_no;
    struct PartitionHandle *ph, *new_ph, *head;
    struct DosEnvec *de;
    BOOL found = FALSE;
    struct PartitionType *ptype;
    LONG error = 0;

    tag = FindTagItem(PT_DOSENVEC, taglist);
    if (tag == NULL)
        error = 1;

    if (error == 0)
    {
        de = (struct DosEnvec *)tag->ti_Data;

        tag = FindTagItem(PT_TYPE, taglist);
        if (tag == NULL)
            error = 1;
    }

    if (error == 0)
    {
        ptype = (struct PartitionType *)tag->ti_Data;

        new_block_no = PartitionDEStart(de);
        new_block_count = PartitionDECount(de);

        /* Find the position in the chain/list of EBRs at which to insert
           new EBR */

        ph = (struct PartitionHandle *)root->table->list.lh_TailPred;
        head = (struct PartitionHandle *)&root->table->list;

        while (ph != head && !found)
        {
            block_no = PartitionDEStart(&ph->de);
            if (block_no < new_block_no)
                found = TRUE;
            else
                ph = (struct PartitionHandle *)ph->ln.ln_Pred;
        }

        /* Calculate appropriate location for new EBR */

        de = &root->de;
        ebr_track_no = (new_block_no - 1) / de->de_BlocksPerTrack;
        ebr_block_no = ebr_track_no * de->de_BlocksPerTrack;

        /* Create new handle and add it to list */

        new_ph = PartitionEBRNewHandle(PartitionBase, root, ptype->id[0],
            new_block_no, new_block_count, ebr_block_no);
        Insert(&root->table->list, &new_ph->ln, &ph->ln);

        return new_ph;
    }
    return NULL;
}

static void PartitionEBRDeletePartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    Remove(&ph->ln);
    PartitionEBRFreeHandle(PartitionBase, ph);
}

static LONG PartitionEBRSetPartitionAttrs(struct Library *PartitionBase, struct PartitionHandle *ph, const struct TagItem *taglist)
{
    struct EBRData *data = (struct EBRData *)ph->data;
    struct TagItem *tag;

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case PT_DOSENVEC:
            // TO DO: move handle to new position in list
            CopyMem((struct DosEnvec *)tag->ti_Data, &ph->de, sizeof(struct DosEnvec));
            break;
        case PT_TYPE:
            data->type = PTYPE(tag->ti_Data)->id[0]; // fix
            /* Update DOSType according to a new type ID */
            setDosType(&ph->de, MBR_FindDosType(data->type));
            break;
        }
    }

    return 0;
}

static ULONG PartitionEBRDestroyPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;


        while ((ph = (struct PartitionHandle *)RemHead(&root->table->list))
            != NULL)
        {
            PartitionEBRDeletePartition(PartitionBase, ph);
        }
        PartitionEBRWritePartitionTable(PartitionBase, root);

    return 0;
}

const struct PTEditFunctionTable PtEditEBR =
{
    PHPTT_EBR,
    PartitionEBRWritePartitionTable,
    PartitionEBRCreatePartitionTable,
    PartitionEBRAddPartition,
    PartitionEBRDeletePartition,
    NULL,
    PartitionEBRSetPartitionAttrs,
    PartitionEBRDestroyPartitionTable
};
