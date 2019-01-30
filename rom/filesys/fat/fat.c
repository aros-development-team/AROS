/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright � 2006 Marek Szyprowski
 * Copyright � 2007-2015 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <exec/types.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

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
ULONG GetFat12Entry(struct FSSuper *sb, ULONG n)
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
ULONG GetFat16Entry(struct FSSuper *sb, ULONG n)
{
    UWORD val = 0, *p;

    p = GetFatEntryPtr(sb, n << 1, NULL, 0);
    if (p != NULL)
        val = AROS_LE2WORD(*p);

    return val;
}

ULONG GetFat32Entry(struct FSSuper *sb, ULONG n)
{
    ULONG val = 0, *p;

    p = GetFatEntryPtr(sb, n << 2, NULL, 0);
    if (p != NULL)
        val = AROS_LE2LONG(*p) & 0x0fffffff;

    return val;
}

BOOL SetFat12Entry(struct FSSuper *sb, ULONG n, ULONG val)
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

BOOL SetFat16Entry(struct FSSuper *sb, ULONG n, ULONG val)
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

BOOL SetFat32Entry(struct FSSuper *sb, ULONG n, ULONG val)
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
            *p = AROS_LONG2LE((AROS_LE2LONG(*p) & 0xf0000000) | val);
            Cache_MarkBlockDirty(sb->cache, b);
        }
        else
            success = FALSE;
    }

    return success;
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
