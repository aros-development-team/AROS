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
#include <proto/exec.h>

#include "cache.h"

#include "fat_fs.h"
#include "fat_protos.h"

#define RESET_HANDLE(dh)                                     \
    do {                                                     \
        dh->cluster_offset = dh->sector_offset = 0xffffffff; \
        dh->cur_index = 0xffffffff;                          \
        if (dh->block != NULL) {                             \
            cache_put_block(glob->cache, dh->block, 0);      \
            dh->block = NULL;                                \
        }                                                    \
    } while (0);

LONG InitDirHandle(struct FSSuper *sb, ULONG cluster, struct DirHandle *dh) {
    dh->sb = sb;

    if (cluster == 0) {
        dh->first_cluster = sb->rootdir_cluster;
        dh->first_sector = sb->rootdir_sector;
    }
    else {
        dh->first_cluster = cluster;
        dh->first_sector = 0;
    }

    dh->block = NULL;

    RESET_HANDLE(dh);

    D(bug("[fat] initialised dir handle, first cluster is %ld, first sector is %ld\n", dh->first_cluster, dh->first_sector));

    return 0;
}

LONG ReleaseDirHandle(struct DirHandle *dh) {
    RESET_HANDLE(dh);
    return 0;
}

LONG GetDirEntry(struct DirHandle *dh, ULONG index, struct DirEntry *de) {
    LONG err = 0;
    ULONG offset;
    struct cache_block *b;

    D(bug("[fat] looking for dir entry %ld in dir starting at cluster %ld\n", index, dh->first_cluster));

    /* fat dirs are limited to 2^16 entries */
    if (index >= 0x10000) {
        D(bug("[fat] request for out-of-range index, returning not found\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }
    
    /* figure out how many sectors into the directory this entry is */
    offset = (index * sizeof(struct FATDirEntry)) >> dh->sb->sectorsize_bits;

    D(bug("[fat] entry is %ld sectors into the dir\n", offset));

    /* if we're not at the right sector, we need to move */
    if (!((dh->cluster_offset == offset >> dh->sb->cluster_sectors_bits) &&
          (dh->sector_offset == (offset & (dh->sb->cluster_sectors_bits-1))))) {

        ULONG i;

        /* if the first cluster is 0 then this is a fat12/16 rootdir then we
         * have the absolute first rootdir sector available so we can just
         * bounce off that */
        if (dh->first_cluster == 0) {
            /* XXX use BPB_RootEntCnt to detect us going out of bounds and
             * return not found */
            dh->cluster_offset = offset >> dh->sb->cluster_sectors_bits;
            dh->cur_cluster = 0;
            dh->sector_offset = offset - dh->first_sector;
            dh->cur_sector = dh->first_sector + offset;
        }

        /* otherwise we've gotta try a bit harder */
        else {
            /* back to the start */
            dh->cur_cluster = dh->first_cluster;

            /* work out how many clusters in we should be looking */
            dh->cluster_offset = offset >> dh->sb->cluster_sectors_bits;

            D(bug("[fat] entry is %ld clusters in, finding it\n", dh->cluster_offset));

            /* find it */
            for (i = 0; i < dh->cluster_offset; i++) {
                /* get the next one */
                dh->cur_cluster = GetFatEntry(dh->cur_cluster);

                /* if it was free (shouldn't happen) or we hit the end of the
                 * chain, then the requested entry doesn't exist */
                if (dh->cur_cluster == 0 || dh->cur_cluster > dh->sb->eoc_mark) {
                    D(bug("[fat] hit empty or eoc cluster %ld, entry not found\n", dh->cur_cluster));

                    RESET_HANDLE(dh);

                    return ERROR_OBJECT_NOT_FOUND;
                }
            }

            D(bug("[fat] moved to cluster %ld\n", dh->cur_cluster));

            /* work out how many sectors in we should be looking */
            dh->sector_offset = offset & (dh->sb->cluster_sectors-1);

            /* simple math to find the absolute sector number */
            dh->cur_sector = SECTOR_FROM_CLUSTER(dh->sb, dh->cur_cluster) + dh->sector_offset;

            D(bug("[fat] entry is %ld sectors into the cluster, which is sector %ld\n", dh->sector_offset, dh->cur_sector));
        }
    }

    /* if we don't have the wanted block kicking around, we need to bring it
     * in from the cache */
    if (dh->block == NULL || dh->cur_sector != dh->block->num) {
        if (dh->block != NULL) {
            cache_put_block(glob->cache, dh->block, 0);
            dh->block = NULL;
        }

        D(bug("[fat] loading dir sector %ld\n", dh->cur_sector));

        err = cache_get_block(glob->cache, glob->diskioreq->iotd_Req.io_Device, glob->diskioreq->iotd_Req.io_Unit, dh->cur_sector, 0, &b);
        if (err > 0) {
            RESET_HANDLE(dh);

            D(bug("[fat] couldn't load sector, return error %ld\n", err));

            return err;
        }

        dh->block = b;
    }

    else
        D(bug("[fat] using cached sector %ld\n", dh->cur_sector));


    /* setup the return object */
    de->sb = dh->sb;
    de->cluster = dh->first_cluster;
    de->index = index;
    de->sector = dh->cur_sector;
    de->offset = (index * sizeof(struct FATDirEntry)) & (dh->sb->sectorsize-1);

    /* copy the data in */
    CopyMem(&(dh->block->data[de->offset]), &(de->e.entry), sizeof(struct FATDirEntry));

    dh->cur_index = index;

    /* done! */
    return 0;
}

LONG GetNextDirEntry(struct DirHandle *dh, struct DirEntry *de) {
    LONG err;

    D(bug("[fat] looking for next entry after index %ld\n", dh->cur_index));

    /* cur_index defaults to -1, so this will do the right thing even on a
     * fresh dirhandle */
    while ((err = GetDirEntry(dh, dh->cur_index + 1, de)) == 0) {
        /* skip unused entries */
        if (de->e.entry.name[0] == 0xe5) {
            D(bug("[fat] entry %ld is empty, skipping it\n", dh->cur_index));
            continue;
        }

        /* this flag will be set for both volume name entries and long
         * filename entries. either way we want to skip them */
        if (de->e.entry.attr & ATTR_VOLUME_ID) {
            D(bug("[fat] entry %ld is a volume name or long filename, skipping it\n", dh->cur_index));
            continue;
        }

        /* end of directory, there is no next entry */
        if (de->e.entry.name[0] == 0x00) {
            D(bug("[fat] entry %ld is end-of-directory marker, we're done\n", dh->cur_index));
            RESET_HANDLE(dh);
            return ERROR_OBJECT_NOT_FOUND;
        }

        D(bug("[fat] returning entry %ld\n", dh->cur_index));

        return 0;
    }

    return err;
}

LONG GetParentDir(struct DirHandle *dh, struct DirEntry *de) {
    D(bug("[fat] getting parent for directory at cluster %ld\n", dh->first_cluster));

    /* if we're already at the root, then we can't go any further */
    if (dh->first_cluster == dh->sb->rootdir_cluster) {
        D(bug("[fat] trying go up past the root, so entry not found\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* otherwise, the next cluster is held in the '..' entry, which is
     * entry #1 */
    GetDirEntry(dh, 1, de);

    /* make sure its actually the parent dir entry */
    if (!((de->e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_DIRECTORY) ||
        strncmp((char *) de->e.entry.name, "..         ", 11) != 0) {
        D(bug("[fat] entry index 1 does not have name '..', can't go up\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* take us up */
    InitDirHandle(dh->sb, FIRST_FILE_CLUSTER(de), dh);

    return 0;
}

LONG GetDirEntryByName(struct DirHandle *dh, STRPTR name, ULONG namelen, struct DirEntry *de) {
    UBYTE buf[256];
    ULONG buflen;
    LONG err;

    D(bug("[fat] looking for dir entry with name '%.*s'\n", namelen, name));

    /* start at the start */
    RESET_HANDLE(dh);

    /* loop through the entries until we find a match */
    while ((err = GetNextDirEntry(dh, de)) == 0) {
        /* compare with the short name first, since we already have it */
        GetDirShortName(de, buf, &buflen);
        if (namelen == buflen && strnicmp((char *) name, (char *) buf, buflen) == 0) {
            D(bug("[fat] matched short name '%.*s' at entry %ld, returning\n", buflen, buf, dh->cur_index));
            return 0;
        }

        /* no match, extract the long name and compare with that instead */
        GetDirLongName(de, buf, &buflen);
        if (namelen == buflen && strnicmp((char *) name, (char *) buf, buflen) == 0) {
            D(bug("[fat] matched long name '%.*s' at entry %ld, returning\n", buflen, buf, dh->cur_index));
            return 0;
        }
    }

    return err;
}

LONG GetDirEntryByPath(struct DirHandle *dh, STRPTR path, ULONG pathlen, struct DirEntry *de) {
    LONG err;
    ULONG len;

    D(bug("[fat] looking for entry with path '%.*s' from dir at cluster %ld\n", pathlen, path, dh->first_cluster));

    /* get back to the start of the dir */
    RESET_HANDLE(dh);

    /* eat up leading slashes */
    while (pathlen >= 0 && path[0] == '/') {
        D(bug("[fat] leading '/', moving up to eat it\n"));

        if ((err = GetParentDir(dh, de)) != 0)
            return err;

        path++;
        pathlen--;

        /* if we ate the whole path, bail out now with the current entry
         * pointing to its parent dir */
        if (pathlen == 0) {
            D(bug("[fat] explicit request for parent dir, returning it\n"));
            return 0;
        }
    }

    /* eat up trailing slashes */
    while (pathlen > 0) {
        if (path[pathlen-1] != '/')
            break;
        pathlen--;
    }

    D(bug("[fat] now looking for entry with path '%.*s' from dir at cluster %ld\n", pathlen, path, dh->first_cluster));

    /* each time around the loop we find one dir/file in the full path */
    while (pathlen > 0) {

        /* zoom forward and find the first dir separator */
        for (len = 0; len < pathlen && path[len] != '/'; len++);

        D(bug("[fat] remaining path is '%.*s' (%d bytes), current chunk is '%.*s' (%d bytes)\n", pathlen, path, pathlen, len, path, len));

        /* if the first character is a /, or they've asked for '..', then we
         * have to go up a level */
        if (len == 0) {

            /* get the parent dir, and bail if we've gone past it (ie we are
             * the root) */
            if ((err = GetParentDir(dh, de)) != 0)
                break;
        }

        /* otherwise, we want to search the current directory for this name */
        else {
            if ((err = GetDirEntryByName(dh, path, len, de)) != 0)
                return ERROR_OBJECT_NOT_FOUND;

            /* if the current chunk if all the name we have left, then we found it */
            if (len == pathlen) {
                D(bug("[fat] found the entry, returning it\n"));
                return 0;
            }

            /* more to do, so this entry had better be a directory */
            if (!(de->e.entry.attr & ATTR_DIRECTORY)) {
                D(bug("[fat] '%.*s' is not a directory, so can't go any further\n", len, path));
                return ERROR_OBJECT_WRONG_TYPE;
            }

            InitDirHandle(dh->sb, FIRST_FILE_CLUSTER(de), dh);
        }

        /* move up the buffer */
        path += len;
        pathlen -= len;

        /* a / here is either the path seperator or the directory we just went
         * up. either way, we have to ignore it */
        if (pathlen > 0 && path[0] == '/') {
            path++;
            pathlen--;
        }
    }

    D(bug("[fat] empty path supplied, so naturally not found\n"));

    return ERROR_OBJECT_NOT_FOUND;
}



#define sb glob->sb

LONG FillFIB (struct ExtFileLock *fl, struct FileInfoBlock *fib) {
    struct DirHandle dh;
    struct DirEntry de;
    LONG result = 0;

    kprintf("\tFilling FIB data.\n");

    if ((fl->attr & (ATTR_DIRECTORY | ATTR_REALENTRY)) == (ATTR_DIRECTORY | ATTR_REALENTRY)) {
        kprintf("\t\ttype: directory\n");
        fib->fib_DirEntryType = ST_USERDIR;
    }
    else if (fl->attr & ATTR_ROOTDIR) {
        kprintf("\t\ttype: root directory\n");
        fib->fib_DirEntryType = ST_ROOT;
    }
    else {
        kprintf("\t\ttype: file\n");
        fib->fib_DirEntryType = ST_FILE;
    }

    kprintf("\t\tsize: %ld\n", fl->size);

    fib->fib_Size = fl->size;
    fib->fib_NumBlocks = ((fl->size + (sb->clustersize - 1)) >> sb->clustersize_bits) << sb->cluster_sectors_bits;
    fib->fib_EntryType = fib->fib_DirEntryType;
    fib->fib_DiskKey = 0xfffffffflu; //fl->entry;

    if (fib->fib_DirEntryType == ST_ROOT)
        CopyMem(&sb->volume.create_time, &fib->fib_Date, sizeof(struct DateStamp));
    else {
        InitDirHandle(sb, fl->dir_cluster, &dh);
        GetDirEntry(&dh, fl->dir_entry, &de);
        ConvertDate(de.e.entry.write_date, de.e.entry.write_time, &fib->fib_Date);
        ReleaseDirHandle(&dh);
    }

    memcpy(fib->fib_FileName, fl->name, 108);
    kprintf("\t\tname (len %ld) ", fib->fib_FileName[0]); knprints(&fib->fib_FileName[1], fib->fib_FileName[0]);

    fib->fib_Protection = 0;
    if (fl->attr & ATTR_READ_ONLY) fib->fib_Protection |= (FIBF_DELETE | FIBF_WRITE);
    if (fl->attr & ATTR_ARCHIVE)   fib->fib_Protection |= FIBF_ARCHIVE;

    fib->fib_Comment[0] = '\0';

    return result;
}
