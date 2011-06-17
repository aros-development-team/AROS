/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2011 The AROS Development Team
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

LONG GetDirEntryShortName(struct DirEntry *de, STRPTR name, ULONG *len) {
    int i;
    UBYTE *raw, *c;

    /* make sure the entry is good */
    raw = de->e.entry.name;
    if (raw[0] == 0x00 || raw[0] == 0xe5 || raw[0] == 0x20) {
        D(bug("[fat] entry name has first byte 0x%02x, returning empty short name\n", raw[0]));
        *name = '\0';
        len = 0;
        return 0;
    }

    D(bug("[fat] extracting short name for name '"); RawPutChars(raw, 11);
      bug("' (index %ld)\n", de->index));

    /* copy the chars into the return string */
    c = name;
    for (i = 0; i < 11; i++) {
        *c = tolower(raw[i]);

        /*
         * fat names are weird. the name FOO.BAR is stored as "FOO     BAR".
         * in that case we've already copied in all the spaces, and we have to
         * backup and insert the dot.
         *
         * note that spaces (0x20) are allowed in the filename, just not as the
         * first char. see FATdoc 1.03 p24 for the details. most people don't
         * know that about fat names. the point of this is to say that we
         * can't just flick forward in our copying at the first sight of a
         * space, it's technically incorrect.
         */
        if (i == 7) {
            /* backtrack to first non-space. this is safe because the first
             * char won't be space, we checked above */
            while (*c == 0x20) c--;
            
            /* forward one and drop in the dot */
            c++;
            *c = '.';
        }

        /* move along */
        c++;
    }

    /* remove any trailing spaces, and perhaps a trailing . */
    while (c[-1] == 0x20) c--;
    if (c[-1] == '.') c--;

    /* apply official hack for Japanese names */
    if (*name == 0x05) *name = 0xe5;

    /* all done */
    *c = '\0';
    *len = strlen(name);

    D(bug("[fat] extracted short name '"); RawPutChars(name, *len); bug("'\n"));

    return 0;
}

