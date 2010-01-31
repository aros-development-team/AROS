/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#define DEBUG 1
#endif
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

static LONG PartitionMBRCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct
{
    union
    {
        struct MBR mbr;
        struct FATBootSector bs;
        UBYTE space[root->de.de_SizeBlock << 2];
    }
    u;
}
blk;
BOOL isfat = TRUE;
ULONG sectorsize, clustersectors;
struct PCPartitionTable *pcpt;

    if (readBlock(PartitionBase, root, 0, &blk) == 0)
    {
        /* Check it doesn't look like a FAT boot sector */

        /* Valid sector size: 512, 1024, 2048, 4096 */
        sectorsize = AROS_LE2WORD(blk.u.bs.bpb_bytes_per_sect);
        if (
                sectorsize != 512 && sectorsize != 1024 && sectorsize != 2048 &&
                sectorsize != 4096
            )
            isfat = FALSE;

        /* Valid bpb_sect_per_clust: 1, 2, 4, 8, 16, 32, 64, 128 */
        clustersectors = blk.u.bs.bpb_sect_per_clust;
        if (
                (clustersectors & (clustersectors - 1)) != 0 ||
                clustersectors == 0 || clustersectors > 128
            )
            isfat = FALSE;

        /* Valid cluster size: 512, 1024, 2048, 4096, 8192, 16k, 32k, 64k */
        if (clustersectors * sectorsize > 64 * 1024)
            isfat = FALSE;

        if (blk.u.bs.bpb_media < 0xF0)
            isfat = FALSE;

        /* Check status bytes of all partition slots and block signature */
        pcpt = blk.u.mbr.pcpt;
        if (
                (AROS_LE2WORD(blk.u.mbr.magic) == 0xAA55) &&
                (((pcpt[0].status & 0x0F)==0) || (pcpt[0].status & 0x80)) &&
                (((pcpt[1].status & 0x0F)==0) || (pcpt[1].status & 0x80)) &&
                (((pcpt[2].status & 0x0F)==0) || (pcpt[2].status & 0x80)) &&
                (((pcpt[3].status & 0x0F)==0) || (pcpt[3].status & 0x80))
            )
        {
            if (root->root || isfat)
            {
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

static struct PartitionHandle *PartitionMBRNewHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        UBYTE position,
        struct PCPartitionTable *entry
    )
{
struct PartitionHandle *ph;
ULONG cylsecs;

    if (entry->first_sector != 0)
    {
        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
        struct MBRData *data;

            data = AllocMem(sizeof(struct MBRData), MEMF_PUBLIC);
            if (data)
            {
                cylsecs = root->de.de_BlocksPerTrack*root->de.de_Surfaces;
                data->entry = entry;
                data->position = position;
                ph->root = root;
                ph->bd = root->bd;
                ph->data = data;

                /* initialize DosEnvec */

                /* Check if partition starts and ends on a cylinder boundary */
                CopyMem(&root->de, &ph->de, sizeof(struct DosEnvec));
                if (
                        (AROS_LE2LONG(data->entry->first_sector) % cylsecs != 0) ||
                        (AROS_LE2LONG(data->entry->count_sector) % cylsecs != 0)
                    )
                {
                    /* Treat each track as a cylinder if possible */
                    ph->de.de_Surfaces = 1;
                    cylsecs = ph->de.de_BlocksPerTrack*ph->de.de_Surfaces;
                    if (
       	                (AROS_LE2LONG(data->entry->first_sector) % cylsecs != 0) ||
               	        (AROS_LE2LONG(data->entry->count_sector) % cylsecs != 0)
                    )
                    {
                        /* We can't. We could find the highest common factor of
                           first_sector and count_sector here, but currently we
                           simply use one block per cylinder */
                        ph->de.de_BlocksPerTrack = 1;
                        cylsecs = ph->de.de_BlocksPerTrack*ph->de.de_Surfaces;
                    }
                }
                ph->de.de_LowCyl = AROS_LE2LONG(data->entry->first_sector) / cylsecs;
                ph->de.de_HighCyl =
                    ph->de.de_LowCyl+
                    (AROS_LE2LONG(data->entry->count_sector)/cylsecs)-1;
                ph->de.de_TableSize = 10; // only until de_HighCyl
                ph->ln.ln_Pri = MBR_MAX_PARTITIONS-1-position;

                /* initialize DriveGeometry */
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

static LONG PartitionMBROpenPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;
struct MBR *mbr;
UBYTE i;

    mbr = AllocMem(root->de.de_SizeBlock<<2, MEMF_PUBLIC);
    if (mbr)
    {
        if (readBlock(PartitionBase, root, 0, mbr) == 0)
        {
            NEWLIST(&root->table->list);
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
#warning "FIXME: readBlock(0) and synchronize data"

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
            fillMem((BYTE *)mbr->pcpt, sizeof(mbr->pcpt), 0);
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

static struct PartitionHandle *PartitionMBRAddPartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{
struct TagItem *tag;

    tag = findTagItem(PT_DOSENVEC, taglist);
    if (tag)
    {
    struct PCPartitionTable *entry;
    struct PartitionHandle *ph;
    struct DosEnvec *de;
    WORD pos = -1, i;

        de =  (struct DosEnvec *)tag->ti_Data;
        tag = findTagItem(PT_POSITION, taglist);
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
            tag = findTagItem(PT_ACTIVE, taglist);
            if (tag)
                entry->status = tag->ti_Data ? 0x80 : 0;
            else
                entry->status = 0;
            tag = findTagItem(PT_TYPE, taglist);
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
                fillMem((BYTE *)entry, sizeof(struct PCPartitionTable), 0);
            return ph;
        }
    }
    return NULL;
}

static void PartitionMBRDeletePartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
struct MBRData *data;

    data = (struct MBRData *)ph->data;
    fillMem((BYTE *)data->entry, sizeof(struct PCPartitionTable), 0);
    Remove(&ph->ln);
    PartitionMBRFreeHandle(PartitionBase, ph);
}

static LONG PartitionMBRGetPartitionTableAttrs
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
        case PTT_RESERVED:
            *((LONG *)taglist[0].ti_Data) =
                root->de.de_BlocksPerTrack; /* One track */
            break;
        case PTT_MAX_PARTITIONS:
            *((LONG *)taglist[0].ti_Data) = MBR_MAX_PARTITIONS;
        }
        taglist++;
    }
    return 0;
}

static LONG PartitionMBRSetPartitionTableAttrs
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

static LONG PartitionMBRGetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct MBRData *data = (struct MBRData *)ph->data;

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

                ptype->id[0] = (LONG)data->entry->type;
                ptype->id_len = 1;
            }
            break;
        case PT_POSITION:
            *((LONG *)taglist[0].ti_Data) = (LONG)data->position;
            break;
        case PT_ACTIVE:
            *((LONG *)taglist[0].ti_Data) = data->entry->status & 0x80 ? 1 : 0;
            break;
        }
        taglist++;
    }
    return 0;
}

