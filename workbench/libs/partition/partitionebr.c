/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <proto/exec.h>
#include <proto/partition.h>

#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_support.h"
#include "partitionmbr.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include "debug.h"

struct EBRData {
    UBYTE type;
    ULONG ebr_block_no;
};

static LONG PartitionEBRCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct MBR mbr;
struct PartitionType type;
struct TagItem tags[] = {{PT_TYPE, (IPTR)&type}, {TAG_DONE, 0}};

    if (readBlock(PartitionBase, root, 0, &mbr) == 0)
    {
        if (AROS_LE2WORD(mbr.magic) == 0xAA55)
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

static struct PartitionHandle *PartitionEBRNewHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        UBYTE type,
        ULONG block_no,
        ULONG block_count,
        ULONG ebr_block_no
    )
{
struct PartitionHandle *ph;
ULONG cylsecs;

    {
        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph != NULL)
        {
        struct EBRData *data;

            data = AllocMem(sizeof(struct EBRData), MEMF_PUBLIC);
            if (data != NULL)
            {
                cylsecs = root->de.de_BlocksPerTrack*root->de.de_Surfaces;
                data->type = type;
                data->ebr_block_no = ebr_block_no;
                ph->root = root;
                ph->bd = root->bd;
                ph->data = data;

                /* Initialize DosEnvec */

                /* Check if partition starts and ends on a cylinder boundary */
                CopyMem(&root->de, &ph->de, sizeof(struct DosEnvec));
                if (
                        block_no % cylsecs != 0 ||
                        block_count % cylsecs != 0
                    )
                {
                    /* It doesn't. We could find the highest common factor of
                       first_sector and count_sector here, but currently we
                       simply use one block per cylinder */
                    ph->de.de_Surfaces = 1;
                    ph->de.de_BlocksPerTrack = 1;
                    cylsecs = ph->de.de_BlocksPerTrack*ph->de.de_Surfaces;
                }
                ph->de.de_LowCyl = block_no / cylsecs;
                ph->de.de_HighCyl = ph->de.de_LowCyl + block_count/cylsecs - 1;
                ph->de.de_TableSize = 10; // only until de_HighCyl

                /* Initialize DriveGeometry */
                ph->dg.dg_DeviceType = DG_DIRECT_ACCESS;
                ph->dg.dg_SectorSize = ph->de.de_SizeBlock<<2;
                ph->dg.dg_Heads = ph->de.de_Surfaces;
                ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
                ph->dg.dg_Cylinders = ph->de.de_HighCyl - ph->de.de_LowCyl + 1;
                ph->dg.dg_BufMemType = ph->de.de_BufMemType;
                return ph;
            }
            FreeMem(ph, sizeof(struct PartitionHandle));
        }
    }
    return NULL;
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
UBYTE i, j = 0;
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

                    if (AROS_LE2LONG(ebr->pcpt[0].count_sector) != 0)
                    {
                        ph = PartitionEBRNewHandle(PartitionBase, root,
                            ebr->pcpt[0].type,
                            block_no + AROS_LE2LONG(ebr->pcpt[0].first_sector),
                            AROS_LE2LONG(ebr->pcpt[0].count_sector),
                            block_no);
                        if (ph != NULL)
                        {
                            Enqueue(&root->table->list, &ph->ln);
                        }
                        else
                            error = 1;
                    }

                    /* Get location of next EBR in chain */

                    block_no = AROS_LE2LONG(ebr->pcpt[1].first_sector);
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

static void PartitionEBRFreeHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    ClosePartitionTable(ph);
    FreeMem(ph->data, sizeof(struct EBRData));
    FreeMem(ph, sizeof(struct PartitionHandle));
}

static void PartitionEBRClosePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;

    while ((ph = (struct PartitionHandle *)RemTail(&root->table->list)))
        PartitionEBRFreeHandle(PartitionBase, ph);
}

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
            block_no = de->de_LowCyl * de->de_Surfaces * de->de_BlocksPerTrack;
            block_count = (de->de_HighCyl - de->de_LowCyl + 1) *
                de->de_Surfaces * de->de_BlocksPerTrack;

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
                block_count = (de->de_HighCyl - de->de_LowCyl + 1) *
                    de->de_Surfaces * de->de_BlocksPerTrack - block_no;
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

static struct PartitionHandle *PartitionEBRAddPartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{
struct TagItem *tag;
struct EBRData *data;
ULONG block_no = 0, new_block_no, new_block_count, ebr_track_no, ebr_block_no;
struct PartitionHandle *ph, *new_ph, *head;
struct DosEnvec *de;
BOOL found = FALSE;
struct PartitionType *ptype;
LONG error = 0;

    tag = findTagItem(PT_DOSENVEC, taglist);
    if (tag == NULL)
        error = 1;

    if (error == 0)
    {
        de = (struct DosEnvec *)tag->ti_Data;

        tag = findTagItem(PT_TYPE, taglist);
        if (tag == NULL)
            error = 1;
    }

