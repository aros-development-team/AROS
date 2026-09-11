/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright (C) 2026 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

/*
 * Volume validation.
 *
 * This is the FAT counterpart of the AFS validator: it is run in the
 * handler process, synchronously, before a volume is made available, and
 * it only ever runs when there is reason to think the volume is not
 * consistent. On FAT the trigger is the "clean shutdown" bit in the
 * second FAT entry, which this handler now maintains: it is cleared on
 * the first write after a mount or a flush, and set again once the cache
 * has been written back. A volume whose bit is clear at mount time was
 * therefore either torn down uncleanly, or dirtied by another OS.
 *
 * The approach mirrors AFS: build a fresh in-memory allocation bitmap by
 * walking every cluster chain reachable from the root directory, fixing
 * what is found along the way, then reconcile the on-disk FAT with the
 * bitmap.
 *
 *  - A chain that runs off the end of the volume, into a free cluster,
 *    into a bad cluster, or into a cluster already claimed by another
 *    chain (a cross link) is cut at the last good cluster.
 *  - A file whose chain is shorter than its size says is truncated to the
 *    data actually present; a chain longer than the size needs is cut.
 *  - A file with no first cluster gets size zero; a directory with no
 *    first cluster, or one whose first cluster is bad, is removed.
 *  - "." and ".." entries are checked against the directory they live in
 *    and its parent.
 *  - Clusters marked in use in the FAT but reached by no chain are freed.
 *
 * Long name entries are skipped: they carry no allocation information,
 * and a stale one is harmless to the handler. FAT12 volumes have no clean
 * shutdown bit and so are never validated automatically.
 */

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <string.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

/* Clean shutdown flags, held in the second FAT entry */
#define FAT16_CLEAN_SHUTDOWN    0x8000
#define FAT32_CLEAN_SHUTDOWN    0x08000000

/* The bad cluster marker sits just below the end-of-chain range */
#define BAD_CLUSTER(sb)         ((sb)->eoc_mark - 8)
#define IS_EOC(sb, cl)          ((cl) >= (sb)->eoc_mark - 7)

/* Directory nesting we are prepared to follow */
#define MAX_DIR_DEPTH           64

/* Never write more than this many directory entries in one pass */
#define ENTRIES_PER_SECTOR(sb)  ((sb)->sectorsize / sizeof(struct FATDirEntry))

struct Validator
{
    struct FSSuper  *sb;
    struct Globals  *glob;

    ULONG           *used;          /* one bit per data cluster */
    ULONG            fixes;         /* number of corrections written */
    ULONG            files;
    ULONG            dirs;
    ULONG            lost;

    BOOL             asked_data_loss; /* the data loss requester was shown */
    BOOL             aborted;         /* the user declined to continue */
    BOOL             incomplete;      /* some part of the tree was not walked */
};

/*
 * Bitmap helpers. Cluster numbers are the on-disk ones (2 based).
 */
static inline BOOL cluster_in_range(struct FSSuper *sb, ULONG cl)
{
    return (cl >= 2) && (cl < sb->clusters_count + 2);
}

static inline BOOL bm_test(struct Validator *v, ULONG cl)
{
    cl -= 2;
    return (v->used[cl >> 5] & (1UL << (cl & 31))) != 0;
}

static inline void bm_set(struct Validator *v, ULONG cl)
{
    cl -= 2;
    v->used[cl >> 5] |= 1UL << (cl & 31);
}

/*
 * Ask once whether the user is prepared to lose data. Returns FALSE if
 * validation should stop.
 */
static BOOL confirm_data_loss(struct Validator *v)
{
    struct Globals *glob = v->glob;

    if (v->asked_data_loss)
        return TRUE;

    v->asked_data_loss = TRUE;
    if (ErrorMessage("Device %s\nhas a damaged FAT structure.\n\n"
        "Repairing it may lose the contents\n"
        "of damaged files. Continue?", "Repair|Cancel",
        (IPTR)AROS_BSTR_ADDR(glob->devnode->dol_Name)) != 1)
    {
        v->aborted = TRUE;
        return FALSE;
    }

    return TRUE;
}

