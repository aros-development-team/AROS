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

#include <string.h>    

#include "fat_fs.h"
#include "fat_protos.h"

LONG GetDirShortName(struct DirEntry *de, STRPTR name, ULONG *len) {
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

    D(bug("[fat] original name is '%10s'\n", raw));

    /* copy the chars into the return string */
    c = name;
    for (i = 0; i < 11; i++) {
        *c = raw[i];

        /*
         * fat names are weird. the name FOO.BAR is stored as "FOO     BAR".
         * in that case we've already copied in all the spaces, and we have to
         * backup and insert the dot.
         *
         * note that spaces (0x20) is allowed in the filename, just not as the
         * first char. see FATdoc 1.03 p24 for the details. most people don't
         * know that about fat names. the point of this is to say that we
         * can't just flick forward in our copying at the first sight of a
         * space, its technically incorrect.
         *
         * XXX it occurs to me just now that AROS may not like spaces in its
         * names. in that case spaces should probably be converted to
         * underscores. that or our dos should be changed so it does allow
         * spaces. either way, thats a project for another time.
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
    
    /* all done */
    *c = '\0';
    *len = strlen(name);

    D(bug("[fat] extracted short name '%.*s'\n", *len, name));

    return 0;
}

LONG GetDirLongName(struct DirEntry *short_de, UBYTE *name, ULONG *len) {
    UBYTE buf[256];
    int i;
    UBYTE *raw, *c;
    UBYTE checksum;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG index;
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

    D(bug("[fat] looking for long name for name '%10s' (index %ld)\n", raw, short_de->index));

    /* compute the short name checksum. this value is held in every associated
     * long name entry to help us identify it. see FATdoc 1.03 p28 */
    checksum = 0;
    for (i = 0; i < 11; i++)
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + raw[i];

    D(bug("[fat] long name checksum is 0x%02x\n", checksum));

    /* get a handle on the directory */
    InitDirHandle(short_de->sb, short_de->cluster, &dh);

    /* loop over the long name entries */
    c = buf;
    order = 1;
    index = short_de->index - 1;
    while (index >= 0) {
        D(bug("[fat] looking for long name order 0x%02x in entry %ld\n", order, index));

        if ((err = GetDirEntry(&dh, index, &de)) != 0)
            break;

        /* make sure its valid */
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
            *c = de.e.long_entry.name1[i << 1];
            c++;
        }
        for (i = 0; i < 6; i++) {
            *c = de.e.long_entry.name2[i << 1];
            c++;
        }
        for (i = 0; i < 2; i++) {
            *c = de.e.long_entry.name3[i << 1];
            c++;
        }

        /* if this is the last entry, clean up and get us out of here */
        if (de.e.long_entry.order & 0x40) {
            *c = 0;
            *len = strlen((char *) buf);
            CopyMem(buf, name, *len);
            return 0;
        }

        index--;
        order++;
    }

    D(bug("[fat] long name construction failed\n"));

    return ERROR_OBJECT_NOT_FOUND;
}
