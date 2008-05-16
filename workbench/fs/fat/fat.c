/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2008 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <aros/macros.h>
#include <exec/errors.h>
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
#include "timer.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

/* helper function to get the location of a fat entry for a cluster. it used
 * to be a define until it got too crazy */
static UBYTE *GetFatEntryPtr(struct FSSuper *sb, ULONG offset, struct cache_block **rb) {
    ULONG entry_cache_block = offset >> sb->fat_cachesize_bits;
    ULONG entry_cache_offset = offset & (sb->fat_cachesize - 1);

    /* if the target cluster is not within the currently loaded chunk of fat,
     * we need to get the right data in */
    if (sb->fat_cache_block != entry_cache_block) {
        D(bug("[fat] loading %ld FAT sectors starting at sector %ld\n", sb->fat_blocks_count, entry_cache_block << (sb->fat_cachesize_bits - sb->sectorsize_bits)));
        /* put the old ones back */
        if (sb->fat_cache_block != 0xffffffff)
            cache_put_blocks(sb->cache, sb->fat_blocks, sb->fat_blocks_count, 0);

        /* load some more */
        cache_get_blocks(sb->cache,
                         sb->first_device_sector + sb->first_fat_sector +
                            (entry_cache_block << (sb->fat_cachesize_bits - sb->sectorsize_bits)),
                         sb->fat_blocks_count,
                         0,
                         sb->fat_blocks);

        /* remember where we are for next time */
        sb->fat_cache_block = entry_cache_block;
    }

    /* give the block back if they asked for it (needed to mark the block
     * dirty if they're writing */
    if (rb != NULL)
        *rb = sb->fat_blocks[entry_cache_offset >> sb->sectorsize_bits];

    /* compute the pointer location and return it */
    return sb->fat_blocks[entry_cache_offset >> sb->sectorsize_bits]->data +
           (entry_cache_offset & (sb->sectorsize-1));
}

/* FAT12 has, as the name suggests, 12-bit FAT entries. This means that two
 * entries are condensed into three bytes, like so:
 *
 * entry: aaaaaaaa aaaabbbb bbbbbbbb
 * bytes: xxxxxxxx xxxxxxxx xxxxxxxx
 *
 * To get at the entry we want, we find and grab the word starting at either
 * byte 0 or 1 of the three-byte set, then shift up or down as needed. FATdoc
 * 1.03 p16-17 describes the method
 * 
 * The only tricky bit is if the word falls such that the first byte is the
 * last byte of the block and the second byte is the first byte of the next
 * block. Since our block data are stored within cache block structures, a
 * simple cast won't do (hell, the second block may not even be in memory if
 * we're at the end of the FAT cache). So we get it a byte at a time, and
 * build the word ourselves.
 */
static ULONG GetFat12Entry(struct FSSuper *sb, ULONG n) {
    ULONG offset = n + n/2;
    UWORD val;

    if ((offset & (sb->sectorsize-1)) == sb->sectorsize-1) {
        D(bug("[fat] fat12 cluster pair on block boundary, compensating\n"));

        val = *GetFatEntryPtr(sb, offset + 1, NULL) << 8;
        val |= *GetFatEntryPtr(sb, offset, NULL);
	val = AROS_LE2WORD(val);
    }
    else
        val = AROS_LE2WORD(*((UWORD *) GetFatEntryPtr(sb, offset, NULL)));

    if (n & 1)
        val >>= 4;
    else
        val &= 0xfff;

    return val;
}

/*
 * FAT16 and FAT32, on the other hand, have nice neat entry widths, so simple
 * word/long casts are fine. There's also no chance that the entry can be
 * split across blocks. Why can't everything be this simple?
 */
static ULONG GetFat16Entry(struct FSSuper *sb, ULONG n) {
    return AROS_LE2WORD(*((UWORD *) GetFatEntryPtr(sb, n << 1, NULL)));
}

static ULONG GetFat32Entry(struct FSSuper *sb, ULONG n) {
    return AROS_LE2LONG(*((ULONG *) GetFatEntryPtr(sb, n << 2, NULL)));
}

