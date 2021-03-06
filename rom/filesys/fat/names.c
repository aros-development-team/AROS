/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright (C) 2006 Marek Szyprowski
 * Copyright (C) 2007-2015 The AROS Development Team
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

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_NAMES
#include "debug.h"

/*
 * Characters allowed in a new name:
 * 0 = disallowed
 * 1 = long names only
 * 2 = allowed
 */
const UBYTE allowed_ascii[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 0, 2, 2, 2, 2, 2,
    2, 2, 0, 1, 1, 2, 1, 0,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 0, 1, 0, 1, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 0, 1, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 0, 2, 2, 0
};

LONG GetDirEntryShortName(struct DirEntry *de, STRPTR name, ULONG *len,
    struct Globals *glob)
{
    int i;
    UBYTE *raw, *c;

    /* Make sure the entry is good */
    raw = de->e.entry.name;
    if (raw[0] == 0x00 || raw[0] == 0xe5 || raw[0] == 0x20)
    {
        D(bug("[fat] entry name has first byte 0x%02x,"
            " returning empty short name\n", raw[0]));
        *name = '\0';
        len = 0;
        return 0;
    }

    D(
        bug("[fat] extracting short name for name '");
        RawPutChars(raw, FAT_MAX_SHORT_NAME);
        bug("' (index %ld)\n", de->index);
    )

    /* Copy the chars into the return string */
    c = name;
    for (i = 0; i < FAT_MAX_SHORT_NAME; i++)
    {
        *c = tolower(raw[i]);

        /*
         * FAT names are weird. the name FOO.BAR is stored as "FOO     BAR".
         * In that case we've already copied in all the spaces, and we have to
         * backup and insert the dot.
         *
         * Note that spaces (0x20) are allowed in the filename, just not as the
         * first char. See FATdoc 1.03 p24 for the details. Most people don't
         * know that about fat names. The point of this is to say that we
         * can't just flick forward in our copying at the first sight of a
         * space; it's technically incorrect.
         */
        if (i == 7)
        {
            /* Backtrack to first non-space. This is safe because the first
             * char won't be a space; we checked above */
            while (*c == 0x20)
                c--;

            /* Forward one and drop in the dot */
            c++;
            *c = '.';
        }

        /* Move along */
        c++;
    }

    /* Remove any trailing spaces, and perhaps a trailing . */
    while (c[-1] == 0x20)
        c--;
    if (c[-1] == '.')
        c--;

    /* Apply official hack for Japanese names */
    if (*name == 0x05)
        *name = 0xe5;

    /* All done */
    *c = '\0';
    *len = strlen(name);

    D(
        bug("[fat] extracted short name '");
        RawPutChars(name, *len);
        bug("'\n");
    )

    return 0;
}

