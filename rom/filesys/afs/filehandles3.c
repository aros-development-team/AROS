/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <stddef.h>
#include "os.h"
#include "filehandles3.h"
#include "afsblocks.h"
#include "cache.h"
#include "hashing.h"
#include "baseredef.h"

LONG sameLock(struct AfsHandle *ah1, struct AfsHandle *ah2) {

        return (ah1->header_block == ah2->header_block) ? LOCK_SAME : LOCK_DIFFERENT;
}

static const ULONG sizes[]=
{
    0,
    offsetof(struct ExAllData,ed_Type),
    offsetof(struct ExAllData,ed_Size),
    offsetof(struct ExAllData,ed_Prot),
    offsetof(struct ExAllData,ed_Days),
    offsetof(struct ExAllData,ed_Comment),
    offsetof(struct ExAllData,ed_OwnerUID),
    offsetof(struct ExAllData,ed_Size64)    /* ED_OWNER: up to ed_OwnerGID */
};

ULONG examineEAD
        (
                struct AFSBase *afsbase,
                struct Volume *volume,
                struct ExAllData *ead,
                struct BlockCache *entryblock,
                ULONG size,
                ULONG mode
        )
{
STRPTR next,end;
UBYTE *name;
ULONG owner;
ULONG *tail;

        next = (STRPTR)ead+sizes[mode];
        end = (STRPTR)ead+size;

        if (next > end) /* > is correct. Not >= */
            return ERROR_BUFFER_OVERFLOW;
            
        tail = BLK_TAILPTR(entryblock->buffer, volume);
        switch (mode)
        {
        case ED_OWNER :
                owner = OS_BE2LONG(tail[BLKO_OWNER]);
                ead->ed_OwnerUID = owner>>16;
                ead->ed_OwnerGID = owner & 0xFFFF;
        case ED_COMMENT :
                if (OS_BE2LONG(tail[BLKO_SECONDARY_TYPE]) != ST_ROOT)
                {
                        name = (UBYTE *)&tail[BLKO_COMMENT_START];
                        if (name[0] > MAX_COMMENT_LENGTH)
                                return ERROR_DISK_NOT_VALIDATED;
                        if ((next+name[0]+1) > end)
                                return ERROR_BUFFER_OVERFLOW;
                        ead->ed_Comment = next;
                        CopyMem(name+1, ead->ed_Comment, name[0]);
                        ead->ed_Comment[(ULONG)name[0]] = 0;
                        next += name[0]+1; /* NULL-Byte */
                }
                else
                        ead->ed_Comment = 0;
        case ED_DATE :
                ead->ed_Days = OS_BE2LONG(tail[BLKO_DAYS]);
                ead->ed_Mins = OS_BE2LONG(tail[BLKO_MINS]);
                ead->ed_Ticks = OS_BE2LONG(tail[BLKO_TICKS]);
        case ED_PROTECTION :
                ead->ed_Prot = OS_BE2LONG(tail[BLKO_PROTECT]);
        case ED_SIZE :
                ead->ed_Size = OS_BE2LONG(tail[BLKO_BYTE_SIZE]);
        case ED_TYPE :
                ead->ed_Type = OS_BE2LONG(tail[BLKO_SECONDARY_TYPE]);
        case ED_NAME :
                name = (UBYTE *)&tail[BLKO_FILENAME_START];
                if (name[0] > MAX_NAME_LENGTH)
                        return ERROR_DISK_NOT_VALIDATED;
                if ((next+name[0]+1) > end)
                        return ERROR_BUFFER_OVERFLOW;
                ead->ed_Name = next;
                CopyMem(name+1, ead->ed_Name, name[0]);
                ead->ed_Name[(ULONG)name[0]] = 0;
                next += name[0]+1; /* NULL-Byte */
        case 0 :
                ead->ed_Next = (struct ExAllData *)(((IPTR)next + OS_PTRALIGN - 1) & ~(OS_PTRALIGN - 1));
        }
        return 0;
}

ULONG examine
        (
                struct AFSBase *afsbase,
                struct AfsHandle *ah,
                struct ExAllData *ead,
                ULONG size,
                ULONG mode,
                IPTR *dirpos
        )
{
struct BlockCache *entryblock;

        D(bug("[afs] examine(%d,ead,%d,%d)\n",ah->header_block,size,mode));
        if (mode > ED_OWNER)
                return ERROR_BAD_NUMBER;
        entryblock = getBlock(afsbase, ah->volume, ah->header_block);
        if (entryblock == NULL)
                return ERROR_UNKNOWN;
        {
                ULONG error = examineEAD(afsbase, ah->volume, ead, entryblock, size, mode);
                if (error != 0)
                        return error;
        }
        *dirpos = ah->header_block;
        return 0;
}