static void SetFat12Entry(struct FSSuper *sb, ULONG n, ULONG val) {
    struct cache_block *b;
    ULONG offset = n + n/2;
    BOOL boundary = FALSE;
    UWORD *fat, newval;

    if ((offset & (sb->sectorsize-1)) == sb->sectorsize-1) {
        boundary = TRUE;

        D(bug("[fat] fat12 cluster pair on block boundary, compensating\n"));

        newval = *GetFatEntryPtr(sb, offset + 1, NULL) << 8;
        newval |= *GetFatEntryPtr(sb, offset, NULL);
	newval = AROS_LE2WORD(newval);
    }
    else {
        fat = (UWORD *) GetFatEntryPtr(sb, offset, &b);
        newval = AROS_LE2WORD(*fat);
    }

    if (n & 1) {
        val <<= 4;
        newval = (newval & 0xf) | val;
    }
    else {
        newval = (newval & 0xf000) | val;
    }

    if (boundary) {
        /* XXX ideally we'd mark both blocks dirty at the same time or only do
         * it once if they're the same block. unfortunately b is essentially
         * invalid after a call to GetFatEntryPtr, as it may have swapped the
         * previous cache out. This is probably safe enough. */
        *GetFatEntryPtr(sb, offset+1, &b) = newval >> 8;
        cache_mark_block_dirty(sb->cache, b);
        *GetFatEntryPtr(sb, offset, &b) = newval & 0xff;
        cache_mark_block_dirty(sb->cache, b);
    }
    else {
        *fat = AROS_WORD2LE(newval);
        cache_mark_block_dirty(sb->cache, b);
    }
}

static void SetFat16Entry(struct FSSuper *sb, ULONG n, ULONG val) {
    struct cache_block *b;

    *((UWORD *) GetFatEntryPtr(sb, n << 1, &b)) = AROS_WORD2LE((UWORD) val);

    cache_mark_block_dirty(sb->cache, b);
}

static void SetFat32Entry(struct FSSuper *sb, ULONG n, ULONG val) {
    struct cache_block *b;
    ULONG *fat = (ULONG *) GetFatEntryPtr(sb, n << 2, &b);

    *fat = (*fat & 0xf0000000) | val;

    cache_mark_block_dirty(sb->cache, b);
}