LONG GetDirEntryLongName(struct DirEntry *short_de, STRPTR name,
    ULONG *len)
{
    struct Globals *glob = short_de->sb->glob;
    UBYTE buf[256];
    int i;
    UBYTE *raw, *c;
    UBYTE checksum;
    struct DirHandle dh;
    struct DirEntry de;
    LONG index;
    UBYTE order;
    LONG err;

    /* Make sure the entry is good */
    raw = short_de->e.entry.name;
    if (raw[0] == 0x00 || raw[0] == 0xe5 || raw[0] == 0x20)
    {
        D(bug("[fat] entry name has first byte 0x%02x,"
            " returning empty long name\n", raw[0]));
        *name = '\0';
        len = 0;
        return 0;
    }

    D(bug("[fat] looking for long name for name '%.*s' (index %ld)\n",
        FAT_MAX_SHORT_NAME, raw, short_de->index));

    /* Compute the short name checksum. This value is held in every associated
     * long name entry to help us identify it. See FATdoc 1.03 p28 */
    CALC_SHORT_NAME_CHECKSUM(raw, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* Get a handle on the directory */
    InitDirHandle(short_de->sb, short_de->cluster, &dh, FALSE, glob);

    /* Loop over the long name entries */
    c = buf;
    order = 1;
    index = short_de->index - 1;
    while (index >= 0)
    {
        D(bug("[fat] looking for long name order 0x%02x in entry %ld\n",
            order, index));

        if ((err = GetDirEntry(&dh, index, &de, glob)) != 0)
            break;

        /* Make sure it's valid */
        if (!((de.e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) ||
            (de.e.long_entry.order & ~0x40) != order ||
            de.e.long_entry.checksum != checksum)
        {

            D(bug("[fat] bad long name entry %ld"
                " (attr 0x%02x order 0x%02x checksum 0x%02x)\n",
                index, de.e.entry.attr, de.e.long_entry.order,
                de.e.long_entry.checksum));

            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }

        /* Copy the characters into the name buffer. Note that filename
         * entries can have null-termination, but don't have to. We take the
         * easy way out - copy everything, and bolt on an additional null just
         * in case. */

        /* XXX: these are in UTF-16, but we're just taking the bottom byte.
         * That works well enough but is still a hack. If our DOS ever
         * supports unicode, this should be revisited */
        for (i = 0; i < 5; i++)
        {
            *c = glob->from_unicode[AROS_LE2WORD(de.e.long_entry.name1[i])];
            c++;
        }
        for (i = 0; i < 6; i++)
        {
            *c = glob->from_unicode[AROS_LE2WORD(de.e.long_entry.name2[i])];
            c++;
        }
        for (i = 0; i < 2; i++)
        {
            *c = glob->from_unicode[AROS_LE2WORD(de.e.long_entry.name3[i])];
            c++;
        }

        /* If this is the last entry, clean up and get us out of here */
        if (de.e.long_entry.order & 0x40)
        {
            *c = 0;
            *len = strlen((char *)buf);
            CopyMem(buf, name, *len);

            D(bug("[fat] extracted long name '%s'\n", buf));

            ReleaseDirHandle(&dh, glob);

            return 0;
        }

        index--;
        order++;
    }

    ReleaseDirHandle(&dh, glob);

    D(bug("[fat] long name construction failed\n"));

    return ERROR_OBJECT_NOT_FOUND;
}

/* Set the name of an entry. This will set the long name too. It assumes
 * that there is room before the entry to store the long filename. If there
 * isn't, the whole thing will fail */
LONG SetDirEntryName(struct DirEntry *short_de, STRPTR name, ULONG len)
{
    struct Globals *glob = short_de->sb->glob;
    UBYTE basis[FAT_MAX_SHORT_NAME];
    ULONG nlong;
    LONG src, dst, i, left, root_end;
    ULONG seq = 0, cur = 0;
    UBYTE tail[8];
    struct DirHandle dh;
    struct DirEntry de;
    LONG err;
    UBYTE checksum;
    UBYTE order;

    D(
        bug("[fat] setting name for entry index %ld to '", short_de->index);
        RawPutChars(name, len);
        bug("'\n");
    )

    /* Discard leading spaces */
    while (len > 0 && name[0] == ' ')
        name++, len--;

    /* Discard trailing spaces and dots */
    while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '.'))
        len--;

    /* Check for empty name and reserved names "." and ".." */
    if (len == 0 || (name[0] == '.'
        && (len == 1 || (name[1] == '.' && len == 2))))
        return ERROR_INVALID_COMPONENT_NAME;

    /* Check for illegal characters */
    for (i = 0; i < len; i++)
        if (name[i] <= 127 && allowed_ascii[name[i]] == 0)
            return ERROR_INVALID_COMPONENT_NAME;

    nlong = NumLongNameEntries(name, len);
    D(bug("[fat] name requires %ld long name entries\n", nlong));

    /* First we generate the "basis name" of the passed in name. XXX: we just
     * take the first eight characters and any three-letter extension and mash
     * them together. FATDoc 1.03 p30-31 outlines a more comprehensive
     * algorithm that handles unicode, but we're not doing unicode yet */

    dst = 0;

    /* Strip off leading periods */
    for (src = 0; src < len; src++)
        if (name[src] != '.')
            break;
    if (src != 0)
        seq = 1;

    /* Copy the first eight chars in, ignoring spaces and stopping at period */
    if (src != len)
    {
        while (src < len && dst < 8 && name[src] != '.')
        {
            if (name[src] != ' ')
            {
                if (allowed_ascii[name[src]] == 1)
                {
                    basis[dst] = '_';
                    seq = 1;
                }
                else
                    basis[dst] = toupper(name[src]);
                dst++;
            }
            src++;
        }
    }

    /* If there were more bytes available, then we need a tail later */
    if (src < len && name[src] != '.')
        seq = 1;

    /* Make a note of the length of the left side. This gets used further down
     * to determine the position to add the tail */
    left = dst;

    /* Remember the current value of src for the multiple-dot check below */
    root_end = src;

    /* Pad the rest of the left side with spaces */
    for (; dst < 8; dst++)
        basis[dst] = ' ';

    /* Now go to the end and track back looking for a dot */
    for (i = len - 1; i >= src && name[i] != '.'; i--);

    /* Found it */
    if (i >= src)
    {
        /* If this isn't the same dot we found earlier, then we need a tail */
        if (i != root_end)
            seq = 1;

        /* First char after the dot */
        src = i + 1;

        /* Copy it in */
        while (src < len && dst < FAT_MAX_SHORT_NAME)
        {
            if (name[src] != ' ')
            {
                if (allowed_ascii[name[src]] == 1)
                {
                    basis[dst] = '_';
                    seq = 1;
                }
                else
                    basis[dst] = toupper(name[src]);
                dst++;
            }
            src++;
        }

        /* If there were more bytes available, then we'll need a tail later */
        if (src < len)
            seq = 1;
    }

    /* Pad the rest of the right side with spaces */
    for (; dst < FAT_MAX_SHORT_NAME; dst++)
        basis[dst] = ' ';

    D(bug("[fat] basis name is '%.*s'\n", FAT_MAX_SHORT_NAME, basis));

    /* Get a fresh handle on the current directory */
    InitDirHandle(short_de->sb, short_de->cluster, &dh, FALSE, glob);

    /* If the name will require one or more entries, then our basis name is
     * actually some conversion of the real name, and we have to look to make
     * sure it's not in use */
    if (nlong > 0)
    {
        D(bug("[fat] searching for basis name to confirm that"
            " it's not in use\n"));

        /* Loop over the entries and compare them with the basis until we find
         * a gap */
        while (1)
        {
            /* Build a new tail if necessary */
            if (cur != seq)
            {
                sprintf(tail, "~%lu", (unsigned long)seq);
                while (left + strlen(tail) > 8)
                    left--;
                CopyMem(tail, &basis[left], strlen(tail));
                cur = seq;

                D(bug("[fat] new basis name is '%.*s'\n",
                    FAT_MAX_SHORT_NAME, basis));
            }

            /* Get the next entry, and bail if we hit the end of the dir */
            if ((err = GetNextDirEntry(&dh, &de, glob))
                == ERROR_OBJECT_NOT_FOUND)
                break;

            /* Abort on any other error */
            if (err != 0)
            {
                ReleaseDirHandle(&dh, glob);
                return err;
            }

            /* Compare the two names */
            D(bug("[fat] comparing '%.*s' with '%.*s'\n",
                FAT_MAX_SHORT_NAME, basis,
                FAT_MAX_SHORT_NAME, de.e.entry.name));
            for (i = 0; i < FAT_MAX_SHORT_NAME; i++)
                if (de.e.entry.name[i] != basis[i])
                    break;

            /* If we reached the end, then our current basis is in use and we
             * need to generate a new one and start again */
            if (i == FAT_MAX_SHORT_NAME)
            {
                seq++;
                RESET_DIRHANDLE(&dh);
            }
        }

        D(bug("[fat] basis name '%.*s' not in use, using it\n",
            FAT_MAX_SHORT_NAME, basis));
    }

    /* Copy the new name into the original entry. We don't write it out -
     * we'll leave that for the caller to do, it's his entry */
    CopyMem(basis, short_de->e.entry.name, FAT_MAX_SHORT_NAME);

    /* We can stop here if no long name is required */
    if (nlong == 0)
    {
        D(bug("[fat] copied short name and long name not required,"
            " we're done\n"));
        ReleaseDirHandle(&dh, glob);
        return 0;
    }

    /* Compute the short name checksum */
    CALC_SHORT_NAME_CHECKSUM(basis, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* Now we loop back over the previous entries and fill them in with
     * long name components */
    src = 0;
    de.index = short_de->index;
    order = 1;
    while (src < len)
    {
        /* Get the previous entry */
        if ((err = GetDirEntry(&dh, de.index - 1, &de, glob)) != 0)
        {
            ReleaseDirHandle(&dh, glob);
            return err;
        }

        /* It must be unused (or end of directory) */
        if (de.e.entry.name[0] != 0xe5 && de.e.entry.name[0] != 0x00)
        {
            D(bug("[fat] index %ld appears to be in use, aborting long name\n",
                de.index));

            /* Clean up any long name entries we already added */
            while ((err = GetDirEntry(&dh, de.index + 1, &de, glob)) == 0
                && (de.e.entry.attr & ATTR_LONG_NAME_MASK))
            {
                de.e.entry.name[0] = 0xe5;
                if ((err = UpdateDirEntry(&de, glob)) != 0)
                {
                    /* FIXME: leaving in corrupt state? */
                    ReleaseDirHandle(&dh, glob);
                    return err;
                }
            }

            ReleaseDirHandle(&dh, glob);
            return ERROR_NO_FREE_STORE;
        }

        D(bug("[fat] building long name entry %ld\n", de.index));

        /* Copy bytes in */
        for (dst = 0; dst < 5; dst++)
        {
            de.e.long_entry.name1[dst] =
                src < len ? AROS_WORD2LE(glob->to_unicode[name[src++]]) :
                (src++ == len ? 0x0000 : 0xffff);
        }

        for (dst = 0; dst < 6; dst++)
        {
            de.e.long_entry.name2[dst] =
                src < len ? AROS_WORD2LE(glob->to_unicode[name[src++]]) :
                (src++ == len ? 0x0000 : 0xffff);
        }

        for (dst = 0; dst < 2; dst++)
        {
            de.e.long_entry.name3[dst] =
                src < len ? AROS_WORD2LE(glob->to_unicode[name[src++]]) :
                (src++ == len ? 0x0000 : 0xffff);
        }

        /* Set up the rest of the entry */
        de.e.long_entry.order = order++;
        de.e.long_entry.attr = ATTR_LONG_NAME;
        de.e.long_entry.type = 0;
        de.e.long_entry.checksum = checksum;
        de.e.long_entry.first_cluster_lo = 0;

        /* If we've reached the end then this is the last entry */
        if (src >= len)
            de.e.long_entry.order |= 0x40;

        /* Write the entry out */
        UpdateDirEntry(&de, glob);

        D(bug("[fat] wrote long name entry %ld order 0x%02x\n", de.index,
            de.e.long_entry.order));
    }

    ReleaseDirHandle(&dh, glob);

    D(bug("[fat] successfully wrote short & long names\n"));

    /* Set hidden flags on .info files */
    if (strcmp(name + len - 5, ".info") == 0)
        short_de->e.entry.attr |= ATTR_HIDDEN;

    return 0;
}

/* Return the number of long name entries that are required to store this
 * name */
ULONG NumLongNameEntries(STRPTR name, ULONG len)
{
    return ((len - 1) / 13) + 1;
}
