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

#include <string.h>    
#include <ctype.h>

#include "fat_fs.h"
#include "fat_protos.h"

LONG GetShortName(struct DirEntry *de, STRPTR dest, UBYTE *dlen) {
    int i,j;

    for (i=0, j=0; i < 8; i++, j++) {
        dest[j] = tolower(de->name[i]);
        if (de->name[i] == ' ')
            break;
    }

    if (de->name[8] != ' ')
        dest[j++] = '.';

    for (i = 8;i < 11; i++, j++) {
        dest[j] = tolower(de->name[i]);
        if (de->name[i] == ' ')
            break;
    }
    dest[j]='\0';

    kprintf("\tShort filename: %s len %ld\n", (LONG)dest, j);

    *dlen = j;
    return 0;
}

static inline UBYTE GetShortNameChecksum(struct DirEntry *de) {
    UWORD len;
    UBYTE sum;
    UBYTE *name = de->name;

    sum=0;
    for (len = 11; len != 0; len--)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *name++;
    return sum;
}

LONG GetLongName(struct FSSuper *sb, struct DirCache *dc, struct DirEntry *de, ULONG entry, STRPTR dest, UBYTE *dlen) {
    static UBYTE buff[FAT_MAX_LONG_FILENAME];
    UBYTE *p = buff;
    LONG err = ERROR_OBJECT_NOT_FOUND;
    ULONG order=1;
    UBYTE checksum = GetShortNameChecksum(de);

    memset(buff, '\0', sizeof(buff));

    if (entry > 0) {
        ULONG i = entry - 1;

        while (i >= 0) {
            struct LongDirEntry *lde;

            if ((err = GetDirCacheEntry(sb, dc, i, &de)) == 0) {
                ULONG j;

                lde = (struct LongDirEntry *)de;
                if ((lde->attr & ATTR_LONG_NAME_MASK) != ATTR_LONG_NAME ||
                    lde->checksum != checksum ||
                    (lde->order & (0x40 - 1)) != order) {

                    err = ERROR_OBJECT_NOT_FOUND;
                    break;
                }

                for (j=0; j < sizeof(lde->name1); j+=2)
                    *p++ = lde->name1[j];
                for (j=0; j < sizeof(lde->name2); j+=2)
                    *p++ = lde->name2[j];
                for (j=0; j < sizeof(lde->name3); j+=2)
                    *p++ = lde->name3[j];

                if (lde->order & 0x40) {
                    err = 0;

                    memcpy(dest, buff, 106);
                    dest[106] = '\0';

                    *dlen = strlen(dest);

                    kprintf("\tLong file name found: %s len %ld\n", (LONG)buff, *dlen);

                    break;
                }

                i--;
                order++;
            }
            else
                break;
        }
    }

    return err;
}