LONG ReadFATSuper(struct FSSuper *sb ) {
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);
    LONG err;
    ULONG bsize = de->de_SizeBlock * 4;
    struct FATBootSector *boot;
    BOOL invalid = FALSE;
    ULONG end;

    D(bug("[fat] reading boot sector\n"));

    boot = AllocMem(bsize, MEMF_ANY);
    if (!boot)
	return ERROR_NO_FREE_STORE;
    /*
     * Read the boot sector. We go direct because we don't have a cache yet,
     * and can't create one until we know the sector size, which is held in
     * the boot sector. In practice it doesn't matter - we're going use this
     * once and once only.
     */

    sb->first_device_sector = de->de_BlocksPerTrack *
                              de->de_LowCyl;

    D(bug("[fat] boot sector at sector %ld\n", sb->first_device_sector));

    if ((err = AccessDisk(FALSE, sb->first_device_sector, 1, bsize, (UBYTE *)boot)) != 0) {
        D(bug("[fat] couldn't read boot block (%ld)\n", err));
	FreeMem(boot, bsize);
        return err;
    }

    D(bug("\tBoot sector:\n"));

    sb->sectorsize = AROS_LE2WORD(boot->bpb_bytes_per_sect);
    sb->sectorsize_bits = log2(sb->sectorsize);
    D(bug("\tSectorSize = %ld\n", sb->sectorsize));
    D(bug("\tSectorSize Bits = %ld\n", sb->sectorsize_bits));

    sb->cluster_sectors = boot->bpb_sect_per_clust;
    sb->clustersize = sb->sectorsize * boot->bpb_sect_per_clust;
    sb->clustersize_bits = log2(sb->clustersize);
    sb->cluster_sectors_bits = sb->clustersize_bits - sb->sectorsize_bits;

    D(bug("\tSectorsPerCluster = %ld\n", (ULONG)boot->bpb_sect_per_clust));
    D(bug("\tClusterSize = %ld\n", sb->clustersize));
    D(bug("\tClusterSize Bits = %ld\n", sb->clustersize_bits));
    D(bug("\tCluster Sectors Bits = %ld\n", sb->cluster_sectors_bits));

    sb->first_fat_sector = AROS_LE2WORD(boot->bpb_rsvd_sect_count);
    D(bug("\tFirst FAT Sector = %ld\n", sb->first_fat_sector));

    if (boot->bpb_fat_size_16 != 0)
        sb->fat_size = AROS_LE2WORD(boot->bpb_fat_size_16);
    else
        sb->fat_size = AROS_LE2LONG(boot->type.fat32.bpb_fat_size_32);
    D(bug("\tFAT Size = %ld\n", sb->fat_size));

    if (boot->bpb_total_sectors_16 != 0)
        sb->total_sectors = AROS_LE2WORD(boot->bpb_total_sectors_16);
    else
        sb->total_sectors = AROS_LE2LONG(boot->bpb_total_sectors_32);
    D(bug("\tTotal Sectors = %ld\n", sb->total_sectors));

    sb->rootdir_sectors = ((AROS_LE2WORD(boot->bpb_root_entries_count) * sizeof(struct FATDirEntry)) + (sb->sectorsize - 1)) >> sb->sectorsize_bits;
    D(bug("\tRootDir Sectors = %ld\n", sb->rootdir_sectors));

    sb->data_sectors = sb->total_sectors - (sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size) + sb->rootdir_sectors);
    D(bug("\tData Sectors = %ld\n", sb->data_sectors));

    sb->clusters_count = sb->data_sectors >> sb->cluster_sectors_bits;
    D(bug("\tClusters Count = %ld\n", sb->clusters_count));

    sb->first_rootdir_sector = sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size);
    D(bug("\tFirst RootDir Sector = %ld\n", sb->first_rootdir_sector));

    sb->first_data_sector = sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size) + sb->rootdir_sectors;
    D(bug("\tFirst Data Sector = %ld\n", sb->first_data_sector));

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
    if (boot->bpb_signature[0] != 0x55 || boot->bpb_signature[1] != 0xaa)
        invalid = TRUE;
 
    if (invalid) {
        D(bug("\tInvalid FAT Boot Sector\n"));
	FreeMem(boot, bsize);
        return ERROR_NOT_A_DOS_DISK;
    }
    end = 0xFFFFFFFF / sb->sectorsize;
    if ((sb->first_device_sector + sb->total_sectors - 1 > end) && (glob->readcmd == CMD_READ)) {
	D(bug("\tDevice is too large\n"));
	FreeMem(boot, bsize);
	return IOERR_BADADDRESS;
    }

    sb->cache = cache_new(64, 256, sb->sectorsize, CACHE_WRITETHROUGH);
 
    if (sb->clusters_count < 4085) {
        D(bug("\tFAT12 filesystem detected\n"));
        sb->type = 12;
        sb->eoc_mark = 0x0FF8;
        sb->func_get_fat_entry = GetFat12Entry;
        sb->func_set_fat_entry = SetFat12Entry;
    }
    else if (sb->clusters_count < 65525) {
        D(bug("\tFAT16 filesystem detected\n"));
        sb->type = 16;
        sb->eoc_mark = 0xFFF8;
        sb->func_get_fat_entry = GetFat16Entry;
        sb->func_set_fat_entry = SetFat16Entry;
    }
    else {
        D(bug("\tFAT32 filesystem detected\n"));
        sb->type = 32;
        sb->eoc_mark = 0x0FFFFFF8;
        sb->func_get_fat_entry = GetFat32Entry;
        sb->func_set_fat_entry = SetFat32Entry;
    }

    /* setup the FAT cache and load the first blocks */
    sb->fat_cachesize = 4096;
    sb->fat_cachesize_bits = log2(sb->fat_cachesize);
    sb->fat_cache_block = 0xffffffff;

    sb->fat_blocks_count = MIN(sb->fat_size, sb->fat_cachesize >> sb->sectorsize_bits);
    sb->fat_blocks = AllocVecPooled(glob->mempool, sizeof(struct cache_block *) * sb->fat_blocks_count);

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

    NEWLIST(&(sb->locks));

    sb->root_lock.dir_cluster = FAT_ROOTDIR_MARK;
    sb->root_lock.dir_entry = FAT_ROOTDIR_MARK;
    sb->root_lock.access = SHARED_LOCK;
    sb->root_lock.first_cluster = 0;
    sb->root_lock.attr = ATTR_DIRECTORY;
    sb->root_lock.size = 0;
    CopyMem(sb->volume.name, sb->root_lock.name, sb->volume.name[0]+1);
    NEWLIST(&sb->root_lock.locks);

    NEWLIST(&(sb->notifies));

    D(bug("\tFAT Filesystem succesfully detected.\n"));
    FreeMem(boot, bsize);
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
	if ((de.e.entry.attr & ATTR_VOLUME_ID_MASK) == ATTR_VOLUME_ID) {
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
            ConvertFATDate(de.e.entry.create_date, de.e.entry.create_time, &volume->create_time);

            D(bug("[fat] volume name is '%s'\n", &(volume->name[1])));

            break;
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

LONG SetVolumeName(struct FSSuper *sb, UBYTE *name) {
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

            /* copy the name in. name is a BSTR */
            de.e.entry.name[0] = name[1];
            for (i = 1; i < 11; i++)
                if (i < name[0])
                    de.e.entry.name[i] = tolower(name[i+1]);
                else
                    de.e.entry.name[i] = ' ';

            if ((err = UpdateDirEntry(&de)) != 0) {
                D(bug("[fat] couldn't change volume name\n"));
                return err;
            }

            sb->volume.name[0] = name[0] < 10 ? name[0] : 10;
            CopyMem(&name[1], &(sb->volume.name[1]), sb->volume.name[0]);
            sb->volume.name[sb->volume.name[0]+1] = '\0';

            D(bug("[fat] new volume name is '%s'\n", &(sb->volume.name[1])));

            break;
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

LONG FindFreeCluster(struct FSSuper *sb, ULONG *rcluster) {
    ULONG cluster = 0;

    /*
     * XXX this implementation is extremely naive. things we
     * could do to make it better:
     *
     *  - don't start looking for a free cluster at the start
     *    each time. start from the current cluster and wrap
     *    around when we hit the end
     *  - track where we last found a free cluster and start
     *    from there
     *  - allocate several contiguous clusters at a time to
     *    reduce fragmentation
     */

    for (cluster = 2; cluster < sb->clusters_count && GET_NEXT_CLUSTER(sb, cluster) != 0; cluster++);

    if (cluster == sb->clusters_count) {
        D(bug("[fat] no more free clusters, we're out of space\n"));
        return ERROR_DISK_FULL;
    }

    D(bug("[fat] found free cluster %ld\n", cluster));

    *rcluster = cluster;

    return 0;
}

void FreeFATSuper(struct FSSuper *sb) {
    D(bug("\tRemoving Super Block from memory\n"));
    cache_free(sb->cache);
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
        if (GET_NEXT_CLUSTER(sb, cluster) == 0)
            free++;

    /* put the value away for later */
    sb->free_clusters = free;

    D(bug("\tfree clusters: %ld\n", free));
}

static const UWORD mdays[] = { 0, 31, 59, 90, 120, 151, 181, 212, 143, 273, 304, 334 };

void ConvertFATDate(UWORD date, UWORD time, struct DateStamp *ds) {
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

    D(bug("[fat] converting fat date: year %d month %d day %d hours %d mins %d secs %d\n", year, month, day, hours, mins, secs));

    /* number of leap years in before this year. note this is only dividing by
     * four, which is fine because FAT dates range 1980-2107. The only year in
     * that range that is divisible by four but not a leap year is 2100. If
     * this code is still being used then, feel free to fix it :) */
    nleap = year >> 2;
    
    /* if this year is a leap year and it's March or later, adjust for this
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
    ds->ds_Tick = (secs << 1) * TICKS_PER_SECOND;

    D(bug("[fat] converted fat date: days %ld minutes %ld ticks %ld\n", ds->ds_Days, ds->ds_Minute, ds->ds_Tick));
}

void ConvertAROSDate(struct DateStamp ds, UWORD *date, UWORD *time) {
    UBYTE year, month, day, hours, mins, secs;
    BOOL leap;

    /* converting no. of days since 1978-01-01 (DOS epoch) to
     * years/months/days since FAT epoch (1980-01-01) */

    /* subtract 730 days in 1978/1979 */
    ds.ds_Days -= 730;

    /* years are 365 days */
    year = ds.ds_Days / 365;
    ds.ds_Days -= year * 365;

    /* leap years. same algorithm as above. get the number of leap years
     * before this year, and subtract that many days */
    ds.ds_Days -= year >> 2;

    /* figure out if we need to adjust for a leap year this year. day 60 is
     * 29-Feb/1-Mar */
    leap = (year & 0x03 && ds.ds_Days >= 60);
    
    /* find the month by checking it against the days-in-month array */
    for (month = 1; month < 12 && ds.ds_Days > mdays[month]; month++);

    /* day of month is whatever's left (+1, since we count from the 1st) */
    day = ds.ds_Days - mdays[month - 1] + 1;

    /* subtract a day if we're after march in a leap year */
    if (leap) {
        day--;
        if (day == 0) {
            month--;
            if (month == 2)
                day = 29;
            else
                day = ds.ds_Days - mdays[month] + 1;
        }
    }

    /* time is easy by comparison. convert minutes since midnight to
     * hours and seconds */
    hours = ds.ds_Minute / 60;
    mins = ds.ds_Minute - (hours * 60);

    /* FAT seconds are 0-29 */
    secs = (ds.ds_Tick / TICKS_PER_SECOND) >> 1;

    /* all that remains is to bit-encode the whole lot */

    /* date bits: yyyy yyym mmmd dddd */
    *date = (((ULONG) year) << 9) | (((ULONG) month) << 5) | day;

    /* time bits: hhhh hmmm mmms ssss */
    *time = (((ULONG) hours) << 11) | (((ULONG) mins) << 5) | secs;
}
