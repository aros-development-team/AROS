/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <clib/macros.h>

#include <string.h>   
#include <ctype.h>

#include "fat_fs.h"
#include "fat_protos.h"

/* helper function to get the location of a fat entry for a cluster. it used
 * to be a define until it got too crazy */
static inline UBYTE *GetFatEntryPtr(struct FSSuper *sb, ULONG offset) {
    ULONG entry_cache_block = offset >> sb->fat_cachesize_bits;
    ULONG entry_cache_offset = offset & (sb->fat_cachesize - 1);

    /* if the target cluster is not within the currently loaded chunk of fat,
     * we need to get the right data in */
    if (sb->fat_cache_block != entry_cache_block) {
        D(bug("[fat] loading %ld FAT sectors starting at sector %ld\n", sb->fat_blocks_count, entry_cache_block));
        /* put the old ones back */
        cache_put_blocks(glob->cache, sb->fat_blocks, sb->fat_blocks_count, 0);

        /* load some more */
        cache_get_blocks(glob->cache,
                         glob->diskioreq->iotd_Req.io_Device,
                         glob->diskioreq->iotd_Req.io_Unit,
                         sb->first_fat_sector +
                            (entry_cache_block << (sb->fat_cachesize_bits - sb->sectorsize_bits)),
                         sb->fat_blocks_count,
                         0,
                         sb->fat_blocks);

        /* remember where we are for next time */
        sb->fat_cache_block = entry_cache_block;
    }

    /* compute the pointer location and return it */
    return sb->fat_blocks[entry_cache_offset >> sb->sectorsize_bits]->data +
           (entry_cache_offset & (sb->sectorsize-1));
}

static ULONG GetFat12Entry(struct FSSuper *sb, ULONG n) {
    UWORD val = AROS_LE2WORD(*((UWORD *) GetFatEntryPtr(sb, n + n/2)));

    if (n & 1)
        val >>= 4;
    else
        val &= 0x0FFF;

    return val;
}

static ULONG GetFat16Entry(struct FSSuper *sb, ULONG n) {
    return AROS_LE2WORD(*((UWORD *) GetFatEntryPtr(sb, n << 1)));
}

static ULONG GetFat32Entry(struct FSSuper *sb, ULONG n) {
    return AROS_LE2LONG(*((ULONG *) GetFatEntryPtr(sb, n << 2)));
}
 
