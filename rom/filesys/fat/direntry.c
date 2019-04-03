/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2019 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <aros/macros.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "cache.h"

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_DIRENTRY
#include "debug.h"

LONG InitDirHandle(struct FSSuper *sb, ULONG cluster, struct DirHandle *dh,
    BOOL reuse, struct Globals *glob)
{
    /* 'dh' may or may not be initialised when this is called. if it is, then
     * it probably has a valid cache block that we need to free, but we
     * wouldn't know. we test the superblock pointer to figure out if it's
     * valid or not */
    if (reuse && (dh->ioh.sb == sb))
    {
        D(bug("[fat] reusing directory handle\n"));
        if (dh->ioh.block != NULL)
        {
            Cache_FreeBlock(sb->cache, dh->ioh.block);
            dh->ioh.block = NULL;
        }
    }
    else
    {
        dh->ioh.sb = sb;
        dh->ioh.block = NULL;
    }

    if (cluster == 0)
    {
        dh->ioh.first_cluster = sb->rootdir_cluster;
        dh->ioh.first_sector = sb->rootdir_sector;
    }
    else
    {
        dh->ioh.first_cluster = cluster;
        dh->ioh.first_sector = 0;
    }

    RESET_DIRHANDLE(dh);
    dh->ioh.cur_sector = dh->ioh.first_sector;
    dh->ioh.sector_offset = 0;

    D(bug("[fat] initialised dir handle, first cluster is %ld,"
        " first sector is %ld\n", dh->ioh.first_cluster,
        dh->ioh.first_sector));

    return 0;
}

LONG ReleaseDirHandle(struct DirHandle *dh, struct Globals *glob)
{
    D(bug("[fat] releasing dir handle (cluster %ld)\n",
        dh->ioh.first_cluster));
    RESET_DIRHANDLE(dh);

    return 0;
}

LONG GetDirEntry(struct DirHandle *dh, ULONG index, struct DirEntry *de,
    struct Globals *glob)
{
    LONG err = 0;
    ULONG nread;

    D(bug("[fat] looking for dir entry %ld in dir starting at cluster %ld\n",
        index, dh->ioh.first_cluster));

