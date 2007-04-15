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

#define RESET_DIRHANDLE(dh)         \
    do {                            \
        RESET_HANDLE(&(dh->ioh));   \
        dh->cur_index = 0xffffffff; \
    } while(0);

LONG InitDirHandle(struct FSSuper *sb, ULONG cluster, struct DirHandle *dh) {
    dh->ioh.sb = sb;

    if (cluster == 0) {
        dh->ioh.first_cluster = sb->rootdir_cluster;
        dh->ioh.first_sector = sb->rootdir_sector;
    }
    else {
        dh->ioh.first_cluster = cluster;
        dh->ioh.first_sector = 0;
    }

    dh->ioh.block = NULL;

    RESET_DIRHANDLE(dh);

    D(bug("[fat] initialised dir handle, first cluster is %ld, first sector is %ld\n", dh->ioh.first_cluster, dh->ioh.first_sector));

    return 0;
}

LONG ReleaseDirHandle(struct DirHandle *dh) {
    RESET_DIRHANDLE(dh);
    return 0;
}

LONG GetDirEntry(struct DirHandle *dh, ULONG index, struct DirEntry *de) {
    LONG err = 0;
    ULONG nread;

    D(bug("[fat] looking for dir entry %ld in dir starting at cluster %ld\n", index, dh->ioh.first_cluster));

    /* fat dirs are limited to 2^16 entries */
    if (index >= 0x10000) {
        D(bug("[fat] request for out-of-range index, returning not found\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* setup the return object */
    de->sb = dh->ioh.sb;
    de->cluster = dh->ioh.first_cluster;
    de->index = index;
    de->pos = index * sizeof(struct FATDirEntry);

    /* get the data directly into the entry */
    err = ReadFileChunk(&(dh->ioh), de->pos, sizeof(struct FATDirEntry), (UBYTE *) &(de->e.entry), &nread);
    if (err != 0) {
        D(bug("[fat] dir entry lookup failed\n"));
        return err;
    }

    /* remember where we are for GetNextDirEntry() */
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
            RESET_DIRHANDLE(dh);
            return ERROR_OBJECT_NOT_FOUND;
        }

        D(bug("[fat] returning entry %ld\n", dh->cur_index));

        return 0;
    }

    return err;
}

LONG GetParentDir(struct DirHandle *dh, struct DirEntry *de) {
    D(bug("[fat] getting parent for directory at cluster %ld\n", dh->ioh.first_cluster));

    /* if we're already at the root, then we can't go any further */
    if (dh->ioh.first_cluster == dh->ioh.sb->rootdir_cluster) {
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
    InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(de), dh);

    return 0;
}

LONG GetDirEntryByName(struct DirHandle *dh, STRPTR name, ULONG namelen, struct DirEntry *de) {
    UBYTE buf[256];
    ULONG buflen;
    LONG err;

    D(bug("[fat] looking for dir entry with name '%.*s'\n", namelen, name));

    /* start at the start */
    RESET_DIRHANDLE(dh);

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

    D(bug("[fat] looking for entry with path '%.*s' from dir at cluster %ld\n", pathlen, path, dh->ioh.first_cluster));

    /* get back to the start of the dir */
    RESET_DIRHANDLE(dh);

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

    D(bug("[fat] now looking for entry with path '%.*s' from dir at cluster %ld\n", pathlen, path, dh->ioh.first_cluster));

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

            InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(de), dh);
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

LONG UpdateDirEntry(struct DirEntry *de) {
    struct DirHandle dh;
    LONG err = 0;
    ULONG nwritten;

    D(bug("[fat] writing dir entry %ld in dir starting at cluster %ld\n", de->index, de->cluster));

    InitDirHandle(glob->sb, de->cluster, &dh);

    err = WriteFileChunk(&(dh.ioh), de->pos, sizeof(struct FATDirEntry), (UBYTE *) &(de->e.entry), &nwritten);
    if (err != 0) {
        D(bug("[fat] dir entry update failed\n"));
        return err;
    }

    ReleaseDirHandle(&dh);

    return 0;
}



#define sb glob->sb

LONG FillFIB (struct ExtFileLock *fl, struct FileInfoBlock *fib) {
    struct DirHandle dh;
    struct DirEntry de;
    LONG result = 0;

    kprintf("\tFilling FIB data.\n");

    if (fl->dir_cluster == 0xffffffff) {
        kprintf("\t\ttype: root directory\n");
        fib->fib_DirEntryType = ST_ROOT;
    }
    else if (fl->attr & ATTR_DIRECTORY) {
        kprintf("\t\ttype: directory\n");
        fib->fib_DirEntryType = ST_USERDIR;
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