LONG ReadFATSuper(struct FSSuper *sb ) {
    LONG err;
    UBYTE raw[512];
    struct FATBootSector *boot = (struct FATBootSector *) &raw;
    BOOL invalid = FALSE;

    D(bug("[fat] reading boot sector\n"));

    /*
     * Read the boot sector. We go direct because we don't have a cache yet,
     * and can't create one until we know the sector size, which is held in
     * the boot sector. In practice it doesn't matter - we're going use this
     * once and once only.
     */

    glob->diskioreq->iotd_Req.io_Command = CMD_READ;
    glob->diskioreq->iotd_Req.io_Offset = 0;
    glob->diskioreq->iotd_Req.io_Length = 512;
    glob->diskioreq->iotd_Req.io_Data = &raw;
    glob->diskioreq->iotd_Req.io_Flags = IOF_QUICK;

    if ((err = DoIO((struct IORequest *) glob->diskioreq)) != 0) {
        D(bug("[fat] couldn't read boot block (%ld)\n", err));
        return err;
    }

    kprintf("\tBoot sector:\n");

    sb->sectorsize = AROS_LE2WORD(boot->bpb_bytes_per_sect);
    sb->sectorsize_bits = log2(sb->sectorsize);
    kprintf("\tSectorSize = %ld\n", sb->sectorsize);
    kprintf("\tSectorSize Bits = %ld\n", sb->sectorsize_bits);

    sb->cluster_sectors = boot->bpb_sect_per_clust;
    sb->clustersize = sb->sectorsize * boot->bpb_sect_per_clust;
    sb->clustersize_bits = log2(sb->clustersize);
    sb->cluster_sectors_bits = sb->clustersize_bits - sb->sectorsize_bits;

    kprintf("\tSectorsPerCluster = %ld\n", (ULONG)boot->bpb_sect_per_clust);
    kprintf("\tClusterSize = %ld\n", sb->clustersize);
    kprintf("\tClusterSize Bits = %ld\n", sb->clustersize_bits);
    kprintf("\tCluster Sectors Bits = %ld\n", sb->cluster_sectors_bits);

    sb->first_fat_sector = AROS_LE2WORD(boot->bpb_rsvd_sect_count);
    kprintf("\tFirst FAT Sector = %ld\n", sb->first_fat_sector);

    if (boot->bpb_fat_size_16 != 0)
        sb->fat_size = AROS_LE2WORD(boot->bpb_fat_size_16);
    else
        sb->fat_size = AROS_LE2LONG(boot->type.fat32.bpb_fat_size_32);
    kprintf("\tFAT Size = %ld\n", sb->fat_size);

    if (boot->bpb_total_sectors_16 != 0)
        sb->total_sectors = AROS_LE2WORD(boot->bpb_total_sectors_16);
    else
        sb->total_sectors = AROS_LE2LONG(boot->bpb_total_sectors_32);
    kprintf("\tTotal Sectors = %ld\n", sb->total_sectors);

    sb->rootdir_sectors = ((AROS_LE2WORD(boot->bpb_root_entries_count) * sizeof(struct FATDirEntry)) + (sb->sectorsize - 1)) >> sb->sectorsize_bits;
    kprintf("\tRootDir Sectors = %ld\n", sb->rootdir_sectors);

    sb->data_sectors = sb->total_sectors - (sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size) + sb->rootdir_sectors);
    kprintf("\tData Sectors = %ld\n", sb->data_sectors);

    sb->clusters_count = sb->data_sectors >> sb->cluster_sectors_bits;
    kprintf("\tClusters Count = %ld\n", sb->clusters_count);

    sb->first_rootdir_sector = sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size);
    kprintf("\tFirst RootDir Sector = %ld\n", sb->first_rootdir_sector);

    sb->first_data_sector = sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size) + sb->rootdir_sectors;
    kprintf("\tFirst Data Sector = %ld\n", sb->first_data_sector);

    sb->free_clusters = 0xffffffff;

    /* check if disk is in fact FAT filesystem */          

    /* valid sector size: 512, 1024, 2048, 4096 */
    if (sb->sectorsize != 512 && sb->sectorsize != 1024 && sb->sectorsize != 2048 && sb->sectorsize != 4096)
        invalid = TRUE;

    /* valid bpb_sect_per_clust: 1, 2, 4, 8, 16, 32, 64, 128 */
    if ((boot->bpb_sect_per_clust & (boot->bpb_sect_per_clust - 1)) != 0 || boot->bpb_sect_per_clust == 0 || boot->bpb_sect_per_clust > 128)
        invalid = TRUE;

    /* valid cluster size: 512, 1024, 2048, 4096, 8192, 16k, 32k, 64k */
    if (sb->clustersize > 64 * 1024)
        invalid = TRUE;

    if (sb->first_fat_sector == 0)
        invalid = TRUE;

    if (boot->bpb_num_fats == 0)
        invalid = TRUE;

    if (boot->bpb_media < 0xF0)
        invalid = TRUE;

    /* FAT "signature" */
    if (raw[510] != 0x55 || raw[511] != 0xaa)
        invalid = TRUE;
 
    if (invalid) {
        kprintf("\tInvalid FAT Boot Sector\n");
        return ERROR_NOT_A_DOS_DISK;
    }
 
    if ((err = InitDevice(glob->fssm, sb->sectorsize) != 0))
        return err;

    if (sb->clusters_count < 4085) {
        kprintf("\tFAT12 filesystem detected\n");
        sb->type = 12;
        sb->eoc_mark = 0x0FF8;
        sb->func_get_fat_entry = GetFat12Entry;
    }
    else if (sb->clusters_count < 65525) {
        kprintf("\tFAT16 filesystem detected\n");
        sb->type = 16;
        sb->eoc_mark = 0xFFF8;
        sb->func_get_fat_entry = GetFat16Entry;
    }
    else {
        kprintf("\tFAT32 filesystem detected\n");
        sb->type = 32;
        sb->eoc_mark = 0x0FFFFFF8;
        sb->func_get_fat_entry = GetFat32Entry;
    }

    /* setup the FAT cache and load the first blocks */
    sb->fat_cachesize = 4096;
    sb->fat_cachesize_bits = log2(sb->fat_cachesize);
    sb->fat_cache_block = 0;

    sb->fat_blocks_count = MIN(sb->fat_size, sb->fat_cachesize >> sb->sectorsize_bits);
    sb->fat_blocks = AllocVecPooled(glob->mempool, sizeof(struct cache_block *) * sb->fat_blocks_count);

    cache_get_blocks(glob->cache, glob->diskioreq->iotd_Req.io_Device, glob->diskioreq->iotd_Req.io_Unit, sb->first_fat_sector, sb->fat_blocks_count, 0, sb->fat_blocks);

    if (sb->type != 32) { /* FAT 12/16 */
        /* setup volume id */
        sb->volume_id = AROS_LE2LONG(boot->type.fat16.bs_volid);

        /* location of root directory */
        sb->rootdir_cluster = 0;
        sb->rootdir_sector = sb->first_rootdir_sector;
    }
    else {
        /* setup volume id */
        sb->volume_id = AROS_LE2LONG(boot->type.fat32.bs_volid);
 
        /* location of root directory */
        sb->rootdir_cluster = AROS_LE2LONG(boot->type.fat32.bpb_root_cluster);
        sb->rootdir_sector = 0;
    }

    D(bug("[fat] rootdir at cluster %ld sector %ld\n", sb->rootdir_cluster, sb->rootdir_sector));

    if (GetVolumeInfo(sb, &(sb->volume)) != 0) {
        LONG i;
        UBYTE *uu = (void *)&sb->volume_id;

        for (i=1; i<10;) {
            int d;

            if (i==5)
                sb->volume.name[i++]='-';

            d = (*uu) & 0x0f;
            sb->volume.name[i++] = (d < 10) ? '0' + d : 'A' - 10 + d;
            d = ((*uu) & 0xf0)>>4;
            sb->volume.name[i++] = (d < 10) ? '0' + d : 'A' - 10 + d;

            uu++;
        }

        sb->volume.name[i] = '\0';
        sb->volume.name[0] = 9;
    }

    kprintf("\tFAT Filesystem succesfully detected.\n");

    return 0;
}