static LONG PartitionMBRSetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct MBRData *data = (struct MBRData *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_DOSENVEC:
            {
            struct DosEnvec *de;
                de = (struct DosEnvec *)taglist[0].ti_Data;
                CopyMem(de, &ph->de, sizeof(struct DosEnvec));
                PartitionMBRSetDosEnvec(ph->root, data->entry, de);
            }
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;

                data->entry->type = ptype->id[0];
            }
            break;
        case PT_POSITION:
            if (taglist[0].ti_Data != data->position)
            {
            struct PartitionHandle *node;
            struct PCPartitionTable *entry;

                node = (struct PartitionHandle *)ph->root->table->list.lh_Head;
                while (node->ln.ln_Succ)
                {
                    if (taglist[0].ti_Data == ((struct MBRData *)node->data)->position)
                        goto posbreak;
                    node = (struct PartitionHandle *)node->ln.ln_Succ;
                }
                data->position = taglist[0].ti_Data;
                entry = &((struct MBR *)ph->root->table->data)->pcpt[data->position];
                CopyMem(data->entry, entry, sizeof(struct PCPartitionTable));
                fillMem((BYTE *)data->entry, sizeof(struct PCPartitionTable), 0);
                data->entry = entry;
                ph->ln.ln_Pri = MBR_MAX_PARTITIONS-1-data->position;
                Remove(&ph->ln);
                Enqueue(&ph->root->table->list, &ph->ln);
posbreak:
            ;
            }
            break;
        case PT_ACTIVE:
            if (taglist[0].ti_Data)
                data->entry->status |= 0x80;
            else
                data->entry->status &= ~0x80;
            break;
        }
        taglist++;
    }
    return 0;
}

static struct PartitionAttribute PartitionMBRPartitionTableAttrs[]=
{
    {PTTA_TYPE,           PLAM_READ},
    {PTTA_RESERVED,       PLAM_READ},
    {PTTA_MAX_PARTITIONS, PLAM_READ},
    {PTTA_DONE,           0}
};

static struct PartitionAttribute *PartitionMBRQueryPartitionTableAttrs(struct Library *PartitionBase)
{
    return PartitionMBRPartitionTableAttrs;
}

static struct PartitionAttribute PartitionMBRPartitionAttrs[]=
{
    {PTA_GEOMETRY, PLAM_READ},
    {PTA_TYPE,     PLAM_READ | PLAM_WRITE},
    {PTA_POSITION, PLAM_READ | PLAM_WRITE},
    {PTA_ACTIVE,   PLAM_READ | PLAM_WRITE},
    {PTA_DONE,     0}
};

struct PartitionAttribute *PartitionMBRQueryPartitionAttrs(struct Library *PartitionBase)
{
    return PartitionMBRPartitionAttrs;
}

static ULONG PartitionMBRDestroyPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct MBR *mbr;

    mbr = root->table->data;
    fillMem((BYTE *)mbr->pcpt, sizeof(mbr->pcpt), 0);
    /* deleting the magic value will invalidate the
     * partition table so it cannot be opened again
     */
    mbr->magic = 0;
    if (writeBlock(PartitionBase, root, 0, root->table->data))
        return 1;
    return 0;
}

struct PTFunctionTable PartitionMBR =
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
    PartitionMBRGetPartitionTableAttrs,
    PartitionMBRSetPartitionTableAttrs,
    PartitionMBRGetPartitionAttrs,
    PartitionMBRSetPartitionAttrs,
    PartitionMBRQueryPartitionTableAttrs,
    PartitionMBRQueryPartitionAttrs,
    PartitionMBRDestroyPartitionTable
};

