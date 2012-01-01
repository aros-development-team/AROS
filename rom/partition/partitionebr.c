/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

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

struct EBRData
{
    UBYTE type;
    ULONG ebr_block_no;	/* Home block of the record itself */
    ULONG block_no;	/* Start partition block	   */
    ULONG block_count;	/* Length of partition in blocks   */
};

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

static struct PartitionHandle *PartitionEBRNewHandle(struct Library *PartitionBase, struct PartitionHandle *root,
						     UBYTE type, ULONG block_no, ULONG block_count, ULONG ebr_block_no)
{
    struct PartitionHandle *ph;

    ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
    if (ph != NULL)
    {
        struct EBRData *data;

        data = AllocMem(sizeof(struct EBRData), MEMF_PUBLIC);
        if (data != NULL)
        {
            data->type         = type;
            data->ebr_block_no = ebr_block_no;
            data->block_no     = block_no;
            data->block_count  = block_count;

            ph->data = data;

            /* Initialize DosEnvec and DriveGeometry */
            initPartitionHandle(root, ph, block_no, block_count);

	    /* Map type ID to a DOSType */
	    setDosType(&ph->de, MBR_FindDosType(data->type));

            return ph;
        }
        FreeMem(ph, sizeof(struct PartitionHandle));
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

static struct PartitionHandle *PartitionEBRAddPartition(struct Library *PartitionBase, struct PartitionHandle *root, struct TagItem *taglist)
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

static LONG PartitionEBRGetPartitionTableAttr(struct Library *PartitionBase, struct PartitionHandle *root, struct TagItem *tag)
{
    switch (tag->ti_Tag)
    {
    case PTT_MAXLEADIN:
        *((LONG *)tag->ti_Data) = root->de.de_BlocksPerTrack;
        return TRUE;
    }

    return 0;
}

static LONG PartitionEBRGetPartitionAttr(struct Library *PartitionBase, struct PartitionHandle *ph, struct TagItem *tag)
{
    struct EBRData *data = (struct EBRData *)ph->data;

    switch (tag->ti_Tag)
    {
    case PT_TYPE:
        PTYPE(tag->ti_Data)->id[0]  = data->type;
        PTYPE(tag->ti_Data)->id_len = 1;
        return TRUE;

    case PT_STARTBLOCK:
	*((ULONG *)tag->ti_Data) = data->block_no;
	return TRUE;

    case PT_ENDBLOCK:
	*((ULONG *)tag->ti_Data) = data->block_no + data->block_count - 1;
	return TRUE;
    }

    return 0;
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

const struct PTFunctionTable PartitionEBR =
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
    PartitionEBRGetPartitionTableAttr,
    NULL,
    PartitionEBRGetPartitionAttr,
    PartitionEBRSetPartitionAttrs,
    PartitionEBRPartitionTableAttrs,
    PartitionEBRPartitionAttrs,
    PartitionEBRDestroyPartitionTable,
    NULL
};
