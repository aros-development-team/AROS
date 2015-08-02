/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2015 The AROS Development Team
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
#include <proto/utility.h>
#include <proto/timer.h>
#include <clib/alib_protos.h>

#include <clib/macros.h>

#include <string.h>
#include <ctype.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

static const UBYTE default_oem_name[] = "MSWIN4.1";
static const UBYTE default_filsystype[] = "FAT16   ";

static const ULONG fat16_cluster_thresholds[] =
{
    8400,
    32680,
    262144,
    524288,
    1048576,
    0xFFFFFFFF
};

static const ULONG fat32_cluster_thresholds[] =
{
    16777216,
    33554432,
    67108864,
    0xFFFFFFFF
};

/* 01-01-1981 */
static const struct DateStamp unset_date_limit =
{
    1096,
    0,
    0
};

static LONG GetVolumeIdentity(struct FSSuper *sb,
    struct VolumeIdentity *volume);

/* Helper function to get the location of a fat entry for a cluster. It used
 * to be a define until it got too crazy */
static APTR GetFatEntryPtr(struct FSSuper *sb, ULONG offset, APTR *rb,
    UWORD fat_no)
{
    D(struct Globals *glob = sb->glob);
    ULONG entry_cache_block = offset >> sb->fat_cachesize_bits;
    ULONG entry_cache_offset = offset & (sb->fat_cachesize - 1);
    ULONG num;
    UWORD i;

    /* If the target cluster is not within the currently loaded chunk of fat,
     * we need to get the right data in */
    if (sb->fat_cache_block != entry_cache_block
        || sb->fat_cache_no != fat_no)
    {
        D(bug("[fat] loading %ld FAT sectors starting at sector %ld\n",
            sb->fat_blocks_count,
            entry_cache_block << (sb->fat_cachesize_bits -
            sb->sectorsize_bits)));

        /* Put the old ones back */
        if (sb->fat_cache_block != 0xffffffff)
        {
            for (i = 0; i < sb->fat_blocks_count; i++)
                Cache_FreeBlock(sb->cache, sb->fat_blocks[i]);
            sb->fat_cache_block = 0xffffffff;
        }

        /* Load some more */
        num = sb->first_device_sector + sb->first_fat_sector
            + sb->fat_size * fat_no + (entry_cache_block
            << (sb->fat_cachesize_bits - sb->sectorsize_bits));
        for (i = 0; i < sb->fat_blocks_count; i++)
        {
            sb->fat_blocks[i] =
                Cache_GetBlock(sb->cache, num + i, &sb->fat_buffers[i]);

            if (sb->fat_blocks[i] == NULL)
            {
                while (i-- != 0)
                    Cache_FreeBlock(sb->cache, sb->fat_blocks[i]);
                return NULL;
            }
        }

        /* Remember where we are for next time */
        sb->fat_cache_block = entry_cache_block;
        sb->fat_cache_no = fat_no;
    }

    /* Give the block back if they asked for it (needed to mark the block
     * dirty if they're writing) */
    if (rb != NULL)
        *rb = sb->fat_blocks[entry_cache_offset >> sb->sectorsize_bits];

    /* Compute the pointer location and return it */
    return sb->fat_buffers[entry_cache_offset >> sb->sectorsize_bits] +
        (entry_cache_offset & (sb->sectorsize - 1));
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
static ULONG GetFat12Entry(struct FSSuper *sb, ULONG n)
{
    D(struct Globals *glob = sb->glob);
    ULONG offset = n + n / 2;
    UBYTE *p1;
    UWORD val = 0, *p2;

    if ((offset & (sb->sectorsize - 1)) == sb->sectorsize - 1)
    {
        D(bug("[fat] fat12 cluster pair on block boundary, compensating\n"));

        p1 = GetFatEntryPtr(sb, offset + 1, NULL, 0);
        if (p1 != NULL)
        {
            val = *p1 << 8;
            p1 = GetFatEntryPtr(sb, offset, NULL, 0);
            if (p1 != NULL)
                val |= *p1;
            else
                val = 0;
        }
    }
    else
    {
        p2 = GetFatEntryPtr(sb, offset, NULL, 0);
        if (p2 != NULL)
            val = AROS_LE2WORD(*p2);
    }

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
static ULONG GetFat16Entry(struct FSSuper *sb, ULONG n)
{
    UWORD val = 0, *p;

    p = GetFatEntryPtr(sb, n << 1, NULL, 0);
    if (p != NULL)
        val = AROS_LE2WORD(*p);

    return val;
}

static ULONG GetFat32Entry(struct FSSuper *sb, ULONG n)
{
    ULONG val = 0, *p;

    p = GetFatEntryPtr(sb, n << 2, NULL, 0);
    if (p != NULL)
        val = AROS_LE2LONG(*p) & 0x0fffffff;

    return val;
}

static BOOL SetFat12Entry(struct FSSuper *sb, ULONG n, ULONG val)
{
    D(struct Globals *glob = sb->glob);
    APTR b;
    ULONG offset = n + n / 2;
    BOOL success = TRUE, boundary = FALSE;
    UBYTE *p1;
    UWORD *p2, newval, i;

    for (i = 0; i < sb->fat_count; i++)
    {
        if ((offset & (sb->sectorsize - 1)) == sb->sectorsize - 1)
        {
            boundary = TRUE;

            D(bug(
                "[fat] fat12 cluster pair on block boundary, compensating\n"));

            p1 = GetFatEntryPtr(sb, offset + 1, NULL, i);
            if (p1 != NULL)
            {
                newval = *p1 << 8;
                p1 = GetFatEntryPtr(sb, offset, NULL, i);
                if (p1 != NULL)
                    newval |= *p1;
                else
                    success = FALSE;
            }
            else
                success = FALSE;
        }
        else
        {
            p2 = (UWORD *) GetFatEntryPtr(sb, offset, &b, i);
            if (p2 != NULL)
                newval = AROS_LE2WORD(*p2);
            else
                success = FALSE;
        }

        if (success)
        {
            if (n & 1)
                newval = (newval & 0xf) | val << 4;
            else
                newval = (newval & 0xf000) | val;

            if (boundary)
            {
                /* XXX: Ideally we'd mark both blocks dirty at the same time or
                 * only do it once if they're the same block. Unfortunately any 
                 * old value of b is invalid after a call to GetFatEntryPtr, as
                 * it may have swapped the previous cache out. This is probably
                 * safe enough. */
                p1 = GetFatEntryPtr(sb, offset + 1, &b, i);
                if (p1 != NULL)
                {
                    *p1 = newval >> 8;
                    Cache_MarkBlockDirty(sb->cache, b);
                    p1 = GetFatEntryPtr(sb, offset, &b, i);
                    if (p1 != NULL)
                    {
                        *p1 = newval & 0xff;
                        Cache_MarkBlockDirty(sb->cache, b);
                    }
                    else
                        success = FALSE;
                }
                else
                    success = FALSE;
            }
            else
            {
                *p2 = AROS_WORD2LE(newval);
                Cache_MarkBlockDirty(sb->cache, b);
            }
        }
    }

    return success;
}

static BOOL SetFat16Entry(struct FSSuper *sb, ULONG n, ULONG val)
{
    BOOL success = TRUE;
    APTR b;
    UWORD i, *p;

    for (i = 0; i < sb->fat_count; i++)
    {
        p = GetFatEntryPtr(sb, n << 1, &b, i);
        if (p != NULL)
        {
            *p = AROS_WORD2LE((UWORD) val);
            Cache_MarkBlockDirty(sb->cache, b);
        }
        else
            success = FALSE;
    }

    return success;
}

static BOOL SetFat32Entry(struct FSSuper *sb, ULONG n, ULONG val)
{
    BOOL success = TRUE;
    APTR b;
    ULONG *p;
    UWORD i;

    for (i = 0; i < sb->fat_count; i++)
    {
        p = (ULONG *) GetFatEntryPtr(sb, n << 2, &b, i);
        if (p != NULL)
        {
            *p = (*p & 0xf0000000) | val;
            Cache_MarkBlockDirty(sb->cache, b);
        }
        else
            success = FALSE;
    }

    return success;
}

LONG ReadFATSuper(struct FSSuper *sb)
{
    struct Globals *glob = sb->glob;
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);
    LONG err = 0, td_err;
    ULONG bsize = de->de_SizeBlock * 4, total_sectors, id;
    struct FATBootSector *boot;
    struct FATEBPB *ebpb;
    struct FATFSInfo *fsinfo;
    BOOL invalid = FALSE;
    ULONG end;
    LONG i;
    struct DirHandle dh;
    struct DirEntry dir_entry;
    APTR block_ref;
    UBYTE *fat_block;

    D(bug("[fat] reading boot sector\n"));

    boot = AllocMem(bsize, MEMF_ANY);
    if (!boot)
        return ERROR_NO_FREE_STORE;

    sb->first_device_sector =
        de->de_BlocksPerTrack * de->de_Surfaces * de->de_LowCyl;

    /* Get a preliminary total-sectors value so we don't risk going outside
     * partition limits */
    sb->total_sectors =
        de->de_BlocksPerTrack * de->de_Surfaces * (de->de_HighCyl + 1)
        - sb->first_device_sector;

    D(bug("[fat] boot sector at sector %ld\n", sb->first_device_sector));

    /*
     * Read the boot sector. We go direct because we don't have a cache yet,
     * and can't create one until we know the sector size, which is held in
     * the boot sector. In practice it doesn't matter - we're going to use
     * this once and once only.
     */
    if ((td_err = AccessDisk(FALSE, sb->first_device_sector, 1, bsize,
        (UBYTE *) boot, glob)) != 0)
    {
        D(bug("[fat] couldn't read boot block (%ld)\n", td_err));
        FreeMem(boot, bsize);
        return ERROR_UNKNOWN;
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

    D(bug("\tSectorsPerCluster = %ld\n", (ULONG) boot->bpb_sect_per_clust));
    D(bug("\tClusterSize = %ld\n", sb->clustersize));
    D(bug("\tClusterSize Bits = %ld\n", sb->clustersize_bits));
    D(bug("\tCluster Sectors Bits = %ld\n", sb->cluster_sectors_bits));

    sb->first_fat_sector = AROS_LE2WORD(boot->bpb_rsvd_sect_count);
    D(bug("\tFirst FAT Sector = %ld\n", sb->first_fat_sector));

    sb->fat_count = boot->bpb_num_fats;
    D(bug("\tNumber of FATs = %d\n", sb->fat_count));

    if (boot->bpb_fat_size_16 != 0)
        sb->fat_size = AROS_LE2WORD(boot->bpb_fat_size_16);
    else
        sb->fat_size = AROS_LE2LONG(boot->ebpbs.ebpb32.bpb_fat_size_32);
    D(bug("\tFAT Size = %ld\n", sb->fat_size));

    if (boot->bpb_total_sectors_16 != 0)
        total_sectors = AROS_LE2WORD(boot->bpb_total_sectors_16);
    else
        total_sectors = AROS_LE2LONG(boot->bpb_total_sectors_32);
    D(bug("\tTotal Sectors = %ld\n", sb->total_sectors));

    /* Check that the boot block's sector count is the same as the
     * partition's sector count. This stops a resized partition being
     * mounted before reformatting */
    if (total_sectors != sb->total_sectors)
        invalid = TRUE;

    sb->rootdir_sectors = ((AROS_LE2WORD(boot->bpb_root_entries_count)
        * sizeof(struct FATDirEntry)) + (sb->sectorsize - 1))
        >> sb->sectorsize_bits;
    D(bug("\tRootDir Sectors = %ld\n", sb->rootdir_sectors));

    sb->data_sectors = sb->total_sectors - (sb->first_fat_sector
        + (sb->fat_count * sb->fat_size) + sb->rootdir_sectors);
    D(bug("\tData Sectors = %ld\n", sb->data_sectors));

    sb->clusters_count = sb->data_sectors >> sb->cluster_sectors_bits;
    D(bug("\tClusters Count = %ld\n", sb->clusters_count));

    sb->first_rootdir_sector =
        sb->first_fat_sector + (sb->fat_count * sb->fat_size);
    D(bug("\tFirst RootDir Sector = %ld\n", sb->first_rootdir_sector));

    sb->first_data_sector =
        sb->first_fat_sector + (sb->fat_count * sb->fat_size)
        + sb->rootdir_sectors;
    D(bug("\tFirst Data Sector = %ld\n", sb->first_data_sector));

    /* Check if disk is in fact a FAT filesystem */

    /* Valid sector size: 512, 1024, 2048, 4096 */
    if (sb->sectorsize != 512 && sb->sectorsize != 1024
        && sb->sectorsize != 2048 && sb->sectorsize != 4096)
        invalid = TRUE;

    /* Valid bpb_sect_per_clust: 1, 2, 4, 8, 16, 32, 64, 128 */
    if ((boot->bpb_sect_per_clust & (boot->bpb_sect_per_clust - 1)) != 0
        || boot->bpb_sect_per_clust == 0 || boot->bpb_sect_per_clust > 128)
        invalid = TRUE;

    /* Valid cluster size: 512, 1024, 2048, 4096, 8192, 16k, 32k, 64k */
    if (sb->clustersize > 64 * 1024)
        invalid = TRUE;

    if (sb->first_fat_sector == 0)
        invalid = TRUE;

    if (sb->fat_count == 0)
        invalid = TRUE;

    if (boot->bpb_media < 0xF0)
        invalid = TRUE;

    /* FAT "signature" */
    if (boot->bpb_signature[0] != 0x55 || boot->bpb_signature[1] != 0xaa)
        invalid = TRUE;

    if (invalid)
    {
        D(bug("\tInvalid FAT Boot Sector\n"));
        FreeMem(boot, bsize);
        return ERROR_NOT_A_DOS_DISK;
    }

    end = 0xFFFFFFFF / sb->sectorsize;
    if ((sb->first_device_sector + sb->total_sectors - 1 > end)
        && (glob->readcmd == CMD_READ))
    {
        D(bug("\tDevice is too large\n"));
        ErrorMessage("Your device driver does not support 64-bit\n"
            "disk addressing, but it is needed to access\n"
            "the volume in device %s.\n\n"
            "In order to prevent data damage, access to\n"
            "this volume was blocked. Please upgrade\n"
            "your device driver.", "OK",
            (IPTR)(AROS_BSTR_ADDR(glob->devnode->dol_Name)));
        FreeMem(boot, bsize);
        return ERROR_UNKNOWN;
    }

    sb->cache = Cache_CreateCache(glob, 64, 64, sb->sectorsize, SysBase,
        DOSBase);
    if (sb->cache == NULL)
    {
        err = IoErr();
        FreeMem(boot, bsize);
        return err;
    }

    if (sb->clusters_count < 4085)
    {
        D(bug("\tFAT12 filesystem detected\n"));
        sb->type = 12;
        sb->eoc_mark = 0x0FFF;
        sb->func_get_fat_entry = GetFat12Entry;
        sb->func_set_fat_entry = SetFat12Entry;
    }
    else if (sb->clusters_count < 65525)
    {
        D(bug("\tFAT16 filesystem detected\n"));
        sb->type = 16;
        sb->eoc_mark = 0xFFFF;
        sb->func_get_fat_entry = GetFat16Entry;
        sb->func_set_fat_entry = SetFat16Entry;
    }
    else
    {
        D(bug("\tFAT32 filesystem detected\n"));
        sb->type = 32;
        sb->eoc_mark = 0x0FFFFFFF;
        sb->func_get_fat_entry = GetFat32Entry;
        sb->func_set_fat_entry = SetFat32Entry;
    }

    /* Set up the FAT cache and load the first blocks */
    sb->fat_cachesize = 4096;
    sb->fat_cachesize_bits = log2(sb->fat_cachesize);
    sb->fat_cache_block = 0xffffffff;

    sb->fat_blocks_count =
        MIN(sb->fat_size, sb->fat_cachesize >> sb->sectorsize_bits);
    sb->fat_blocks = AllocVecPooled(glob->mempool,
        sizeof(APTR) * sb->fat_blocks_count);
    sb->fat_buffers = AllocVecPooled(glob->mempool,
        sizeof(APTR) * sb->fat_blocks_count);
    if (sb->fat_blocks == NULL || sb->fat_buffers == NULL)
    {
        err = ERROR_NO_FREE_STORE;
        FreeVecPooled(glob->mempool, sb->fat_blocks);
        FreeVecPooled(glob->mempool, sb->fat_buffers);
        FreeMem(boot, bsize);
        return err;
    }
    glob->sb = sb;

    if (sb->type != 32)
    {
        /* Set up volume ID */
        sb->volume_id = AROS_LE2LONG(boot->ebpbs.ebpb.bs_volid);

        /* Location of root directory */
        sb->rootdir_cluster = 0;
        sb->rootdir_sector = sb->first_rootdir_sector;
        ebpb = &boot->ebpbs.ebpb;
    }
    else
    {
        /* Set up volume ID */
        sb->volume_id = AROS_LE2LONG(boot->ebpbs.ebpb32.ebpb.bs_volid);

        /* Location of root directory */
        sb->rootdir_cluster =
            AROS_LE2LONG(boot->ebpbs.ebpb32.bpb_root_cluster);
        sb->rootdir_sector = 0;
        ebpb = &boot->ebpbs.ebpb32.ebpb;
    }

    D(bug("[fat] rootdir at cluster %ld sector %ld\n", sb->rootdir_cluster,
        sb->rootdir_sector));

    /* Initialise the root directory if this is a newly formatted volume */
    if (glob->formatting)
    {
        /* Clear all FAT sectors */
        for (i = 0; err == 0 && i < sb->fat_size * 2; i++)
        {
            block_ref = Cache_GetBlock(sb->cache,
                sb->first_device_sector + sb->first_fat_sector + i,
                &fat_block);
            if (block_ref != NULL)
            {
                /* FIXME: Handle IO errors on cache read! */
                memset(fat_block, 0, bsize);
                if (i == 0)
                {
                    /* The first two entries are special */
                    if (sb->type == 32)
                        *(UQUAD *) fat_block =
                            AROS_QUAD2LE(0x0FFFFFFF0FFFFFF8);
                    else if (sb->type == 16)
                        *(ULONG *) fat_block = AROS_LONG2LE(0xFFFFFFF8);
                    else
                        *(ULONG *) fat_block = AROS_LONG2LE(0x00FFFFF8);
                }
                Cache_MarkBlockDirty(sb->cache, block_ref);
                Cache_FreeBlock(sb->cache, block_ref);
            }
            else
                err = IoErr();
        }

        if (err == 0)
        {
            /* Allocate first cluster of the root directory */
            if (sb->type == 32)
                AllocCluster(sb, sb->rootdir_cluster);

            /* Get a handle on the root directory */
            InitDirHandle(sb, 0, &dh, FALSE, glob);
        }

        /* Clear all entries in the root directory */
        for (i = 0; err == 0 && GetDirEntry(&dh, i, &dir_entry, glob) == 0;
            i++)
        {
            memset(&dir_entry.e.entry, 0, sizeof(struct FATDirEntry));
            err = UpdateDirEntry(&dir_entry, glob);
        }

        if (err == 0)
        {
            err = SetVolumeName(sb, ebpb->bs_vollab, FAT_MAX_SHORT_NAME);

            ReleaseDirHandle(&dh, glob);
            glob->formatting = FALSE;
            D(bug("\tRoot dir created.\n"));
        }

        if (err == 0)
        {
            /* Check everything is really written to disk before we proceed */
            if (!Cache_Flush(sb->cache))
                err = IoErr();
        }
    }

    if (GetVolumeIdentity(sb, &(sb->volume)) != 0)
    {
        LONG i;
        UBYTE *uu = (void *)&sb->volume_id;

        /* No volume name entry, so construct name from serial number */
        for (i = 1; i < 10;)
        {
            int d;

            if (i == 5)
                sb->volume.name[i++] = '-';

            d = (*uu) & 0x0f;
            sb->volume.name[i++] = (d < 10) ? '0' + d : 'A' - 10 + d;
            d = ((*uu) & 0xf0) >> 4;
            sb->volume.name[i++] = (d < 10) ? '0' + d : 'A' - 10 + d;

            uu++;
        }

        sb->volume.name[i] = '\0';
        sb->volume.name[0] = 9;
    }

    /* Many FAT volumes do not have a creation date set, with the result
     * that two volumes with the same name are likely to be indistinguishable
     * on the DOS list. To work around this problem, we set the ds_Tick field
     * of such volumes' dol_VolumeDate timestamp to a pseudo-random value based
     * on the serial number. Since there are 3000 ticks in a minute, we use an
     * 11-bit hash value in the range 0 to 2047.
     */
    if (CompareDates(&sb->volume.create_time, &unset_date_limit) > 0)
    {
        id = sb->volume_id;
        sb->volume.create_time.ds_Days = 0;
        sb->volume.create_time.ds_Minute = 0;
        sb->volume.create_time.ds_Tick = (id >> 22 ^ id >> 11 ^ id) & 0x7FF;
        D(bug("[fat] Set hash time to %ld ticks\n",
            sb->volume.create_time.ds_Tick));
    }

    /* Get initial number of free clusters */
    sb->free_clusters = -1;
    sb->next_cluster = -1;
    if (sb->type == 32)
    {
        sb->fsinfo_block = Cache_GetBlock(sb->cache, sb->first_device_sector
            + AROS_LE2WORD(boot->ebpbs.ebpb32.bpb_fs_info),
            (UBYTE **) &fsinfo);
        if (sb->fsinfo_block != NULL)
        {
            if (fsinfo->lead_sig == AROS_LONG2LE(FSI_LEAD_SIG)
                && fsinfo->struct_sig == AROS_LONG2LE(FSI_STRUCT_SIG)
                && fsinfo->trail_sig == AROS_LONG2LE(FSI_TRAIL_SIG))
            {
                sb->free_clusters = AROS_LE2LONG(fsinfo->free_count);
                sb->next_cluster = AROS_LE2LONG(fsinfo->next_free);
                D(bug("[fat] valid FATFSInfo block found\n"));
                sb->fsinfo_buffer = fsinfo;
            }
            else
                Cache_FreeBlock(sb->cache, sb->fsinfo_block);
        }
    }
    if (sb->free_clusters == -1)
        CountFreeClusters(sb);
    if (sb->next_cluster == -1)
        sb->next_cluster = 2;

    FreeMem(boot, bsize);
    if (err != 0)
        FreeFATSuper(sb);
    else
    {
        D(bug("\tFAT Filesystem successfully detected.\n"));
        D(bug("\tFree Clusters = %ld\n", sb->free_clusters));
        D(bug("\tNext Free Cluster = %ld\n", sb->next_cluster));
    }

    return err;
}

static LONG GetVolumeIdentity(struct FSSuper *sb,
    struct VolumeIdentity *volume)
{
    struct Globals *glob = sb->glob;
    struct DirHandle dh;
    struct DirEntry de;
    LONG err;
    int i;

    D(bug("[fat] searching root directory for volume name\n"));

    /* Search the directory for the volume ID entry. It would've been nice to
     * just use GetNextDirEntry but I didn't want a flag or something to tell
     * it not to skip the volume name */
    InitDirHandle(sb, sb->rootdir_cluster, &dh, FALSE, glob);

    while ((err = GetDirEntry(&dh, dh.cur_index + 1, &de, glob)) == 0)
    {

        /* Match the volume ID entry */
        if ((de.e.entry.attr & ATTR_VOLUME_ID_MASK) == ATTR_VOLUME_ID
            && de.e.entry.name[0] != 0xe5)
        {
            D(bug("[fat] found volume id entry %ld\n", dh.cur_index));

            /* Copy the name in. 'volume->name' is a BSTR */
            volume->name[1] = de.e.entry.name[0];
            for (i = 1; i < FAT_MAX_SHORT_NAME; i++)
            {
                if (volume->name[i] == ' ')
                    volume->name[i + 1] = de.e.entry.name[i];
                else
                    volume->name[i + 1] = tolower(de.e.entry.name[i]);
            }

            for (i = 10; volume->name[i + 1] == ' '; i--);
            volume->name[i + 2] = '\0';
            volume->name[0] = strlen(&(volume->name[1]));

            /* Get the volume creation date too */
            ConvertFATDate(de.e.entry.create_date, de.e.entry.create_time,
                &volume->create_time, glob);

            D(bug("[fat] volume name is '%s'\n", &(volume->name[1])));

            break;
        }

        /* Bail out if we hit the end of the dir */
        if (de.e.entry.name[0] == 0x00)
        {
            D(bug("[fat] found end-of-directory marker,"
                " volume name entry not found\n"));
            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }
    }

    ReleaseDirHandle(&dh, glob);
    return err;
}

LONG FormatFATVolume(const UBYTE *name, UWORD len, struct Globals *glob)
{
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);
    LONG td_err;
    ULONG bsize = de->de_SizeBlock * 4;
    struct FATBootSector *boot;
    struct FATEBPB *ebpb;
    struct FATFSInfo *fsinfo;
    UWORD type, i, root_entries_count;
    struct EClockVal eclock;
    ULONG sectors_per_cluster = 0, sector_count, first_fat_sector,
        fat_size, root_dir_sectors, first_device_sector, temp1, temp2;

    /* Decide on FAT type based on number of sectors */
    sector_count = (de->de_HighCyl - de->de_LowCyl + 1)
        * de->de_Surfaces * de->de_BlocksPerTrack;
    if (sector_count < 4085)
        type = 12;
    else if (sector_count < 1024 * 1024)
        type = 16;
    else
        type = 32;

    D(bug("[fat] writing boot sector\n"));

    /* Decide on cluster size and root dir entries */
    first_fat_sector = 1;
    if (type == 12)
    {
        if (sector_count == 1440)
        {
            sectors_per_cluster = 2;
            root_entries_count = 112;
        }
        else if (sector_count == 2880)
        {
            sectors_per_cluster = 1;
            root_entries_count = 224;
        }
        else if (sector_count == 5760)
        {
            sectors_per_cluster = 2;
            root_entries_count = 240;
        }
        else
        {
            /* We only support some common 3.5" floppy formats */
            return ERROR_NOT_IMPLEMENTED;
        }
    }
    else if (type == 16)
    {
        for (i = 0; fat16_cluster_thresholds[i] < sector_count; i++);
        sectors_per_cluster = 1 << i;
        root_entries_count = 512;
    }
    else
    {
        for (i = 0; fat32_cluster_thresholds[i] < sector_count; i++);
        sectors_per_cluster = 8 << i;
        root_entries_count = 0;
        first_fat_sector = 32;
    }

    D(bug("\tFirst FAT Sector = %ld\n", first_fat_sector));

    /* Determine FAT size */
    root_dir_sectors = (root_entries_count * 32 + (bsize - 1)) / bsize;
    temp1 = sector_count - (first_fat_sector + root_dir_sectors);
    temp2 = 256 * sectors_per_cluster + 2;
    if (type == 32)
        temp2 /= 2;
    fat_size = (temp1 + temp2 - 1) / temp2;

    boot = AllocMem(bsize, MEMF_CLEAR);
    if (!boot)
        return ERROR_NO_FREE_STORE;

    /* Install x86 infinite loop boot code to keep major OSes happy */
    boot->bs_jmp_boot[0] = 0xEB;
    boot->bs_jmp_boot[1] = 0xFE;
    boot->bs_jmp_boot[2] = 0x90;

    CopyMem(default_oem_name, boot->bs_oem_name, 8);

    boot->bpb_bytes_per_sect = AROS_WORD2LE(bsize);
    boot->bpb_sect_per_clust = sectors_per_cluster;

    boot->bpb_rsvd_sect_count = AROS_WORD2LE(first_fat_sector);

    boot->bpb_num_fats = 2;

    boot->bpb_root_entries_count = AROS_WORD2LE(root_entries_count);

    if (sector_count < 0x10000 && type != 32)
        boot->bpb_total_sectors_16 = AROS_WORD2LE(sector_count);
    else
        boot->bpb_total_sectors_32 = AROS_LONG2LE(sector_count);

    boot->bpb_media = 0xF8;

    boot->bpb_sect_per_track = AROS_WORD2LE(de->de_BlocksPerTrack);
    boot->bpb_num_heads = AROS_WORD2LE(de->de_Surfaces);
    boot->bpb_hidden_sect = AROS_LONG2LE(de->de_Reserved);

    if (type == 32)
    {
        boot->ebpbs.ebpb32.bpb_fat_size_32 = AROS_LONG2LE(fat_size);
        boot->ebpbs.ebpb32.bpb_root_cluster = AROS_LONG2LE(2);
        boot->ebpbs.ebpb32.bpb_fs_info = AROS_WORD2LE(1);
        boot->ebpbs.ebpb32.bpb_back_bootsec = AROS_WORD2LE(6);
        ebpb = &boot->ebpbs.ebpb32.ebpb;
    }
    else
    {
        boot->bpb_fat_size_16 = AROS_WORD2LE(fat_size);
        ebpb = &boot->ebpbs.ebpb;
    }

    ebpb->bs_drvnum = 0x80;
    ebpb->bs_bootsig = 0x29;

    /* Generate a pseudo-random serial number. Not the original algorithm,
     * but it shouldn't matter */
    ReadEClock(&eclock);
    ebpb->bs_volid = FastRand(eclock.ev_lo ^ eclock.ev_hi);

    /* Copy volume name in */
    for (i = 0; i < FAT_MAX_SHORT_NAME; i++)
        if (i < len)
            ebpb->bs_vollab[i] = toupper(name[i]);
        else
            ebpb->bs_vollab[i] = ' ';

    CopyMem(default_filsystype, ebpb->bs_filsystype, 8);
    if (type != 16)
    {
        if (type == 32)
            ebpb->bs_filsystype[3] = '3';
        ebpb->bs_filsystype[4] = '2';
    }

    boot->bpb_signature[0] = 0x55;
    boot->bpb_signature[1] = 0xaa;

    /* Write the boot sector */
    first_device_sector =
        de->de_BlocksPerTrack * de->de_Surfaces * de->de_LowCyl;

    D(bug("[fat] boot sector at sector %ld\n", first_device_sector));

    if ((td_err = AccessDisk(TRUE, first_device_sector, 1, bsize,
        (UBYTE *) boot, glob)) != 0)
    {
        D(bug("[fat] couldn't write boot block (%ld)\n", td_err));
        FreeMem(boot, bsize);
        return ERROR_UNKNOWN;
    }

    /* Write back-up boot sector and FS info sector */
    if (type == 32)
    {
        if ((td_err = AccessDisk(TRUE, first_device_sector + 6, 1, bsize,
            (UBYTE *) boot, glob)) != 0)
        {
            D(bug("[fat] couldn't write back-up boot block (%ld)\n", td_err));
            FreeMem(boot, bsize);
            return ERROR_UNKNOWN;
        }

        fsinfo = (APTR) boot;
        memset(fsinfo, 0, bsize);

        fsinfo->lead_sig = AROS_LONG2LE(FSI_LEAD_SIG);
        fsinfo->struct_sig = AROS_LONG2LE(FSI_STRUCT_SIG);
        fsinfo->trail_sig = AROS_LONG2LE(FSI_TRAIL_SIG);
        fsinfo->free_count = AROS_LONG2LE(0xFFFFFFFF);
        fsinfo->next_free = AROS_LONG2LE(0xFFFFFFFF);

        if ((td_err = AccessDisk(TRUE, first_device_sector + 1, 1, bsize,
            (UBYTE *) fsinfo, glob)) != 0)
        {
            D(bug("[fat] couldn't write back-up boot block (%ld)\n", td_err));
            FreeMem(boot, bsize);
            return ERROR_UNKNOWN;
        }
    }

    FreeMem(boot, bsize);

    glob->formatting = TRUE;

    return 0;
}

LONG SetVolumeName(struct FSSuper *sb, UBYTE *name, UWORD len)
{
    struct Globals *glob = sb->glob;
    struct DirHandle dh;
    struct DirEntry de;
    LONG err, td_err;
    int i;
    struct DosEnvec *dosenv = BADDR(glob->fssm->fssm_Environ);
    ULONG bsize = dosenv->de_SizeBlock * 4;
    struct FATBootSector *boot;

    /* Truncate name if necessary */
    if (len > FAT_MAX_SHORT_NAME)
        len = FAT_MAX_SHORT_NAME;

    /* Read boot block */
    boot = AllocMem(bsize, MEMF_ANY);
    if (!boot)
        return ERROR_NO_FREE_STORE;

    if ((td_err = AccessDisk(FALSE, sb->first_device_sector, 1, bsize,
        (UBYTE *) boot, glob)) != 0)
    {
        D(bug("[fat] couldn't read boot block (%ld)\n", td_err));
        FreeMem(boot, bsize);
        return ERROR_UNKNOWN;
    }

    D(bug("[fat] searching root directory for volume name\n"));

    /* Search the directory for the volume ID entry. It would've been nice to
     * just use GetNextDirEntry but I didn't want a flag or something to tell
     * it not to skip the volume name */
    InitDirHandle(sb, 0, &dh, FALSE, glob);

    while ((err = GetDirEntry(&dh, dh.cur_index + 1, &de, glob)) == 0)
    {

        /* Match the volume ID entry */
        if ((de.e.entry.attr & ATTR_VOLUME_ID_MASK) == ATTR_VOLUME_ID
            && de.e.entry.name[0] != 0xe5)
        {
            D(bug("[fat] found volume id entry %ld\n", dh.cur_index));
            err = 0;
            break;
        }

        /* Bail out if we hit the end of the dir */
        if (de.e.entry.name[0] == 0x00)
        {
            D(bug("[fat] found end-of-directory marker,"
                " volume name entry not found\n"));
            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }
    }

    /* Create a new volume ID entry if there wasn't one */
    if (err != 0)
    {
        err = AllocDirEntry(&dh, 0, &de, glob);
        if (err == 0)
            FillDirEntry(&de, ATTR_VOLUME_ID, 0, glob);
    }

    /* Copy the name in */
    if (err == 0)
    {
        for (i = 0; i < FAT_MAX_SHORT_NAME; i++)
            if (i < len)
                de.e.entry.name[i] = toupper(name[i]);
            else
                de.e.entry.name[i] = ' ';

        if ((err = UpdateDirEntry(&de, glob)) != 0)
        {
            D(bug("[fat] couldn't change volume name\n"));
            return err;
        }
    }

    /* Copy name to boot block as well, and save */
    if (sb->type == 32)
        CopyMem(de.e.entry.name, boot->ebpbs.ebpb32.ebpb.bs_vollab,
            FAT_MAX_SHORT_NAME);
    else
        CopyMem(de.e.entry.name, boot->ebpbs.ebpb.bs_vollab,
            FAT_MAX_SHORT_NAME);

    if ((td_err = AccessDisk(TRUE, sb->first_device_sector, 1, bsize,
        (UBYTE *) boot, glob)) != 0)
        D(bug("[fat] couldn't write boot block (%ld)\n", td_err));
    FreeMem(boot, bsize);

    /* Update name in SB */
    sb->volume.name[0] = len;
    sb->volume.name[1] = toupper(name[0]);
    for (i = 1; i < len; i++)
        sb->volume.name[i + 1] = tolower(name[i]);
    sb->volume.name[len + 1] = '\0';

    D(bug("[fat] new volume name is '%s'\n", &(sb->volume.name[1])));

    ReleaseDirHandle(&dh, glob);
    return err;
}

LONG FindFreeCluster(struct FSSuper *sb, ULONG *rcluster)
{
    D(struct Globals *glob = sb->glob);
    ULONG cluster = 0;
    BOOL found = FALSE;

    for (cluster = sb->next_cluster;
        cluster < 2 + sb->clusters_count && !found; cluster++)
    {
        if (GET_NEXT_CLUSTER(sb, cluster) == 0)
        {
            *rcluster = cluster;
            found = TRUE;
        }
    }

    if (!found)
    {
        for (cluster = 2; cluster < sb->next_cluster && !found; cluster++)
        {
            if (GET_NEXT_CLUSTER(sb, cluster) == 0)
            {
                *rcluster = cluster;
                found = TRUE;
            }
        }
    }

    if (!found)
    {
        D(bug("[fat] no more free clusters, we're out of space\n"));
        return ERROR_DISK_FULL;
    }

    sb->next_cluster = *rcluster;

    D(bug("[fat] found free cluster %ld\n", *rcluster));

    return 0;
}

void FreeFATSuper(struct FSSuper *sb)
{
    struct Globals *glob = sb->glob;
    D(bug("\tRemoving Super Block from memory\n"));
    Cache_DestroyCache(sb->cache);
    FreeVecPooled(glob->mempool, sb->fat_buffers);
    sb->fat_buffers = NULL;
    FreeVecPooled(glob->mempool, sb->fat_blocks);
    sb->fat_blocks = NULL;
}

/* See how many unused clusters are available */
void CountFreeClusters(struct FSSuper *sb)
{
    D(struct Globals *glob = sb->glob);
    ULONG cluster = 0;
    ULONG free = 0;

    /* Loop over all the data clusters */
    for (cluster = 2; cluster < sb->clusters_count + 2; cluster++)
    {
        /* Record the free ones */
        if (GET_NEXT_CLUSTER(sb, cluster) == 0)
            free++;
    }

    /* Put the value away for later */
    sb->free_clusters = free;

    D(bug("\tfree clusters: %ld\n", free));
}

void AllocCluster(struct FSSuper *sb, ULONG cluster)
{
    SET_NEXT_CLUSTER(sb, cluster, sb->eoc_mark);
    sb->free_clusters--;
    if (sb->fsinfo_buffer != NULL)
    {
        sb->fsinfo_buffer->free_count = AROS_LONG2LE(sb->free_clusters);
        sb->fsinfo_buffer->next_free = AROS_LONG2LE(sb->next_cluster);
        Cache_MarkBlockDirty(sb->cache, sb->fsinfo_block);
    }
}

void FreeCluster(struct FSSuper *sb, ULONG cluster)
{
    SET_NEXT_CLUSTER(sb, cluster, 0);
    sb->free_clusters++;
    if (sb->fsinfo_buffer != NULL)
    {
        sb->fsinfo_buffer->free_count = AROS_LONG2LE(sb->free_clusters);
        Cache_MarkBlockDirty(sb->cache, sb->fsinfo_block);
    }
}

void ConvertFATDate(UWORD date, UWORD time, struct DateStamp *ds,
    struct Globals *glob)
{
    ULONG year, month, day, hours, mins, secs;
    struct ClockData clock_data;

    /* Date bits: yyyy yyym mmmd dddd */
    year = (date & 0xfe00) >> 9;    /* Bits 15-9 */
    month = (date & 0x01e0) >> 5;   /* bits 8-5 */
    day = date & 0x001f;            /* Bits 4-0 */

    /* Time bits: hhhh hmmm mmms ssss */
    hours = (time & 0xf800) >> 11;  /* Bits 15-11 */
    mins = (time & 0x07e0) >> 5;    /* Bits 10-5 */
    secs = time & 0x001f;           /* Bits 4-0 */

    D(bug("[fat] converting fat date: year %d month %d day %d hours %d"
        " mins %d secs %d\n", year, month, day, hours, mins, secs));

    if (month < 1 || month > 12 || day < 1 || day > 31 || hours > 23 ||
        mins > 59 || secs > 29)
    {
        D(bug("[fat] invalid fat date: using 01-01-1978 instead\n"));
        secs = 0;
    }
    else
    {
        clock_data.year = 1980 + year;
        clock_data.month = month;
        clock_data.mday = day;
        clock_data.hour = hours;
        clock_data.min = mins;
        clock_data.sec = secs << 1;
        secs = Date2Amiga(&clock_data);
    }

    /* Calculate days since 1978-01-01 (DOS epoch) */
    ds->ds_Days = secs / (60 * 60 * 24);

    /* Minutes since midnight */
    ds->ds_Minute = secs / 60 % (24 * 60);

    /* 1/50 sec ticks since last minute */
    ds->ds_Tick = secs % 60 * TICKS_PER_SECOND;

    D(bug("[fat] converted fat date: days %ld minutes %ld ticks %ld\n",
        ds->ds_Days, ds->ds_Minute, ds->ds_Tick));
}

void ConvertDOSDate(struct DateStamp *ds, UWORD * date, UWORD * time,
    struct Globals *glob)
{
    ULONG year, month, day, hours, mins, secs;
    struct ClockData clock_data;

    /* Convert datestamp to seconds since 1978 */
    secs = ds->ds_Days * 60 * 60 * 24 + ds->ds_Minute * 60
        + ds->ds_Tick / TICKS_PER_SECOND;

    /* Round up to next even second because of FAT's two-second granularity */
    secs = (secs & ~1) + 2;

    /* Convert seconds since 1978 to calendar/time data */
    Amiga2Date(secs, &clock_data);

    /* Get values used in FAT dates */
    year = clock_data.year - 1980;
    month = clock_data.month - 0;
    day = clock_data.mday;
    hours = clock_data.hour;
    mins = clock_data.min;
    secs = clock_data.sec >> 1;

    /* All that remains is to bit-encode the whole lot */

    /* Date bits: yyyy yyym mmmd dddd */
    *date = (((ULONG) year) << 9) | (((ULONG) month) << 5) | day;

    /* Time bits: hhhh hmmm mmms ssss */
    *time = (((ULONG) hours) << 11) | (((ULONG) mins) << 5) | secs;
}
