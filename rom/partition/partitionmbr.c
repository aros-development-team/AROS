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

struct MBRData {
    struct PCPartitionTable *entry;
    UBYTE position;
};

struct FATBootSector {
    UBYTE bs_jmp_boot[3];
    UBYTE bs_oem_name[8];
    UWORD bpb_bytes_per_sect;
    UBYTE bpb_sect_per_clust;
    UWORD bpb_rsvd_sect_count;
    UBYTE bpb_num_fats;
    UWORD bpb_root_entries_count;
    UWORD bpb_total_sectors_16;
    UBYTE bpb_media;
    UWORD bpb_fat_size_16;
    UWORD bpb_sect_per_track;
    UWORD bpb_num_heads;
    ULONG bpb_hidden_sect;
    ULONG bpb_total_sectors_32;

    union {
        struct {
            UBYTE bs_drvnum;
            UBYTE bs_reserved1;
            UBYTE bs_bootsig;
            ULONG bs_volid;
            UBYTE bs_vollab[11];
            UBYTE bs_filsystype[8];
        } __attribute__ ((__packed__)) fat16;

        struct {
            ULONG bpb_fat_size_32;
            UWORD bpb_extflags;
            UWORD bpb_fs_verion;
            ULONG bpb_root_cluster;
            UWORD bpb_fs_info;
            UWORD bpb_back_bootsec;
            UBYTE bpb_reserved[12];
            UBYTE bs_drvnum;
            UBYTE bs_reserved1;
            UBYTE bs_bootsig;
            ULONG bs_volid;
            UBYTE bs_vollab[11];
            UBYTE bs_filsystype[8];
        } __attribute__ ((__packed__)) fat32;
    } type;
    UBYTE pad[420];
    UBYTE bpb_signature[2];
} __attribute__ ((__packed__));

struct rootblock
{
    union
    {
        struct MBR mbr;
        struct FATBootSector bs;
    }
    u;
};

LONG MBRCheckPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root, void *buffer)
{
    struct rootblock *blk = buffer;
    LONG res = 0;

    if (readBlock(PartitionBase, root, 0, blk) == 0)
    {
        /* Check it doesn't look like a FAT boot sector */
	ULONG sectorsize, clustersectors;

        /* Valid sector size: 512, 1024, 2048, 4096 */
        sectorsize = AROS_LE2WORD(blk->u.bs.bpb_bytes_per_sect);
        if (sectorsize != 512 && sectorsize != 1024 && sectorsize != 2048 && sectorsize != 4096)
            res = 1;

        /* Valid bpb_sect_per_clust: 1, 2, 4, 8, 16, 32, 64, 128 */
        clustersectors = blk->u.bs.bpb_sect_per_clust;
        if ((clustersectors & (clustersectors - 1)) != 0 || clustersectors == 0 || clustersectors > 128)
            res = 1;

        /* Valid cluster size: 512, 1024, 2048, 4096, 8192, 16k, 32k, 64k */
        if (clustersectors * sectorsize > 64 * 1024)
            res = 1;

        if (blk->u.bs.bpb_media < 0xF0)
            res = 1;

	if (res)
	{
	    struct PCPartitionTable *pcpt = blk->u.mbr.pcpt;

	    /* Check status bytes of all partition slots and block signature */
            if ((AROS_LE2WORD(blk->u.mbr.magic) != MBR_MAGIC) ||
                (!MBR_STATUS_VALID(pcpt[0].status)) || (!MBR_STATUS_VALID(pcpt[1].status)) || 
            	(!MBR_STATUS_VALID(pcpt[2].status)) || (!MBR_STATUS_VALID(pcpt[3].status)))
            {
            	res = 0;
            }
        }
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

static struct PartitionHandle *PartitionMBRNewHandle(struct Library *PartitionBase, struct PartitionHandle *root, UBYTE position, struct PCPartitionTable *entry)
{
    struct PartitionHandle *ph;

    if (entry->first_sector != 0)
    {
        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
        struct MBRData *data;

            data = AllocMem(sizeof(struct MBRData), MEMF_PUBLIC);
            if (data)
            {
                data->entry    = entry;
                data->position = position;
                ph->data = data;

                /* initialize DosEnvec and DriveGeometry */
                initPartitionHandle(root, ph, AROS_LE2LONG(data->entry->first_sector), AROS_LE2LONG(data->entry->count_sector));

		/* Map type ID to a DOSType */
		setDosType(&ph->de, MBR_FindDosType(data->entry->type));

		/* Set position as priority */
                ph->ln.ln_Pri = MBR_MAX_PARTITIONS - 1 - position;
                return ph;
            }
            FreeMem(ph, sizeof(struct PartitionHandle));
        }
    }
    return NULL;
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

static void PartitionMBRFreeHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    ClosePartitionTable(ph);
    FreeMem(ph->data, sizeof(struct MBRData));
    FreeMem(ph, sizeof(struct PartitionHandle));
}

static void PartitionMBRClosePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;

    while ((ph = (struct PartitionHandle *)RemTail(&root->table->list)))
        PartitionMBRFreeHandle(PartitionBase, ph);
    FreeMem(root->table->data, root->de.de_SizeBlock<<2);
}

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

            memset(mbr->pcpt, 0, sizeof(mbr->pcpt));
            mbr->magic = AROS_WORD2LE(0xAA55);

            NEWLIST(&ph->table->list);
            return 0;
        }
        FreeMem(mbr, ph->de.de_SizeBlock<<2);
    }
    return 1;
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
ULONG track;
ULONG cyl;

    /* Store LBA start block and block count */

    entry->first_sector = AROS_LONG2LE(sector - relative_sector);
    entry->count_sector = AROS_LONG2LE(count);

    /* Store CHS-address of start block. The upper two bits of the cylinder
       number are stored in the upper two bits of the sector field */

    track = sector/root->de.de_BlocksPerTrack;
    cyl = track/root->de.de_Surfaces;
    if (cyl<1024)
    {
        entry->start_head = track % root->de.de_Surfaces;
        entry->start_sector =
            ((sector % root->de.de_BlocksPerTrack) + 1)
            | ((cyl & 0x300) >> 2);
        entry->start_cylinder = (cyl & 0xFF);
    }
    else
    {
        entry->start_head = 0xFE;
        entry->start_sector = 0xFF;
        entry->start_cylinder = 0xFF;
    }

    /* Store CHS-address of last block */

    sector += count - 1;
    track = sector/root->de.de_BlocksPerTrack;
    cyl = track/root->de.de_Surfaces;
    if (cyl<1024)
    {
        entry->end_head = track % root->de.de_Surfaces;
        entry->end_sector = ((sector % root->de.de_BlocksPerTrack) + 1)
            | ((cyl & 0x300)>>2);
        entry->end_cylinder = (cyl & 0xFF);
    }
    else
    {
        entry->end_head = 0xFE;
        entry->end_sector = 0xFF;
        entry->end_cylinder = 0xFF;
    }
}