LONG GetVolumeInfo(struct FSSuper *sb, struct VolumeInfo *volume) {
    struct DirHandle dh;
    struct DirEntry de;
    LONG err;
    int i;

    D(bug("[fat] searching root directory for volume name\n"));

    /* search the directory for the volume id entry. it would've been nice to
     * just use GetNextDirEntry but I didn't want a flag or something to tell
     * it not to skip the volume name */
    InitDirHandle(sb, 0, &dh);

    while ((err = GetDirEntry(&dh, dh.cur_index + 1, &de)) == 0) {

        /* match the volume id entry */
        if ((de.e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_VOLUME_ID) {
            D(bug("[fat] found volume id entry %ld\n", dh.cur_index));

            /* copy the name in. volume->name is a BSTR */

            volume->name[1] = de.e.entry.name[0];
            volume->name[12] = '\0';

            for (i = 1; i < 11; i++)
                volume->name[i+1] = tolower(de.e.entry.name[i]);

            for (i = 10; i > 1; i--)
                if (volume->name[i+1] == ' ')
                    volume->name[i+1] = '\0';

            volume->name[0] = strlen(&(volume->name[1]));

            /* get the volume creation date date too */
            ConvertDate(de.e.entry.create_date, de.e.entry.create_time, &volume->create_time);

            D(bug("[fat] volume name is '%s'\n", &(volume->name[1])));

            return 0;
        }

        /* bail out if we hit the end of the dir */
        if (de.e.entry.name[0] == 0x00) {
            D(bug("[fat] found end-of-directory marker, volume name entry not found\n"));
            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }
    }

    ReleaseDirHandle(&dh);
    return err;
}

void FreeFATSuper(struct FSSuper *sb) {
    kprintf("\tRemoving Super Block from memory\n");
    FreeVecPooled(glob->mempool, sb->fat_blocks);
    sb->fat_blocks = NULL;
}

LONG CompareFATSuper(struct FSSuper *s1, struct FSSuper *s2) {
    LONG res;

    if ((res = memcmp(s1->volume.name, s2->volume.name, s1->volume.name[0])) != 0)
        return res;

    return s1->volume_id - s2->volume_id;
}


/* see how many unused clusters are available */
void CountFreeClusters(struct FSSuper *sb) {
    ULONG cluster = 0;
    ULONG free = 0;

    /* don't calculate this if we already have it */
    if (sb->free_clusters != 0xffffffff)
        return;

    /* loop over all the data clusters */
    for (cluster = 2; cluster < sb->clusters_count + 2; cluster++)

        /* record the free ones */
        if (NEXT_CLUSTER(sb, cluster) == 0)
            free++;

    /* put the value away for later */
    sb->free_clusters = free;

    kprintf("\tfree clusters: %ld\n", free);
}

static const UWORD mdays[] = { 0, 31, 59, 90, 120, 151, 181, 212, 143, 273, 304, 334 };

void ConvertDate(UWORD date, UWORD time, struct DateStamp *ds) {
    UBYTE year, month, day, hours, mins, secs;
    UBYTE nleap;

    /* date bits: yyyy yyym mmmd dddd */
    year = (date & 0xfe00) >> 9;    /* bits 15-9 */
    month = (date & 0x01e0) >> 5;   /* bits 8-5 */
    day = date & 0x001f;            /* bits 4-0 */

    /* time bits: hhhh hmmm mmms ssss */
    hours = (time & 0xf800) >> 11;  /* bits 15-11 */
    mins = (time & 0x07e0) >> 5;    /* bits 8-5 */
    secs = time & 0x001f;           /* bits 4-0 */

    D(bug("[fat] convert date: year %d month %d day %d hours %d mins %d secs %d\n", year, month, day, hours, mins, secs));

    /* number of leap years in before this year. note this is only dividing by
     * four, which is fine because FAT dates range 1980-2107. The only year in
     * that range that is divisible by four but not a leap year is 2100. If
     * this code is still being used then, feel free to fix it :) */
    nleap = year >> 2;
    
    /* if this year is a leap year and its March or later, adjust for this
     * year too */
    if (year & 0x03 && month >= 3)
        nleap++;

    /* calculate days since 1978-01-01 (DOS epoch):
     *   730 days in 1978+1979, getting us to the FAT epoch 1980-01-01
     *   years * 365 days
     *   leap days
     *   days in all the months before this one
     *   day of this month */
    ds->ds_Days = 730 + year * 365 + nleap + mdays[month-1] + day-1;

    /* minutes since midnight */
    ds->ds_Minute = hours * 60 + mins;

    /* 1/50 sec ticks. FAT dates are 0-29, so we have to multiply them by two
     * as well */
    ds->ds_Tick = (secs << 1) * 50;

    D(bug("[fat] convert date: days %ld minutes %ld ticks %ld\n", ds->ds_Days, ds->ds_Minute, ds->ds_Tick));
}
