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

#define sb glob->sb

LONG File_Read(struct ExtFileLock *fl, ULONG togo, void *buffer, LONG *result) {
    struct Extent *ext = fl->data_ext;
    ULONG start_block, start_offset, block;
    APTR p = buffer;
    LONG err = 0;

    start_block = fl->pos >> sb->sectorsize_bits;
    start_offset = fl->pos & (sb->sectorsize - 1);

    kprintf("\tReading file data from offset %ld length %ld, starting from file block %ld\n", fl->pos, togo, start_block);

    if (start_block < ext->offset || start_block >= ext->offset + ext->count) {
        if ((err = SeekExtent(sb, ext, start_block)) != 0) {
            kprintf("\tExtent seek error: %ld\n", err);
            *result = 0;
            return err;
        }
    }

    while (togo) {
        kprintf("\tDoing read loop - extent offset %ld\n", ext->offset);

        if (start_offset || togo < sb->sectorsize) {
            void *buff;
            ULONG bytes;

            block = ext->sector + start_block - ext->offset;
            bytes = sb->sectorsize - start_offset;

            if (bytes > togo)
                bytes = togo;

            kprintf("\tReading %ld bytes from block %ld offset %ld\n", bytes, block, start_offset);

            if ((buff=FS_AllocBlock()) != NULL) {
                if ((err = FS_GetBlock(block, buff)) == 0) {
                    CopyMem(buff + start_offset, p, bytes);

                    start_offset = 0;
                    p += bytes;
                    togo -= bytes;
                    start_block++;
                }
                FS_FreeBlock(buff);            
            }
            else
                err = ERROR_NO_FREE_STORE;
        }

        else if (togo >= sb->sectorsize) {
            ULONG ex_left;
            ULONG togo_blocks = togo >> sb->sectorsize_bits;
            ULONG bytes;

            block = ext->sector + start_block - ext->offset;
            ex_left = ext->count - start_block + ext->offset;

            if (togo_blocks > ex_left)
                togo_blocks = ex_left;

            kprintf("\tReading %ld blocks, starting from block %ld\n", togo_blocks, block);

            FS_GetBlocks(block, p, togo_blocks);

            bytes = togo_blocks << sb->sectorsize_bits;
            togo -= bytes;
            p += bytes;
            start_block += togo_blocks;
        }

        if (err == 0 && togo != 0 && start_block == ext->offset + ext->count) {
            if ((err = NextExtent(sb, ext)) != 0) {
                kprintf("\tNextExtent failed %ld\n", err);
                break;
            }
        }
    }

    *result = (p-buffer);

    return err;
}