    /* FAT dirs are limited to 2^16 entries */
    if (index >= 0x10000)
    {
        D(bug("[fat] request for out-of-range index, returning not found\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* Set up the return object */
    de->sb = dh->ioh.sb;
    de->cluster = dh->ioh.first_cluster;
    de->index = index;
    de->pos = index * sizeof(struct FATDirEntry);

    /* Get the data directly into the entry */
    err = ReadFileChunk(&(dh->ioh), de->pos, sizeof(struct FATDirEntry),
        (UBYTE *) &(de->e.entry), &nread);
    if (err != 0)
    {
        D(bug("[fat] dir entry lookup failed\n"));
        return err;
    }

    /* Remember where we are for GetNextDirEntry() */
    dh->cur_index = index;

    /* Done! */
    return 0;
}

LONG GetNextDirEntry(struct DirHandle *dh, struct DirEntry *de,
    struct Globals *glob)
{
    LONG err;

    D(bug("[fat] looking for next entry after index %ld\n", dh->cur_index));

    /* 'cur_index' defaults to -1, so this will do the right thing even on a
     * fresh dirhandle */
    while ((err = GetDirEntry(dh, dh->cur_index + 1, de, glob)) == 0)
    {
        /* End of directory, there is no next entry */
        if (de->e.entry.name[0] == 0x00)
        {
            D(bug("[fat] entry %ld is end-of-directory marker, we're done\n",
                dh->cur_index));
            RESET_DIRHANDLE(dh);
            return ERROR_OBJECT_NOT_FOUND;
        }

        /* Skip unused entries */
        if (de->e.entry.name[0] == 0xe5)
        {
            D(bug("[fat] entry %ld is empty, skipping it\n", dh->cur_index));
            continue;
        }

        /* This flag will be set for both volume name entries and long
         * filename entries. Either way we want to skip them */
        if (de->e.entry.attr & ATTR_VOLUME_ID)
        {
            D(bug("[fat] entry %ld is a volume name or long filename,"
                " skipping it\n", dh->cur_index));
            continue;
        }

        /* Ignore the . and .. entries */
        if (de->e.entry.name[0] == '.' && ((de->index == 0
            && strncmp((char *)de->e.entry.name, ".          ",
                FAT_MAX_SHORT_NAME) == 0)
            || (de->index == 1
            && strncmp((char *)de->e.entry.name, "..         ",
                FAT_MAX_SHORT_NAME) == 0)))
        {
            D(bug("[fat] skipping . or .. entry\n"));
            continue;
        }

        D(bug("[fat] returning entry %ld\n", dh->cur_index));

        return 0;
    }

    return err;
}

LONG GetParentDir(struct DirHandle *dh, struct DirEntry *de,
    struct Globals *glob)
{
    LONG err = 0;
    ULONG cluster;

    D(bug("[fat] getting parent for directory at cluster %ld\n",
        dh->ioh.first_cluster));

    /* If we're already at the root, then we can't go any further */
    if (dh->ioh.first_cluster == dh->ioh.sb->rootdir_cluster)
    {
        D(bug("[fat] trying to go up past the root, so entry not found\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* Otherwise, the next cluster is held in the '..' entry, which is
     * entry #1 */
    GetDirEntry(dh, 1, de, glob);

    /* Make sure it's actually the parent dir entry */
    if (((de->e.entry.attr & ATTR_DIRECTORY) == 0) ||
        strncmp((char *)de->e.entry.name, "..         ", FAT_MAX_SHORT_NAME)
        != 0)
    {
        D(bug("[fat] entry index 1 does not have name '..', can't go up\n"));
        D(bug("[fat] actual name: '%.*s', attrs: 0x%x\n", FAT_MAX_SHORT_NAME,
            de->e.entry.name, de->e.entry.attr));
        return ERROR_OBJECT_NOT_FOUND;
    }

    /* Take us up */
    InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(de), dh, TRUE, glob);

    /* Get handle on grandparent dir so we can find entry with parent's
     * name */
    if (dh->ioh.first_cluster != dh->ioh.sb->rootdir_cluster)
    {
        D(bug("[fat] getting grandparent, first cluster is %ld,"
            " root cluster is %ld\n", dh->ioh.first_cluster,
            dh->ioh.sb->rootdir_cluster));
        cluster = dh->ioh.first_cluster;
        GetDirEntry(dh, 1, de, glob);
        InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(de), dh, TRUE, glob);

        err = GetDirEntryByCluster(dh, cluster, de, glob);
    }

    return err;
}

LONG GetDirEntryByCluster(struct DirHandle *dh, ULONG cluster,
    struct DirEntry *de, struct Globals *glob)
{
    LONG err;

    D(bug("[fat] looking for dir entry with first cluster %lu\n", cluster));

    /* Start at the start */
    RESET_DIRHANDLE(dh);

    /* Loop through the entries until we find a match */
    while ((err = GetNextDirEntry(dh, de, glob)) == 0)
    {
        if (de->e.entry.first_cluster_hi == (cluster >> 16)
            && de->e.entry.first_cluster_lo == (cluster & 0xffff))
        {
            D(bug("[fat] matched starting cluster at entry %ld, returning\n",
                dh->cur_index));
            return 0;
        }
    }

    D(bug("[fat] dir entry with first cluster %lu not found\n", cluster));
    return err;
}

LONG GetDirEntryByName(struct DirHandle *dh, STRPTR name, ULONG namelen,
    struct DirEntry *de, struct Globals *glob)
{
    UBYTE buf[256];
    ULONG buflen;
    LONG err;

    D(bug("[fat] looking for dir entry with name '%s'\n", name));

    /* Start at the start */
    RESET_DIRHANDLE(dh);

    /* Loop through the entries until we find a match */
    while ((err = GetNextDirEntry(dh, de, glob)) == 0)
    {
        /* Compare with the short name first, since we already have it */
        GetDirEntryShortName(de, buf, &buflen, glob);
        if (namelen == buflen
            && strnicmp((char *)name, (char *)buf, buflen) == 0)
        {
            D(bug("[fat] matched short name '%s' at entry %ld, returning\n",
                buf, dh->cur_index));
            return 0;
        }

        /* No match, extract the long name and compare with that instead */
        GetDirEntryLongName(de, buf, &buflen);
        if (namelen == buflen
            && strnicmp((char *)name, (char *)buf, buflen) == 0)
        {
            D(bug("[fat] matched long name '%s' at entry %ld, returning\n",
                buf, dh->cur_index));
            return 0;
        }
    }

    return err;
}

LONG GetDirEntryByPath(struct DirHandle *dh, STRPTR path, ULONG pathlen,
    struct DirEntry *de, struct Globals *glob)
{
    LONG err;
    ULONG len, i;

    D(
        bug("[fat] looking for entry with path '");
        RawPutChars(path, pathlen);
        bug("' from dir at cluster %ld\n", dh->ioh.first_cluster);
    )

    /* Get back to the start of the dir */
    RESET_DIRHANDLE(dh);

    /* If it starts with a volume specifier (or just a :), remove it and get
     * us back to the root dir */
    for (i = 0; i < pathlen; i++)
        if (path[i] == ':')
        {
            D(bug(
                "[fat] path has volume specifier, moving to the root dir\n"));

            pathlen -= (i + 1);
            path = &path[i + 1];

            InitDirHandle(dh->ioh.sb, 0, dh, TRUE, glob);

            /* If we were called with simply ":" as the name we will return
               immediately after this, so we prepare a fictional direntry for
               such a case. Note that we fill only fields which are actually
               used in our handler */
            de->cluster = 0;
            de->index = -1;     /* WARNING! Dummy index */
            de->e.entry.attr = ATTR_DIRECTORY;
            de->e.entry.first_cluster_hi = 0;
            de->e.entry.first_cluster_lo = 0;

            break;
        }

    D(
        bug("[fat] now looking for entry with path '");
        RawPutChars(path, pathlen);
        bug("' from dir at cluster %ld\n", dh->ioh.first_cluster);
    )

    /* Each time around the loop we find one dir/file in the full path */
    while (pathlen > 0)
    {

        /* Zoom forward and find the first dir separator */
        for (len = 0; len < pathlen && path[len] != '/'; len++);

        D(
            bug("[fat] remaining path is '");
            RawPutChars(path, pathlen);
            bug("' (%d bytes), current chunk is '", pathlen);
            RawPutChars(path, len); bug("' (%d bytes)\n", len);
        )

        /* If the first character is a '/', then we have to go up a level */
        if (len == 0)
        {
            /* Get the parent dir, and bale if we've gone past it (i.e. we are
             * the root) */
            if ((err = GetParentDir(dh, de, glob)) != 0)
                return err;
        }

        /* Otherwise, we want to search the current directory for this name */
        else
        {
            if ((err = GetDirEntryByName(dh, path, len, de, glob)) != 0)
                return ERROR_OBJECT_NOT_FOUND;
        }

        /* Move up the buffer */
        path += len;
        pathlen -= len;

        /* A '/' here is either the path separator or the directory we just
         * went up. Either way, we have to ignore it */
        if (pathlen > 0 && path[0] == '/')
        {
            path++;
            pathlen--;
        }

        if (pathlen > 0)
        {
            /* More to do, so this entry had better be a directory */
            if (!(de->e.entry.attr & ATTR_DIRECTORY))
            {
                D(bug("[fat] '%.*s' is not a directory,"
                    " so can't go any further\n", len, path));
                return ERROR_OBJECT_WRONG_TYPE;
            }

            InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(de), dh, TRUE,
                glob);
        }
    }

    /* Nothing left, so we've found it */
    D(bug("[fat] found the entry, returning it\n"));
    return 0;
}

LONG UpdateDirEntry(struct DirEntry *de, struct Globals *glob)
{
    struct DirHandle dh;
    LONG err = 0;
    ULONG nwritten;

    D(bug("[fat] writing dir entry %ld in dir starting at cluster %ld\n",
        de->index, de->cluster));

    InitDirHandle(glob->sb, de->cluster, &dh, FALSE, glob);

    err =
        WriteFileChunk(&(dh.ioh), de->pos, sizeof(struct FATDirEntry),
        (UBYTE *) &(de->e.entry), &nwritten);
    if (err != 0)
    {
        D(bug("[fat] dir entry update failed\n"));
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    ReleaseDirHandle(&dh, glob);

    return 0;
}

LONG AllocDirEntry(struct DirHandle *dh, ULONG gap, struct DirEntry *de,
    struct Globals *glob)
{
    ULONG nwant;
    LONG err;
    ULONG nfound;
    BOOL clusteradded = FALSE;

    /* Find out how many entries we need */
    nwant = gap + 1;

    D(bug("[fat] need to find room for %ld contiguous entries\n", nwant));

    /* Get back to the start of the dir */
    RESET_DIRHANDLE(dh);

    /* Search the directory until we find a large enough gap */
    nfound = 0;
    de->index = -1;
    while (nfound < nwant)
    {
        err = GetDirEntry(dh, de->index + 1, de, glob);

        /* If we can't get the entry, then we ran off the end, so there's no
         * space left */
        if (err == ERROR_OBJECT_NOT_FOUND)
        {
            D(bug("[fat] ran off the end of the directory, no space left\n"));
            return ERROR_NO_FREE_STORE;
        }

        /* Return any other error direct */
        if (err != 0)
            return err;

        /* If it's unused, make a note */
        if (de->e.entry.name[0] == 0xe5)
        {
            nfound++;
            continue;
        }

        /* If we hit end-of-directory, then we can shortcut it */
        if (de->e.entry.name[0] == 0x00)
        {
            ULONG last;

            if (de->index + nwant >= 0x10000)
            {
                D(bug("[fat] hit end-of-directory marker,"
                    " but there's not enough room left after it\n"));
                return ERROR_NO_FREE_STORE;
            }

            D(bug("[fat] found end-of-directory marker,"
                " making space after it\n"));

            last = de->index + nwant;
            do
            {
                if (GetDirEntry(dh, de->index + 1, de, glob) != 0)
                    clusteradded = TRUE;
                de->e.entry.name[0] = 0x00;
                UpdateDirEntry(de, glob);
            }
            while (de->index != last);

            D(bug("[fat] new end-of-directory is entry %ld\n", de->index));

            /* Clear all remaining entries in any new cluster added */
            if (clusteradded)
                while (GetDirEntry(dh, de->index + 1, de, glob) == 0)
                {
                    memset(&de->e.entry, 0, sizeof(struct FATDirEntry));
                    UpdateDirEntry(de, glob);
                }

            /* Get the previous entry; this is the base (short name) entry */
            GetDirEntry(dh, last - 1, de, glob);

            break;
        }

        /* Anything else is an in-use entry, so reset our count */
        nfound = 0;
    }

    D(bug("[fat] found a gap, base (short name) entry is %ld\n",
        de->index));
    return 0;
}

LONG CreateDirEntry(struct DirHandle *dh, STRPTR name, ULONG namelen,
    UBYTE attr, ULONG cluster, struct DirEntry *de, struct Globals *glob)
{
    ULONG gap;
    LONG err;

    D(
        bug("[fat] creating dir entry (name '");
        RawPutChars(name, namelen);
        bug("' attr 0x%02x cluster %ld)\n", attr, cluster);
    )

    /* Find out how many extra entries we need for the long name */
    gap = NumLongNameEntries(name, namelen);

    /* Search for a suitable unused entry */
    err = AllocDirEntry(dh, gap, de, glob);
    if (err != 0)
        return err;

    /* Build the entry */
    FillDirEntry(de, attr, cluster, glob);

    SetDirEntryName(de, name, namelen);

    if ((err = UpdateDirEntry(de, glob)) != 0)
    {
        D(bug(
            "[fat] couldn't update base directory entry, creation failed\n"));
        return err;
    }

    D(bug("[fat] created dir entry %ld\n", de->index));

    return 0;
}

void FillDirEntry(struct DirEntry *de, UBYTE attr, ULONG cluster,
    struct Globals *glob)
{
    struct DateStamp ds;
    UWORD cdate, ctime;

    de->e.entry.attr = attr;
    de->e.entry.nt_res = 0;

    DateStamp(&ds);
    cdate = de->e.entry.create_date;
    ctime = de->e.entry.create_time;
    ConvertDOSDate(&ds, &cdate,
        &ctime, glob);
    de->e.entry.create_date = cdate;
    de->e.entry.create_time = ctime;

    de->e.entry.write_date = cdate;
    de->e.entry.write_time = ctime;
    de->e.entry.last_access_date = cdate;
    de->e.entry.create_time_tenth = ds.ds_Tick % (TICKS_PER_SECOND * 2)
        / (TICKS_PER_SECOND / 10);

    de->e.entry.first_cluster_lo = cluster & 0xffff;
    de->e.entry.first_cluster_hi = cluster >> 16;

    de->e.entry.file_size = 0;
}

LONG DeleteDirEntry(struct DirEntry *de, struct Globals *glob)
{
    struct DirHandle dh;
    UBYTE checksum;
    ULONG order;
    LONG err;

    InitDirHandle(glob->sb, de->cluster, &dh, FALSE, glob);

    /* Calculate the short name checksum before we trample on the name */
    CALC_SHORT_NAME_CHECKSUM(de->e.entry.name, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* Mark the short entry free */
    de->e.entry.name[0] = 0xe5;
    UpdateDirEntry(de, glob);

    D(bug("[fat] deleted short name entry\n"));

    /* Now we loop over the previous entries, looking for matching long name
     * entries and killing them */
    order = 1;
    while ((err = GetDirEntry(&dh, de->index - 1, de, glob)) == 0)
    {

        /* See if this is a matching long name entry. If it's not, we're done */
        if (!((de->e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) ||
            (de->e.long_entry.order & ~0x40) != order ||
            de->e.long_entry.checksum != checksum)

            break;

        /* Kill it */
        de->e.entry.name[0] = 0xe5;
        UpdateDirEntry(de, glob);

        order++;
    }

    D(bug("[fat] deleted %ld long name entries\n", order - 1));

    ReleaseDirHandle(&dh, glob);

    return err;
}

LONG FillFIB(struct ExtFileLock *fl, struct FileInfoBlock *fib,
    struct Globals *glob)
{
    struct FSSuper *sb = glob->sb;
    struct GlobalLock *gl = (fl != NULL ? fl->gl : &sb->info->root_lock);
    struct DirHandle dh;
    struct DirEntry de;
    LONG result = 0;
    int len;

    D(bug("\tFilling FIB data.\n"));

    if (gl == &sb->info->root_lock)
    {
        D(bug("\t\ttype: root directory\n"));
        fib->fib_DirEntryType = ST_ROOT;
    }
    else if (gl->attr & ATTR_DIRECTORY)
    {
        D(bug("\t\ttype: directory\n"));
        fib->fib_DirEntryType = ST_USERDIR;
    }
    else
    {
        D(bug("\t\ttype: file\n"));
        fib->fib_DirEntryType = ST_FILE;
    }

    D(bug("\t\tsize: %ld\n", gl->size));

    fib->fib_Size = gl->size;
    fib->fib_NumBlocks = ((gl->size + (sb->clustersize - 1))
        >> sb->clustersize_bits) << sb->cluster_sectors_bits;
    fib->fib_EntryType = fib->fib_DirEntryType;
    fib->fib_DiskKey = 0xfffffffflu;    //fl->entry;

    if (fib->fib_DirEntryType == ST_ROOT)
        CopyMem(&sb->volume.create_time, &fib->fib_Date,
            sizeof(struct DateStamp));
    else
    {
        InitDirHandle(sb, gl->dir_cluster, &dh, FALSE, glob);
        GetDirEntry(&dh, gl->dir_entry, &de, glob);
        ConvertFATDate(de.e.entry.write_date, de.e.entry.write_time,
            &fib->fib_Date, glob);
        ReleaseDirHandle(&dh, glob);
    }

    len = gl->name[0] <= 106 ? gl->name[0] : 106;
    CopyMem(gl->name, fib->fib_FileName, len + 1);
    fib->fib_FileName[len + 1] = '\0';
    D(bug("\t\tname (len %ld) %s\n", len, fib->fib_FileName + 1));

    fib->fib_Protection = 0;
    if (gl->attr & ATTR_READ_ONLY)
        fib->fib_Protection |= (FIBF_DELETE | FIBF_WRITE);
    if (gl->attr & ATTR_ARCHIVE)
        fib->fib_Protection |= FIBF_ARCHIVE;

    fib->fib_Comment[0] = 0;
#if DEBUG_NAMES
    CopyMem(gl->shortname, fib->fib_Comment, gl->shortname[0] + 1);
#endif

    return result;
}
