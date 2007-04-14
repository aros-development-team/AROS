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

#include "fat_fs.h"
#include "fat_protos.h"

#undef DEBUG_DUMP

#ifdef DEBUG_DUMP
#include <ctype.h>

#define CHUNK 16

static void fat_hexdump(unsigned char *buf, int bufsz) {
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
#define fat_hexdump(b,c)
#endif

LONG ReadFileChunk(struct IOHandle *ioh, ULONG file_pos, ULONG nwant, UBYTE *data, ULONG *nread) {
    LONG err = 0;
    ULONG sector_offset, byte_offset, cluster_offset;
    struct cache_block *b;
    ULONG pos, ncopy;

    /* figure out how far into the file to look for the requested data */
    sector_offset = file_pos >> ioh->sb->sectorsize_bits;
    byte_offset = file_pos & (ioh->sb->sectorsize-1);

    /* loop until we get all we want */
    pos = 0;
    while (nwant > 0) {

        D(bug("[fat] trying to read %ld bytes (%ld sectors + %ld bytes into the file)\n", nwant, sector_offset, byte_offset));

        /* move clusters if necessary */
        cluster_offset = sector_offset >> ioh->sb->cluster_sectors_bits;
        if (ioh->cluster_offset != cluster_offset) {
            ULONG i;

            /* if we're already ahead of the wanted cluster, then we need to
             * back to the start of the cluster list */
            if (ioh->cluster_offset > cluster_offset) {
                ioh->cur_cluster = ioh->first_cluster;
                ioh->cluster_offset = 0;
            }

            D(bug("[fat] moving forward %ld clusters from cluster %ld\n", cluster_offset - ioh->cluster_offset, ioh->cur_cluster));

            /* find it */
            for (i = 0; i < cluster_offset - ioh->cluster_offset; i++) {
                /* get the next one */
                ioh->cur_cluster = GET_NEXT_CLUSTER(ioh->sb, ioh->cur_cluster);

                /* if it was free (shouldn't happen) or we hit the end of the
                 * chain, the requested data isn't here */
                if (ioh->cur_cluster == 0 || ioh->cur_cluster > ioh->sb->eoc_mark) {
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
        if (ioh->sector_offset != (sector_offset & (ioh->sb->cluster_sectors-1))) {

            /* work out how many sectors in we should be looking */
            ioh->sector_offset = sector_offset & (ioh->sb->cluster_sectors-1);

            /* simple math to find the absolute sector number */
            ioh->cur_sector = SECTOR_FROM_CLUSTER(ioh->sb, ioh->cur_cluster) + ioh->sector_offset;

            /* if this is cluster 0 and the first sector has been set, adjust
             * for it. this is hack to support fat12/16 root dirs, which live
             * in the first cluster. there's no checks to make sure we have
             * adjusted off the end of the cluster */
            if (ioh->cur_cluster == 0 && ioh->first_sector > 0) {
                ioh->sector_offset = sector_offset - ioh->first_sector;
                ioh->cur_sector = ioh->first_sector + sector_offset;

                D(bug("[fat] adjusted for cluster 0, chunks starts in sector %ld\n", ioh->cur_sector));
            }
            
            else
                D(bug("[fat] chunk starts %ld sectors into the cluster, which is sector %ld\n", ioh->sector_offset, ioh->cur_sector));
        }

        /* if we don't have the wanted block kicking around, we need to bring it
         * in from the cache */
        if (ioh->block == NULL || ioh->cur_sector != ioh->block->num) {
            if (ioh->block != NULL) {
                cache_put_block(glob->cache, ioh->block, 0);
                ioh->block = NULL;
            }

            D(bug("[fat] requesting sector %ld from cache\n", ioh->cur_sector));

            err = cache_get_block(glob->cache, glob->diskioreq->iotd_Req.io_Device, glob->diskioreq->iotd_Req.io_Unit, ioh->cur_sector, 0, &b);
            if (err > 0) {
                RESET_HANDLE(ioh);

                D(bug("[fat] couldn't load sector, returning error %ld\n", err));

                return err;
            }

            ioh->block = b;
        }

        else
            D(bug("[fat] using cached sector %ld\n", ioh->cur_sector));

        /* now copy in the data */
        ncopy = ioh->sb->sectorsize - byte_offset;
        if (ncopy > nwant) ncopy = nwant;
        CopyMem(&(ioh->block->data[byte_offset]), &(data[pos]), ncopy);

#ifdef DEBUG_DUMP
        D(bug("[fat] dump of last read, %ld bytes:\n", ncopy));
        fat_hexdump(&(data[pos]), ncopy);
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

LONG WriteFileChunk(struct IOHandle *ioh, ULONG file_pos, ULONG nwant, UBYTE *data, ULONG *nwritten) {
    LONG err = 0;
    ULONG sector_offset, byte_offset, cluster_offset;
    struct cache_block *b;
    ULONG pos, ncopy;

    /* figure out how far into the file to start */
    sector_offset = file_pos >> ioh->sb->sectorsize_bits;
    byte_offset = file_pos & (ioh->sb->sectorsize-1);

    /* loop until we've finished writing */
    pos = 0;
    while (nwant > 0) {

        D(bug("[fat] trying to write %ld bytes (%ld sectors + %ld bytes into the file)\n", nwant, sector_offset, byte_offset));

        /* move clusters if necessary */
        cluster_offset = sector_offset >> ioh->sb->cluster_sectors_bits;
        if (ioh->cluster_offset != cluster_offset) {
            ULONG i;

            /* if we're already ahead of the wanted cluster, then we need to
             * back to the start of the cluster list */
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
                if (next_cluster == 0 || next_cluster > ioh->sb->eoc_mark) {
                    D(bug("[fat] hit empty or eoc cluster, allocating another\n"));

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

                    /* search for a free cluster */
                    for (next_cluster = 0;
                         next_cluster < ioh->sb->clusters_count &&
                         GET_NEXT_CLUSTER(ioh->sb, next_cluster) != 0; next_cluster++);

                    /* if we reached the end, there's none left */
                    if (next_cluster == ioh->sb->clusters_count) {
                        D(bug("[fat] no more free clusters, we're out of space\n"));

                        RESET_HANDLE(ioh);

                        return ERROR_DISK_FULL;
                    }

                    SET_NEXT_CLUSTER(ioh->sb, ioh->cur_cluster, next_cluster);
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
        if (ioh->sector_offset != (sector_offset & (ioh->sb->cluster_sectors-1))) {

            /* work out how many sectors in we should be looking */
            ioh->sector_offset = sector_offset & (ioh->sb->cluster_sectors-1);

            /* simple math to find the absolute sector number */
            ioh->cur_sector = SECTOR_FROM_CLUSTER(ioh->sb, ioh->cur_cluster) + ioh->sector_offset;

            /* if this is cluster 0 and the first sector has been set, adjust
             * for it. this is hack to support fat12/16 root dirs, which live
             * in the first cluster. there's no checks to make sure we have
             * adjusted off the end of the cluster */
            if (ioh->cur_cluster == 0 && ioh->first_sector > 0) {
                ioh->sector_offset = sector_offset - ioh->first_sector;
                ioh->cur_sector = ioh->first_sector + sector_offset;

                D(bug("[fat] adjusted for cluster 0, chunks starts in sector %ld\n", ioh->cur_sector));
            }
            
            else
                D(bug("[fat] chunk starts %ld sectors into the cluster, which is sector %ld\n", ioh->sector_offset, ioh->cur_sector));
        }

        /* if we don't have the wanted block kicking around, we need to bring it
         * in from the cache */
        if (ioh->block == NULL || ioh->cur_sector != ioh->block->num) {
            if (ioh->block != NULL) {
                cache_put_block(glob->cache, ioh->block, 0);
                ioh->block = NULL;
            }

            D(bug("[fat] requesting sector %ld from cache\n", ioh->cur_sector));

            err = cache_get_block(glob->cache, glob->diskioreq->iotd_Req.io_Device, glob->diskioreq->iotd_Req.io_Unit, ioh->cur_sector, 0, &b);
            if (err > 0) {
                RESET_HANDLE(ioh);

                D(bug("[fat] couldn't load sector, returning error %ld\n", err));

                return err;
            }

            ioh->block = b;
        }

        else
            D(bug("[fat] using cached sector %ld\n", ioh->cur_sector));

        /* copy our data into the block */
        ncopy = ioh->sb->sectorsize - byte_offset;
        if (ncopy > nwant) ncopy = nwant;
        CopyMem(&(data[pos]), &(ioh->block->data[byte_offset]), ncopy);

#ifdef DEBUG_DUMP
        D(bug("[fat] dump of last write, %ld bytes:\n", ncopy));
        fat_hexdump(&(ioh->block->data[pos]), ncopy);
#endif

        cache_mark_block_dirty(glob->cache, ioh->block);

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