ULONG getNextExamineBlock
        (struct AFSBase *afsbase, struct AfsHandle *ah, ULONG *key, ULONG *pos)
{
struct Volume *volume = ah->volume;
struct BlockCache *entryblock;
UBYTE cstr[34];
UBYTE *string;

        entryblock = getBlock(afsbase, volume, *key);
        if (entryblock == NULL)
                return ERROR_UNKNOWN;
        if (*key == ah->header_block) /* start examining entries in ah */
        {
                *pos = BLK_TABLE_START;
        }
        else
        {
                if      (entryblock->buffer[BLK_HASHCHAIN(volume)] != 0)
                {
                        /* chained entry */
                        *pos = BLK_HASHCHAIN(volume);
                }
                else
                {
                        string = (UBYTE *)entryblock->buffer+(BLK_FILENAME_START(volume)*4);
                        if (string[0] > MAX_NAME_LENGTH || string[0] >= sizeof(cstr))
                                return ERROR_DISK_NOT_VALIDATED;
                        CopyMem(string+1, cstr, string[0]);
                        cstr[(ULONG)string[0]] = 0;
                        *pos = BLK_TABLE_START+getHashKey(cstr, volume->SizeBlock-56, volume->dosflags)+1;
                        if (*pos > BLK_TABLE_END(volume))
                                return ERROR_NO_MORE_ENTRIES;
                        entryblock = getBlock(afsbase, volume, ah->header_block);
                        if (entryblock == NULL)
                                return ERROR_UNKNOWN;
                }
        }
        /* in one case entryblock->buffer[i] points to a block
                which should be examined next
                in the other cases it may point to NULL */
        while (entryblock->buffer[*pos] == 0)
        {
                if (*pos == BLK_TABLE_END(volume))
                        return ERROR_NO_MORE_ENTRIES;
                *pos += 1;
        }
        /* now i is on a valid position */
        *key = OS_BE2LONG(entryblock->buffer[*pos]);
        return 0;
}


ULONG examineNext
        (struct AFSBase *afsbase, struct AfsHandle *ah, struct FileInfoBlock *fib)
{
        struct Volume *volume = ah->volume;
        struct BlockCache *entryblock;
        UBYTE *string;
        ULONG *tail;
        ULONG filelistentries,datablocksize,datablocks;
        ULONG owner, commentlength;
        ULONG error,filekey;
        ULONG dirkey = fib->fib_DiskKey; /* fib_DiskKey is an IPTR, so we need this conversion */

        D(bug("[afs] examineNext(%d,fib)\n", ah->header_block));
        D(bug("[afs] examineNext: diskey=%d\n", dirkey));

        error = getNextExamineBlock(afsbase, ah, &dirkey, &filekey);
        fib->fib_DiskKey = dirkey;
        if (error != 0)
                return error;
        /* examine the block */
        entryblock = getBlock(afsbase, volume, dirkey);
        if (entryblock == NULL)
                return ERROR_UNKNOWN;
        tail = BLK_TAILPTR(entryblock->buffer, volume);
        string = (UBYTE *)&tail[BLKO_FILENAME_START];
        if (string[0] > MAX_NAME_LENGTH)
                return ERROR_DISK_NOT_VALIDATED;
        string = (UBYTE *)&tail[BLKO_COMMENT_START];
        if (OS_BE2LONG(tail[BLKO_SECONDARY_TYPE]) != ST_ROOT
                && string[0] > MAX_COMMENT_LENGTH)
                return ERROR_DISK_NOT_VALIDATED;
        fib->fib_DirEntryType =
                OS_BE2LONG(tail[BLKO_SECONDARY_TYPE]);
        string = (UBYTE *)&tail[BLKO_FILENAME_START];
        CopyMem(string, fib->fib_FileName, string[0]+1);
        fib->fib_Protection = OS_BE2LONG(tail[BLKO_PROTECT]);
        fib->fib_EntryType = fib->fib_DirEntryType;
        fib->fib_Size = OS_BE2LONG(tail[BLKO_BYTE_SIZE]);
        filelistentries = volume->SizeBlock-56;
        datablocksize = BLOCK_SIZE(volume);
        if (volume->dosflags == 0)
                datablocksize = datablocksize-24;
        datablocks = ((fib->fib_Size+datablocksize-1)/datablocksize);
        fib->fib_NumBlocks =
                datablocks+((datablocks+filelistentries-1)/filelistentries);
        fib->fib_Date.ds_Days = OS_BE2LONG(tail[BLKO_DAYS]);
        fib->fib_Date.ds_Minute = OS_BE2LONG(tail[BLKO_MINS]);
        fib->fib_Date.ds_Tick = OS_BE2LONG(tail[BLKO_TICKS]);
        if (fib->fib_DirEntryType != ST_ROOT)
        {
                string = (UBYTE *)&tail[BLKO_COMMENT_START];
                commentlength = string[0];
                if (commentlength >= sizeof(fib->fib_Comment))
                        commentlength = sizeof(fib->fib_Comment) - 1;
                fib->fib_Comment[0] = commentlength;
                CopyMem(string + 1, fib->fib_Comment + 1, commentlength);
        }
        owner = OS_BE2LONG(tail[BLKO_OWNER]);
        fib->fib_OwnerUID = owner>>16;
        fib->fib_OwnerGID = owner & 0xFFFF;
        return 0;
}