/*
 * Walk (and if needed repair) the chain starting at 'first'. Every cluster
 * visited is marked in the bitmap. If 'max' is non-zero the chain is cut
 * after that many clusters. Returns the number of clusters in the chain
 * after any repair; *pfirst is set to 0 if the first cluster itself was
 * unusable.
 */
static ULONG walk_chain(struct Validator *v, ULONG *pfirst, ULONG max)
{
    struct FSSuper *sb = v->sb;
    D(struct Globals *glob = v->glob);
    ULONG cl = *pfirst, prev = 0, count = 0, next;

    while (1)
    {
        /* Is this cluster usable at all? */
        if (!cluster_in_range(sb, cl) || bm_test(v, cl))
        {
            D(bug("[fat validate] chain cluster %lu is %s, cutting chain\n",
                cl, cluster_in_range(sb, cl) ? "cross linked" : "out of range"));

            if (!confirm_data_loss(v))
                return count;

            if (prev == 0)
                *pfirst = 0;
            else
                SET_NEXT_CLUSTER(sb, prev, sb->eoc_mark);
            v->fixes++;
            return count;
        }

        bm_set(v, cl);
        count++;

        next = GET_NEXT_CLUSTER(sb, cl);

        /* Chain longer than the file needs: cut it here */
        if (max != 0 && count >= max && !IS_EOC(sb, next))
        {
            D(bug("[fat validate] chain at %lu longer than needed, cutting"
                " at cluster %lu\n", *pfirst, cl));
            SET_NEXT_CLUSTER(sb, cl, sb->eoc_mark);
            v->fixes++;
            return count;
        }

        if (IS_EOC(sb, next))
            return count;

        /* Ran into a free or bad cluster: terminate here */
        if (next == 0 || next == BAD_CLUSTER(sb))
        {
            D(bug("[fat validate] chain at %lu runs into %s cluster after"
                " %lu, terminating\n", *pfirst,
                next == 0 ? "a free" : "a bad", cl));
            SET_NEXT_CLUSTER(sb, cl, sb->eoc_mark);
            v->fixes++;
            return count;
        }

        prev = cl;
        cl = next;
    }
}

/*
 * Directory entry helpers
 */
static inline ULONG entry_first_cluster(struct FSSuper *sb,
    struct FATDirEntry *e)
{
    ULONG cl = AROS_LE2WORD(e->first_cluster_lo);

    if (sb->type == 32)
        cl |= ((ULONG)AROS_LE2WORD(e->first_cluster_hi)) << 16;

    return cl;
}

static inline void entry_set_first_cluster(struct FATDirEntry *e, ULONG cl)
{
    e->first_cluster_lo = AROS_WORD2LE(cl & 0xffff);
    e->first_cluster_hi = AROS_WORD2LE(cl >> 16);
}

static inline BOOL entry_is_dot(struct FATDirEntry *e)
{
    return strncmp((char *)e->name, ".          ", FAT_MAX_SHORT_NAME) == 0;
}

static inline BOOL entry_is_dotdot(struct FATDirEntry *e)
{
    return strncmp((char *)e->name, "..         ", FAT_MAX_SHORT_NAME) == 0;
}

static LONG check_directory(struct Validator *v, ULONG cluster,
    ULONG parent, ULONG depth);

/*
 * Check every entry in one directory sector. Returns 1 when the end of
 * directory marker was seen, 0 otherwise, or a negative DOS error.
 */