    if (error == 0)
    {
        ptype = (struct PartitionType *)tag->ti_Data;

        new_block_no =
            de->de_LowCyl * de->de_Surfaces * de->de_BlocksPerTrack;
        new_block_count = (de->de_HighCyl - de->de_LowCyl + 1) *
            de->de_Surfaces * de->de_BlocksPerTrack;

        /* Find the position in the chain/list of EBRs at which to insert
           new EBR */

        ph = (struct PartitionHandle *)root->table->list.lh_TailPred;
        head = (struct PartitionHandle *)&root->table->list;

        while (ph != head && !found)
        {
            data = (struct EBRData *)ph->data;
            de = &ph->de;
            block_no = de->de_LowCyl * de->de_Surfaces * de->de_BlocksPerTrack;
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

static LONG PartitionEBRGetPartitionTableAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{
    while (taglist[0].ti_Tag != TAG_DONE)
    {
        switch (taglist[0].ti_Tag)
        {
        case PTT_TYPE:
            *((LONG *)taglist[0].ti_Data) = root->table->type;
            break;
        case PTT_MAXLEADIN:
            *((LONG *)taglist[0].ti_Data) = root->de.de_BlocksPerTrack;
            break;
        }
        taglist++;
    }
    return 0;
}

static LONG PartitionEBRSetPartitionTableAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{
    while (taglist[0].ti_Tag != TAG_DONE)
    {
        switch (taglist[0].ti_Tag)
        {
        }
        taglist++;
    }
    return 0;
}

static LONG PartitionEBRGetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{
ULONG i;
struct PartitionHandle *list_ph;

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct EBRData *data = (struct EBRData *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_GEOMETRY:
            {
                struct DriveGeometry *dg = (struct DriveGeometry *)taglist[0].ti_Data;
                CopyMem(&ph->dg, dg, sizeof(struct DriveGeometry));
            }
            break;
        case PT_DOSENVEC:
            CopyMem(&ph->de, (struct DosEnvec *)taglist[0].ti_Data, sizeof(struct DosEnvec));
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;

                ptype->id[0] = data->type;
                ptype->id_len = 1;
            }
            break;
        case PT_POSITION:
            i = 0;
            ForeachNode(&ph->root->table->list, list_ph)
            {
                if (list_ph == ph)
                    *((LONG *)taglist[0].ti_Data) = i;
                i++;
            }
            break;
        }
        taglist++;
    }
    return 0;
}

static LONG PartitionEBRSetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{
    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct EBRData *data = (struct EBRData *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_DOSENVEC:
            {
// TO DO: move handle to new position in list
            struct DosEnvec *de;
                de = (struct DosEnvec *)taglist[0].ti_Data;
                CopyMem(de, &ph->de, sizeof(struct DosEnvec));
            }
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;

                data->type = ptype->id[0]; // fix
            }
            break;
        }
        taglist++;
    }
    return 0;
}

static struct PartitionAttribute PartitionEBRPartitionTableAttrs[]=
{
    {PTTA_TYPE,           PLAM_READ},
    {PTTA_RESERVED,       PLAM_READ},
    {PTTA_MAXLEADIN,      PLAM_READ},
    {PTTA_DONE,           0}
};

static struct PartitionAttribute *PartitionEBRQueryPartitionTableAttrs(
    struct Library *PartitionBase)
{
    return PartitionEBRPartitionTableAttrs;
}

static struct PartitionAttribute PartitionEBRPartitionAttrs[]=
{
    {PTA_GEOMETRY, PLAM_READ},
    {PTA_TYPE,     PLAM_READ | PLAM_WRITE},
    {PTA_DOSENVEC, PLAM_READ | PLAM_WRITE},
    {PTA_POSITION, PLAM_READ},
    {PTA_LEADIN,   PLAM_READ},
    {PTA_DONE,     0}
};

static struct PartitionAttribute *PartitionEBRQueryPartitionAttrs(
    struct Library *PartitionBase)
{
    return PartitionEBRPartitionAttrs;
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

struct PTFunctionTable PartitionEBR =
{
    PHPTT_EBR,
    "PC-EBR",
    PartitionEBRCheckPartitionTable,
    PartitionEBROpenPartitionTable,
    PartitionEBRClosePartitionTable,
    PartitionEBRWritePartitionTable,
    PartitionEBRCreatePartitionTable,
    PartitionEBRAddPartition,
    PartitionEBRDeletePartition,
    PartitionEBRGetPartitionTableAttrs,
    PartitionEBRSetPartitionTableAttrs,
    PartitionEBRGetPartitionAttrs,
    PartitionEBRSetPartitionAttrs,
    PartitionEBRQueryPartitionTableAttrs,
    PartitionEBRQueryPartitionAttrs,
    PartitionEBRDestroyPartitionTable
};

