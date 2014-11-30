/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2011 The AROS Development Team
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

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_FILE
#include "debug.h"

#if defined(DEBUG_DUMP) && DEBUG_DUMP != 0
#include <ctype.h>

#define CHUNK 16

static void HexDump(unsigned char *buf, int bufsz) {
  int i,j;
  int count;

  /* do this in chunks of CHUNK bytes */
  for (i=0; i<bufsz; i+=CHUNK) {
    /* show the offset */
    bug("0x%06x  ", i);

    /* max of CHUNK or remaining bytes */
    count = ((bufsz-i) > CHUNK ? CHUNK : bufsz-i);

    /* show the bytes */
    for (j=0; j<count; j++) {
      if (j==CHUNK/2) bug(" ");
      bug("%02x ",buf[i+j]);
    }

    /* pad with spaces if less than CHUNK */
    for (j=count; j<CHUNK; j++) {
      if (j==CHUNK/2) bug(" ");
      bug("   ");
    }

    /* divider between hex and ascii */
    bug(" ");

    for (j=0; j<count; j++)
      bug("%c",(isprint(buf[i+j]) ? buf[i+j] : '.'));

    bug("\n");
  }
}
#else
#define HexDump(b,c)
#endif

LONG ReadFileChunk(struct IOHandle *ioh, ULONG file_pos, ULONG nwant,
    UBYTE *data, ULONG *nread) {
    ULONG sector_offset, byte_offset, cluster_offset, old_sector;
    APTR b;
    ULONG pos, ncopy;
    UBYTE *p;

    /* files with no data can't be read from */
    if (ioh->first_cluster == 0xffffffff && nwant > 0) {
        D(bug("[fat] file has no first cluster, so nothing to read!\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* figure out how far into the file to look for the requested data */
    sector_offset = file_pos >> ioh->sb->sectorsize_bits;
    byte_offset = file_pos & (ioh->sb->sectorsize-1);

    /* loop until we get all we want */
    pos = 0;
    while (nwant > 0) {

        D(bug("[fat] trying to read %ld bytes (%ld sectors + %ld bytes into the file)\n", nwant, sector_offset, byte_offset));

        /* move clusters if necessary */
        cluster_offset = sector_offset >> ioh->sb->cluster_sectors_bits;
        if (ioh->cluster_offset != cluster_offset && ioh->first_cluster != 0) {
            ULONG i;

            /* if we're already ahead of the wanted cluster, then we need to
             * go back to the start of the cluster list */
            if (ioh->cluster_offset > cluster_offset) {
                ioh->cur_cluster = ioh->first_cluster;
                ioh->cluster_offset = 0;
            }

            D(bug("[fat] moving forward %ld clusters from cluster %ld\n",
                cluster_offset - ioh->cluster_offset, ioh->cur_cluster));

            /* find it */
            for (i = 0; i < cluster_offset - ioh->cluster_offset; i++) {
                /* get the next one */
                ioh->cur_cluster = GET_NEXT_CLUSTER(ioh->sb, ioh->cur_cluster);

                /* if it was free (shouldn't happen) or we hit the end of the
                 * chain, the requested data isn't here */
                if (ioh->cur_cluster == 0
                    || ioh->cur_cluster >= ioh->sb->eoc_mark - 7) {
                    D(bug("[fat] hit empty or eoc cluster, no more file left\n"));

                    RESET_HANDLE(ioh);

                    return ERROR_OBJECT_NOT_FOUND;
                }
            }

            /* remember how far in we are now */
            ioh->cluster_offset = cluster_offset;

            D(bug("[fat] moved to cluster %ld\n", ioh->cur_cluster));

            /* reset the sector offset so the sector recalc gets triggered */
            ioh->sector_offset = 0xffffffff;
        }

        /* recalculate the sector location if we moved */
        old_sector = ioh->cur_sector;
        if (ioh->sector_offset != (sector_offset & (ioh->sb->cluster_sectors-1))
            || ioh->first_cluster == 0) {

            /* work out how many sectors in we should be looking */
            ioh->sector_offset = sector_offset & (ioh->sb->cluster_sectors-1);

            /* simple math to find the absolute sector number */
            ioh->cur_sector = SECTOR_FROM_CLUSTER(ioh->sb, ioh->cur_cluster) + ioh->sector_offset;

            /* if the first cluster is zero, we use sector addressing instead
             * of clusters. this is a hack to support fat12/16 root dirs, which
             * live before the data region */
            if (ioh->first_cluster == 0) {
                ioh->sector_offset = sector_offset - ioh->first_sector;
                ioh->cur_sector = ioh->first_sector + sector_offset;

                D(bug("[fat] adjusted for cluster 0, chunk starts in sector %ld\n", ioh->cur_sector));
            }
            else
                D(bug("[fat] chunk starts %ld sectors into the cluster, which is sector %ld\n", ioh->sector_offset, ioh->cur_sector));
        }

        /* if we don't have the wanted block kicking around, we need to bring it
         * in from the cache */
        if (ioh->block == NULL || ioh->cur_sector != old_sector) {
            if (ioh->block != NULL) {
                Cache_FreeBlock(ioh->sb->cache, ioh->block);
                ioh->block = NULL;
            }

            D(bug("[fat] requesting sector %ld from cache\n", ioh->cur_sector));

            b = Cache_GetBlock(ioh->sb->cache,
                ioh->sb->first_device_sector + ioh->cur_sector, &p);
            if (b == NULL) {
                RESET_HANDLE(ioh);

                D(bug("[fat] couldn't load sector, returning error %ld\n",
                    IoErr()));

                return IoErr();
            }

            ioh->block = b;
            ioh->data = p;
        }

        else
            D(bug("[fat] using cached sector %ld\n", ioh->cur_sector));

        /* now copy in the data */
        ncopy = ioh->sb->sectorsize - byte_offset;
        if (ncopy > nwant) ncopy = nwant;
        CopyMem(ioh->data + byte_offset, data + pos, ncopy);

#if defined(DEBUG_DUMP) && DEBUG_DUMP != 0
        D(bug("[fat] dump of last read, %ld bytes:\n", ncopy));
        HexDump(&(data[pos]), ncopy);
#endif

        pos += ncopy;
        nwant -= ncopy;

        D(bug("[fat] copied %ld bytes, want %ld more\n", ncopy, nwant));

        if (nwant > 0) {
            sector_offset++;
            byte_offset = 0;
        }
    }

    *nread = pos;

    return 0;
}

LONG WriteFileChunk(struct IOHandle *ioh, ULONG file_pos, ULONG nwant,
    UBYTE *data, ULONG *nwritten) {
    LONG err = 0;
    ULONG sector_offset, byte_offset, cluster_offset, old_sector;
    struct cache_block *b;
    ULONG pos, ncopy;
    UBYTE *p;

    /* figure out how far into the file to start */
    sector_offset = file_pos >> ioh->sb->sectorsize_bits;
    byte_offset = file_pos & (ioh->sb->sectorsize-1);

    /* loop until we've finished writing */
    pos = 0;
    while (nwant > 0) {

        D(bug("[fat] trying to write %ld bytes (%ld sectors + %ld bytes into the file)\n", nwant, sector_offset, byte_offset));

        /* move clusters if necessary */
        cluster_offset = sector_offset >> ioh->sb->cluster_sectors_bits;
        if (ioh->cluster_offset != cluster_offset && ioh->first_cluster != 0) {
            ULONG i;

            /* if we have no first cluster, this is a new file. we allocate
             * the first cluster and then update the ioh */
            if (ioh->first_cluster == 0xffffffff) {
                ULONG cluster;

                D(bug("[fat] no first cluster, allocating one\n"));

                /* allocate a cluster */
                if ((err = FindFreeCluster(ioh->sb, &cluster)) != 0) {
                    RESET_HANDLE(ioh);
                    return err;
                }

                /* mark the cluster used */
                AllocCluster(ioh->sb, cluster);

                /* now setup the ioh */
                ioh->first_cluster = cluster;
                RESET_HANDLE(ioh);
            }

            /* if we're already ahead of the wanted cluster, then we need to
             * go back to the start of the cluster list */
            if (ioh->cluster_offset > cluster_offset) {
                ioh->cur_cluster = ioh->first_cluster;
                ioh->cluster_offset = 0;
            }

            D(bug("[fat] moving forward %ld clusters from cluster %ld\n", cluster_offset - ioh->cluster_offset, ioh->cur_cluster));

            /* find it */
            for (i = 0; i < cluster_offset - ioh->cluster_offset; i++) {
                /* get the next one */
                ULONG next_cluster = GET_NEXT_CLUSTER(ioh->sb, ioh->cur_cluster);

                /* if it was free (shouldn't happen) or we hit the end of the
                 * chain, there is no next cluster, so we have to allocate a
                 * new one */
                if (next_cluster == 0
                    || next_cluster >= ioh->sb->eoc_mark - 7) {
                    D(bug("[fat] hit empty or eoc cluster, allocating another\n"));

                    if ((err = FindFreeCluster(ioh->sb, &next_cluster)) != 0) {
                        RESET_HANDLE(ioh);
                        return err;
                    }

                    /* link the current cluster to the new one */
                    SET_NEXT_CLUSTER(ioh->sb, ioh->cur_cluster, next_cluster);

                    /* and mark the new one used */
                    AllocCluster(ioh->sb, next_cluster);

                    ioh->cur_cluster = next_cluster;

                    D(bug("[fat] allocated cluster %d\n", next_cluster));
                }

                else
                    ioh->cur_cluster = next_cluster;
            }

            /* remember how far in we are now */
            ioh->cluster_offset = cluster_offset;

            D(bug("[fat] moved to cluster %ld\n", ioh->cur_cluster));

            /* reset the sector offset so the sector recalc gets triggered */
            ioh->sector_offset = 0xffffffff;
        }

        /* recalculate the sector location if we moved */
        old_sector = ioh->cur_sector;
        if (ioh->sector_offset != (sector_offset & (ioh->sb->cluster_sectors-1))
            || ioh->first_cluster == 0) {

            /* work out how many sectors in we should be looking */
            ioh->sector_offset = sector_offset & (ioh->sb->cluster_sectors-1);

            /* simple math to find the absolute sector number */
            ioh->cur_sector = SECTOR_FROM_CLUSTER(ioh->sb, ioh->cur_cluster) + ioh->sector_offset;

            /* if the first cluster is zero, we use sector addressing instead
             * of clusters. this is a hack to support fat12/16 root dirs, which
             * live before the data region */
            if (ioh->first_cluster == 0) {
                ioh->sector_offset = sector_offset - ioh->first_sector;
                ioh->cur_sector = ioh->first_sector + sector_offset;

                D(bug("[fat] adjusted for cluster 0, chunk starts in sector %ld\n", ioh->cur_sector));
            }
            else
                D(bug("[fat] chunk starts %ld sectors into the cluster, which is sector %ld\n", ioh->sector_offset, ioh->cur_sector));
        }

        /* if we don't have the wanted block kicking around, we need to bring it
         * in from the cache */
        if (ioh->block == NULL || ioh->cur_sector != old_sector) {
            if (ioh->block != NULL) {
                Cache_FreeBlock(ioh->sb->cache, ioh->block);
                ioh->block = NULL;
            }

            D(bug("[fat] requesting sector %ld from cache\n", ioh->cur_sector));

            b = Cache_GetBlock(ioh->sb->cache, ioh->sb->first_device_sector
                + ioh->cur_sector, &p);
            if (b == NULL) {
                RESET_HANDLE(ioh);

                D(bug("[fat] couldn't load sector, returning error %ld\n", err));

                return IoErr();
            }

            ioh->block = b;
            ioh->data = p;
        }
        else
            D(bug("[fat] using cached sector %ld\n", ioh->cur_sector));

        /* copy our data into the block */
        ncopy = ioh->sb->sectorsize - byte_offset;
        if (ncopy > nwant) ncopy = nwant;
        CopyMem(data + pos, ioh->data + byte_offset, ncopy);

#if defined(DEBUG_DUMP) && DEBUG_DUMP != 0
        D(bug("[fat] dump of last write, %ld bytes:\n", ncopy));
        HexDump(&(ioh->data[byte_offset]), ncopy);
#endif

        Cache_MarkBlockDirty(ioh->sb->cache, ioh->block);

        pos += ncopy;
        nwant -= ncopy;

        D(bug("[fat] wrote %ld bytes, want %ld more\n", ncopy, nwant));

        if (nwant > 0) {
            sector_offset++;
            byte_offset = 0;
        }
    }

    *nwritten = pos;

    return 0;
}