static LONG check_dir_sector(struct Validator *v, ULONG sector,
    ULONG dir_cluster, ULONG parent, ULONG depth)
{
    struct FSSuper *sb = v->sb;
    struct Globals *glob = v->glob;
    APTR block;
    UBYTE *data;
    struct FATDirEntry *e;
    ULONG i, cl, need, got;
    BOOL dirty = FALSE, done = FALSE;
    LONG err = 0;

    block = Cache_GetBlock(sb->cache, sb->first_device_sector + sector,
        &data);
    if (block == NULL)
        return -IoErr();

    for (i = 0; i < ENTRIES_PER_SECTOR(sb) && !done && !v->aborted; i++)
    {
        e = (struct FATDirEntry *)(data + i * sizeof(struct FATDirEntry));

        if (e->name[0] == 0x00)
        {
            done = TRUE;
            break;
        }
        if (e->name[0] == 0xe5)
            continue;
        if ((e->attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
            continue;
        if (e->attr & ATTR_VOLUME_ID)
            continue;

        cl = entry_first_cluster(sb, e);

        /* On FAT12/16 the high cluster word is reserved and must be zero */
        if (sb->type != 32 && e->first_cluster_hi != 0)
        {
            D(bug("[fat validate] entry has stray high cluster word,"
                " clearing\n"));
            e->first_cluster_hi = 0;
            dirty = TRUE;
            v->fixes++;
        }

        /* The . and .. entries only ever point back at known places */
        if (e->attr & ATTR_DIRECTORY)
        {
            if (entry_is_dot(e))
            {
                if (cl != dir_cluster)
                {
                    D(bug("[fat validate] '.' points to %lu, should be %lu\n",
                        cl, dir_cluster));
                    entry_set_first_cluster(e, dir_cluster);
                    dirty = TRUE;
                    v->fixes++;
                }
                continue;
            }
            if (entry_is_dotdot(e))
            {
                /* The root is recorded as cluster 0 in ".." */
                ULONG want = (parent == sb->rootdir_cluster) ? 0 : parent;

                if (cl != want)
                {
                    D(bug("[fat validate] '..' points to %lu, should be %lu\n",
                        cl, want));
                    entry_set_first_cluster(e, want);
                    dirty = TRUE;
                    v->fixes++;
                }
                continue;
            }
        }

        if (e->attr & ATTR_DIRECTORY)
        {
            v->dirs++;

            if (cl == 0 || !cluster_in_range(sb, cl) || bm_test(v, cl))
            {
                D(bug("[fat validate] directory entry has unusable first"
                    " cluster %lu, removing entry\n", cl));
                if (!confirm_data_loss(v))
                    break;
                e->name[0] = 0xe5;
                dirty = TRUE;
                v->fixes++;
                continue;
            }

            walk_chain(v, &cl, 0);
            if (v->aborted)
                break;

            if (depth < MAX_DIR_DEPTH)
            {
                /* Release our block while recursing: the cache is small */
                if (dirty)
                {
                    Cache_MarkBlockDirty(sb->cache, block);
                    dirty = FALSE;
                }
                Cache_FreeBlock(sb->cache, block);

                err = check_directory(v, cl, dir_cluster, depth + 1);

                block = Cache_GetBlock(sb->cache,
                    sb->first_device_sector + sector, &data);
                if (block == NULL)
                    return -IoErr();
                if (err != 0)
                    break;
            }
            else
            {
                /* Whatever lives below is unseen, so the lost cluster pass
                 * must be skipped for this volume */
                D(bug("[fat validate] directory nesting too deep,"
                    " not descending\n"));
                v->incomplete = TRUE;
            }
            continue;
        }

        /* A plain file */
        v->files++;

        if (cl == 0)
        {
            if (AROS_LE2LONG(e->file_size) != 0)
            {
                D(bug("[fat validate] file with no clusters has size %lu,"
                    " zeroing\n", AROS_LE2LONG(e->file_size)));
                e->file_size = 0;
                dirty = TRUE;
                v->fixes++;
            }
            continue;
        }

        if (AROS_LE2LONG(e->file_size) == 0)
        {
            D(bug("[fat validate] empty file owns cluster %lu, detaching\n",
                cl));
            entry_set_first_cluster(e, 0);
            dirty = TRUE;
            v->fixes++;
            continue;
        }

        need = (ULONG)(((UQUAD)AROS_LE2LONG(e->file_size)
            + sb->clustersize - 1) >> sb->clustersize_bits);

        got = walk_chain(v, &cl, need);
        if (v->aborted)
            break;

        if (cl == 0)
        {
            /* First cluster was unusable: the file is now empty */
            entry_set_first_cluster(e, 0);
            e->file_size = 0;
            dirty = TRUE;
        }
        else if (got < need)
        {
            D(bug("[fat validate] file has %lu clusters but size needs %lu,"
                " truncating size\n", got, need));
            e->file_size = AROS_LONG2LE(got << sb->clustersize_bits);
            dirty = TRUE;
            v->fixes++;
        }
    }

    if (dirty)
        Cache_MarkBlockDirty(sb->cache, block);
    Cache_FreeBlock(sb->cache, block);

    if (err != 0)
        return -err;
    return done ? 1 : 0;
}

/*
 * Check one directory. 'cluster' is its first cluster, already verified
 * and marked, or the root marker for the fixed FAT12/16 root.
 */
static LONG check_directory(struct Validator *v, ULONG cluster,
    ULONG parent, ULONG depth)
{
    struct FSSuper *sb = v->sb;
    D(struct Globals *glob = v->glob);
    ULONG sector, i, cl;
    LONG res;

    D(bug("[fat validate] checking directory at cluster %lu (depth %lu)\n",
        cluster, depth));

    /* Fixed root directory on FAT12/16 */
    if (cluster == 0 && sb->type != 32)
    {
        for (i = 0; i < sb->rootdir_sectors; i++)
        {
            res = check_dir_sector(v, sb->first_rootdir_sector + i,
                0, 0, depth);
            if (res != 0)
                return res < 0 ? -res : 0;
            if (v->aborted)
                return 0;
        }
        return 0;
    }

    /* Cluster chain directory. The chain has been validated already, so
     * following it is safe */
    cl = cluster;
    while (!IS_EOC(sb, cl) && cluster_in_range(sb, cl))
    {
        sector = SECTOR_FROM_CLUSTER(sb, cl);
        for (i = 0; i < sb->cluster_sectors; i++)
        {
            res = check_dir_sector(v, sector + i, cluster, parent, depth);
            if (res != 0)
                return res < 0 ? -res : 0;
            if (v->aborted)
                return 0;
        }
        cl = GET_NEXT_CLUSTER(sb, cl);
    }

    return 0;
}

/*
 * Free every cluster the FAT says is in use but nothing refers to.
 */
static void free_lost_clusters(struct Validator *v)
{
    struct FSSuper *sb = v->sb;
    D(struct Globals *glob = v->glob);
    ULONG cl, val;

    for (cl = 2; cl < sb->clusters_count + 2; cl++)
    {
        if (bm_test(v, cl))
            continue;

        val = GET_NEXT_CLUSTER(sb, cl);
        if (val == 0 || val == BAD_CLUSTER(sb))
            continue;

        v->lost++;
        SET_NEXT_CLUSTER(sb, cl, 0);
    }

    if (v->lost != 0)
    {
        D(bug("[fat validate] freed %lu lost clusters\n", v->lost));
        v->fixes += v->lost;
    }
}

/*
 * Clean shutdown bit handling
 */
static ULONG clean_bit(struct FSSuper *sb)
{
    if (sb->type == 16)
        return FAT16_CLEAN_SHUTDOWN;
    if (sb->type == 32)
        return FAT32_CLEAN_SHUTDOWN;
    return 0;
}

BOOL IsVolumeClean(struct FSSuper *sb)
{
    ULONG bit = clean_bit(sb);

    if (bit == 0)
        return TRUE;

    return (GET_NEXT_CLUSTER(sb, 1) & bit) != 0;
}

static void set_clean_bit(struct FSSuper *sb, BOOL clean)
{
    ULONG bit = clean_bit(sb), val;

    if (bit == 0)
        return;

    val = GET_NEXT_CLUSTER(sb, 1);
    if (clean)
        val |= bit;
    else
        val &= ~bit;
    SET_NEXT_CLUSTER(sb, 1, val);
}

/*
 * Called by the cache whenever a block is dirtied. The first write after a
 * mount or a flush clears the clean shutdown bit so that a crash before the
 * next flush is noticed at the next mount.
 */
void MarkVolumeDirty(struct Globals *glob)
{
    struct FSSuper *sb = glob->sb;

    if (sb == NULL || sb->volume_dirty || sb->suppress_dirty
        || clean_bit(sb) == 0)
        return;

    /* Set the flag first: the write below dirties a cache block itself */
    sb->volume_dirty = TRUE;
    set_clean_bit(sb, FALSE);
}

/*
 * Called after a successful cache flush. Sets the clean shutdown bit and
 * returns TRUE if that dirtied the cache, in which case the caller flushes
 * once more. A volume still awaiting a successful validation keeps its
 * bit clear so that the next mount tries again.
 */
BOOL MarkVolumeClean(struct Globals *glob)
{
    struct FSSuper *sb = glob->sb;

    if (sb == NULL || !sb->volume_dirty || sb->needs_validation
        || clean_bit(sb) == 0)
        return FALSE;

    sb->suppress_dirty = TRUE;   /* the write below must not re-dirty us */
    set_clean_bit(sb, TRUE);
    sb->suppress_dirty = FALSE;
    sb->volume_dirty = FALSE;

    return TRUE;
}

/*
 * Entry point. Called from ReadFATSuper once the FAT is accessible and
 * before anything follows a cluster chain. Returns 0 on success, or a DOS
 * error if the check could not be carried out. Corrections are left in the
 * cache for the caller to flush; *changed says whether any were made and
 * *complete whether the whole volume was covered (the user may cancel).
 */
LONG ValidateVolume(struct FSSuper *sb, BOOL *changed, BOOL *complete)
{
    struct Globals *glob = sb->glob;
    struct Validator v;
    ULONG bmsize, root;
    LONG err;

    D(bug("[fat validate] validating volume, %lu clusters\n",
        sb->clusters_count));

    memset(&v, 0, sizeof(v));
    v.sb = sb;
    v.glob = glob;

    bmsize = ((sb->clusters_count + 31) >> 5) * sizeof(ULONG);
    v.used = AllocVec(bmsize, MEMF_ANY | MEMF_CLEAR);
    if (v.used == NULL)
        return ERROR_NO_FREE_STORE;

    sb->suppress_dirty = TRUE;

    /* The FAT32 root is an ordinary chain and must be checked as one */
    root = sb->rootdir_cluster;
    if (sb->type == 32)
    {
        if (!cluster_in_range(sb, root))
        {
            D(bug("[fat validate] root cluster %lu out of range\n", root));
            FreeVec(v.used);
            sb->suppress_dirty = FALSE;
            return ERROR_NOT_A_DOS_DISK;
        }
        walk_chain(&v, &root, 0);
        if (root == 0)
        {
            FreeVec(v.used);
            sb->suppress_dirty = FALSE;
            return ERROR_NOT_A_DOS_DISK;
        }
    }

    err = check_directory(&v, root, root, 0);

    if (err == 0 && !v.aborted && !v.incomplete)
        free_lost_clusters(&v);

    D(bug("[fat validate] done: %lu files, %lu dirs, %lu lost clusters,"
        " %lu fixes%s\n", v.files, v.dirs, v.lost, v.fixes,
        v.aborted ? " (aborted)" : v.incomplete ? " (incomplete)" : ""));

    FreeVec(v.used);
    sb->suppress_dirty = FALSE;

    if (changed != NULL)
        *changed = (v.fixes != 0);
    if (complete != NULL)
        *complete = (err == 0 && !v.aborted && !v.incomplete);

    return err;
}