static void PartitionMBRSetDosEnvec
    (
        struct PartitionHandle *root,
        struct PCPartitionTable *entry,
        struct DosEnvec *de
    )
{
ULONG sector, count;

    sector = de->de_LowCyl * de->de_Surfaces * de->de_BlocksPerTrack;
    count = (de->de_HighCyl - de->de_LowCyl + 1) *
            de->de_Surfaces *
            de->de_BlocksPerTrack;
    PartitionMBRSetGeometry(root, entry, sector, count, 0);
}

static struct PartitionHandle *PartitionMBRAddPartition(struct Library *PartitionBase, struct PartitionHandle *root, struct TagItem *taglist)
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
                memset(entry, 0, sizeof(struct PCPartitionTable));

            return ph;
        }
    }
    return NULL;
}

static void PartitionMBRDeletePartition(struct Library *PartitionBase, struct PartitionHandle *ph)
{
    struct MBRData *data = (struct MBRData *)ph->data;

    memset(data->entry, 0, sizeof(struct PCPartitionTable));

    Remove(&ph->ln);
    PartitionMBRFreeHandle(PartitionBase, ph);
}

static LONG PartitionMBRGetPartitionTableAttr(struct Library *PartitionBase, struct PartitionHandle *root, struct TagItem *tag)
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

static LONG PartitionMBRGetPartitionAttr(struct Library *PartitionBase, struct PartitionHandle *ph, struct TagItem *tag)
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
	*((ULONG *)tag->ti_Data) = AROS_LE2LONG(data->entry->first_sector);
	return TRUE;

    case PT_ENDBLOCK:
	*((ULONG *)tag->ti_Data) = AROS_LE2LONG(data->entry->first_sector) + AROS_LE2LONG(data->entry->count_sector) - 1;
	return TRUE;
    }

    /* Everything else gets default values */
    return 0;
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
                memset(data->entry, 0, sizeof(struct PCPartitionTable));
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

static ULONG PartitionMBRDestroyPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    struct MBR *mbr = root->table->data;

    memset(mbr->pcpt, 0, sizeof(mbr->pcpt));
    /* deleting the magic value will invalidate the
     * partition table so it cannot be opened again
     */
    mbr->magic = 0;
    if (writeBlock(PartitionBase, root, 0, root->table->data))
        return 1;
    return 0;
}

const struct PTFunctionTable PartitionMBR =
{
    PHPTT_MBR,
    "PC-MBR",
    PartitionMBRCheckPartitionTable,
    PartitionMBROpenPartitionTable,
    PartitionMBRClosePartitionTable,
    PartitionMBRWritePartitionTable,
    PartitionMBRCreatePartitionTable,
    PartitionMBRAddPartition,
    PartitionMBRDeletePartition,
    PartitionMBRGetPartitionTableAttr,
    NULL,
    PartitionMBRGetPartitionAttr,
    PartitionMBRSetPartitionAttrs,
    PartitionMBRPartitionTableAttrs,
    PartitionMBRPartitionAttrs,
    PartitionMBRDestroyPartitionTable,
    NULL
};