LONG GetDirEntryLongName(struct DirEntry *short_de, STRPTR name, ULONG *len) {
    UBYTE buf[256];
    int i;
    UBYTE *raw, *c;
    UBYTE checksum;
    struct DirHandle dh;
    struct DirEntry de;
    LONG index;
    UBYTE order;
    LONG err;

    /* make sure the entry is good */
    raw = short_de->e.entry.name;
    if (raw[0] == 0x00 || raw[0] == 0xe5 || raw[0] == 0x20) {
        D(bug("[fat] entry name has first byte 0x%02x, returning empty long name\n", raw[0]));
        *name = '\0';
        len = 0;
        return 0;
    }

    D(bug("[fat] looking for long name for name '%.11s' (index %ld)\n", raw, short_de->index));

    /* compute the short name checksum. this value is held in every associated
     * long name entry to help us identify it. see FATdoc 1.03 p28 */
    CALC_SHORT_NAME_CHECKSUM(raw, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* get a handle on the directory */
    InitDirHandle(short_de->sb, short_de->cluster, &dh, FALSE);

    /* loop over the long name entries */
    c = buf;
    order = 1;
    index = short_de->index - 1;
    while (index >= 0) {
        D(bug("[fat] looking for long name order 0x%02x in entry %ld\n", order, index));

        if ((err = GetDirEntry(&dh, index, &de)) != 0)
            break;

        /* make sure it's valid */
        if (!((de.e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) ||
            (de.e.long_entry.order & ~0x40) != order ||
            de.e.long_entry.checksum != checksum) {

            D(bug("[fat] bad long name entry %ld (attr 0x%02x order 0x%02x checksum 0x%02x)\n",
                  index, de.e.entry.attr, de.e.long_entry.order, de.e.long_entry.checksum));

            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }

        /* copy the characters into the name buffer. note that filename
         * entries can have null-termination, but don't have to. we take the
         * easy way out - copy everything, and bolt on an additional null just
         * in case. */

        /* XXX these are in UTF-16, but we're just taking the bottom byte.
         * that works well enough but is still a hack. if our dos ever
         * supports unicode this should be revisited */
        for (i = 0; i < 5; i++) {
	    *c = glob->from_unicode[AROS_LE2WORD(de.e.long_entry.name1[i])];
            c++;
        }
        for (i = 0; i < 6; i++) {
	    *c = glob->from_unicode[AROS_LE2WORD(de.e.long_entry.name2[i])];
            c++;
        }
        for (i = 0; i < 2; i++) {
	    *c = glob->from_unicode[AROS_LE2WORD(de.e.long_entry.name3[i])];
            c++;
        }

        /* if this is the last entry, clean up and get us out of here */
        if (de.e.long_entry.order & 0x40) {
            *c = 0;
            *len = strlen((char *) buf);
            CopyMem(buf, name, *len);

	    D(bug("[fat] extracted long name '%s'\n", buf));

            ReleaseDirHandle(&dh);

            return 0;
        }

        index--;
        order++;
    }

    ReleaseDirHandle(&dh);

    D(bug("[fat] long name construction failed\n"));

    return ERROR_OBJECT_NOT_FOUND;
}

/* set the name of an entry. this will set the long name too. it assumes
 * that there is room before the entry to store the long filename. if there
 * isn't the whole thing will fail */
LONG SetDirEntryName(struct DirEntry *short_de, STRPTR name, ULONG len) {
    UBYTE basis[11];
    ULONG nlong;
    ULONG src, dst, i, left;
    ULONG seq = 0, cur = 0;
    UBYTE tail[8];
    struct DirHandle dh;
    struct DirEntry de;
    LONG err;
    UBYTE checksum;
    UBYTE order;

    D(bug("[fat] setting name for entry index %ld to '", short_de->index);
      RawPutChars(name, len); bug("'\n"));

    nlong = NumLongNameEntries(name, len);
    D(bug("[fat] name requires %ld long name entries\n", nlong));

    /* first we generate the "basis name" of the passed in name. XXX we just
     * take the first eight characters and any three-letter extension and mash
     * them together. FATDoc 1.03 p30-31 outlines a more comprehensive
     * algorithm that handles unicode, but we're not doing unicode yet */
    
    dst = 0;

    /* strip off leading spaces and periods */
    for (src = 0; src < len; src++)
        if (name[src] != ' ' && name[src] != '.')
            break;

    /* copy the first eight chars in, ignoring spaces and stopping at period */
    if (src != len) {
        while (src < len && dst < 8 && name[src] != '.') {
            if (name[src] != ' ') {
                basis[dst] = toupper(name[src]);
                if (basis[dst] != name[src])
                    seq = 1;
                dst++;
            }
            src++;
        }
    }

    /* if there was more bytes available, then we need a tail later */
    if (src < len && name[src] != '.')
        seq = 1;

    /* make a note of the length of the left side. this gets used further down
     * to determine the position to add the tail */
    left = dst;

    /* remember the current value of src for the multiple-dot check below */
    i = src;
    
    /* pad the rest of the left side with spaces */
    for (; dst < 8; dst++)
        basis[dst] = ' ';

    /* now go to the end and track back looking for a dot */
    for (src = len-1; src >= 0 && name[src] != '.'; src--);

    /* found it */
    if (src != 0) {
        /* if this isn't the same dot we found earlier, then we need a tail */
        if (src != i)
            seq = 1;

        /* first char after the dot */
        src++;

        /* copy it in */
        while(src < len && dst < 11) {
            if (name[src] != ' ') {
                basis[dst] = toupper(name[src]);
                if (basis[dst] != name[src])
                    seq = 1;
                dst++;
            }
            src++;
        }
    }

    /* if there were more bytes available, then we'll need a tail later */
    if (src < len)
        seq = 1;

    /* pad the rest of the right side with spaces */
    for (; dst < 11; dst++)
        basis[dst] = ' ';

    D(bug("[fat] basis name is '%.11s'\n", basis));

    /* get a fresh handle on the current directory */
    InitDirHandle(short_de->sb, short_de->cluster, &dh, FALSE);

    /* if the name will require one or more entries, then our basis name is
     * actually some conversion of the real name, and we have to look to make
     * sure it's not in use */
    if (nlong > 0) {
        D(bug("[fat] searching for basis name to confirm that it's not in use\n"));

        /* loop over the entries and compare them with the basis until we find
         * a gap */
        while (1) {
            /* build a new tail if necessary */
            if (cur != seq) {
                sprintf(tail, "~%lu", (unsigned long)seq);
                while (left + strlen(tail) > 8) left--;
                CopyMem(tail, &basis[left], strlen(tail));
                cur = seq;

                D(bug("[fat] new basis name is '%.11s'\n", basis));
            }

            /* get the next entry, and bail if we hit the end of the dir */
            if ((err = GetNextDirEntry(&dh, &de)) == ERROR_OBJECT_NOT_FOUND)
                break;

            /* abort on any other error */
            if (err != 0) {
                ReleaseDirHandle(&dh);
                return err;
            }

            /* compare the two names */
            D(bug("[fat] comparing '%.11s' with '%.11s'\n", basis,
                de.e.entry.name));
            for (i = 0; i < 11; i++)
                if (de.e.entry.name[i] != basis[i])
                    break;

            /* if we reached the end, then our current basis is in use and we
             * need to generate a new one and start again */
            if (i == 11) {
                seq++;
                RESET_DIRHANDLE(&dh)
            }
        }

        D(bug("[fat] basis name '%.11s' not in use, using it\n", basis));
    }

    /* copy the new name into the original entry. we don't write it out -
     * we'll leave that for the caller to do, it's his entry */
    CopyMem(basis, short_de->e.entry.name, 11);

    /* we can stop here if no long name is required */
    if (nlong == 0) {
        D(bug("[fat] copied short name and long name not required, we're done\n"));
        ReleaseDirHandle(&dh);
        return 0;
    }

    /* compute the short name checksum */
    CALC_SHORT_NAME_CHECKSUM(basis, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* now we loop back over the previous entries and fill them in with
     * long name components */
    src = 0;
    de.index = short_de->index;
    order = 1;
    while (src < len) {
        /* get the previous entry */
        if ((err = GetDirEntry(&dh, de.index-1, &de)) != 0) {
            ReleaseDirHandle(&dh);
            return err;
        }

        /* it must be unused (or end of directory) */
        if (de.e.entry.name[0] != 0xe5 && de.e.entry.name[0] != 0x00) {
            D(bug("[fat] index %ld appears to be in use, aborting long name\n", de.index));

            /* clean up any long name entries we already added */
            while ((err = GetDirEntry(&dh, de.index+1, &de)) == 0 && 
                   (de.e.entry.attr & ATTR_LONG_NAME_MASK)) {
                de.e.entry.name[0] = 0xe5;
                if ((err = UpdateDirEntry(&de)) != 0) {
                    /* XXX corrupt */
                    ReleaseDirHandle(&dh);
                    return err;
                }
            }

            ReleaseDirHandle(&dh);
            return ERROR_NO_FREE_STORE;
        }

        D(bug("[fat] building long name entry %ld\n", de.index));

        /* copy bytes in */
        for (dst = 0; dst < 5; dst++) {
	    de.e.long_entry.name1[dst] =
                src < len ? AROS_WORD2LE(glob->to_unicode[name[src++]]) :
                (src++ == len ? 0x0000 : 0xffff);
        }

        for (dst = 0; dst < 6; dst++) {
	    de.e.long_entry.name2[dst] =
                src < len ? AROS_WORD2LE(glob->to_unicode[name[src++]]) :
                (src++ == len ? 0x0000 : 0xffff);
        }

        for (dst = 0; dst < 2; dst++) {
	    de.e.long_entry.name3[dst] =
                src < len ? AROS_WORD2LE(glob->to_unicode[name[src++]]) :
                (src++ == len ? 0x0000 : 0xffff);
        }

        /* setup the rest of the entry */
        de.e.long_entry.order = order++;
        de.e.long_entry.attr = ATTR_LONG_NAME;
        de.e.long_entry.type = 0;
        de.e.long_entry.checksum = checksum;
        de.e.long_entry.first_cluster_lo = 0;

        /* if we've reached the end then this is the last entry */
        if (src >= len)
            de.e.long_entry.order |= 0x40;

        /* write the entry out */
        UpdateDirEntry(&de);

        D(bug("[fat] wrote long name entry %ld order 0x%02x\n", de.index, de.e.long_entry.order));
    }

    ReleaseDirHandle(&dh);

    D(bug("[fat] successfully wrote short & long names\n"));

    /* Set hidden flags on .info files */
    if (strcmp(name + len - 5, ".info") == 0)
        short_de->e.entry.attr |= ATTR_HIDDEN;

    return 0;
}

/* return the number of long name entries that are required to store this name */
ULONG NumLongNameEntries(STRPTR name, ULONG len) {
#if UPPERCASE_SHORT_NAMES
    ULONG i, left;

    /* XXX because we don't handle unicode this is pretty simple - thirteen
     * characters per long entry. if we ever support unicode, then this
     * function will need to be changed to deal with each character and keep a
     * running total */

    /* if the name is standard 8.3 (or less) then we don't need any long name
     * entries - the name can be contained within the standard entry */
    if (len <= 12) {
        left = 0;

        for (i = 0; i < 8 && i < len; i++) {
            if (name[i] == '.')
                break;
            if (name[i] != toupper(name[i]))
                break;
            left++;
        }

        if (i == len)
            return 0;

        if (name[i] == '.') {
            for (i = 0; i < 3 && left + 1 + i < len; i++)
                if (name[left+1+i] != toupper(name[left+1+i]))
                    break;

            if (left + 1 + i == len)
                return 0;
        }
    }
#endif

    return ((len-1) / 13) + 1;
}
