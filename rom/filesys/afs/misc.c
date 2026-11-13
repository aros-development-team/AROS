/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      added disk validation
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected tabulation
 */


#ifndef DEBUG
#define DEBUG 0
#endif

#include "os.h"
#include "misc.h"
#include "afsblocks.h"
#include "bitmap.h"
#include "cache.h"
#include "checksums.h"
#include "extstrings.h"
#include "baseredef.h"
#include "validator.h"

/* store a DateStamp into three consecutive (days, mins, ticks) block longs */
void putStamp(ULONG *dest, const struct DateStamp *ds)
{
        dest[0] = OS_LONG2BE(ds->ds_Days);
        dest[1] = OS_LONG2BE(ds->ds_Minute);
        dest[2] = OS_LONG2BE(ds->ds_Tick);
}

/**********************************************
 Name  : writeHeader
 Descr.: update header information (time)
 Input : volume      -
         blockbuffer - pointer to struct BlockCache
                       containing the headerblock
 Output: 0 for success; error code otherwise
***********************************************/
ULONG writeHeader
        (
                struct AFSBase *afsbase,
                struct Volume *volume,
                struct BlockCache *blockbuffer
        )
{
struct DateStamp ds;
ULONG *tail;

        /* store current time as modification date for this object and all its ancestors */
        DateStamp(&ds);
        for (;;)
        {
                tail = BLK_TAILPTR(blockbuffer->buffer, volume);
                putStamp(&tail[BLKO_DAYS], &ds);
                if (tail[BLKO_PARENT] == 0)
                        break;
                writeBlockDeferred(afsbase, volume, blockbuffer, BLK_CHECKSUM);
                blockbuffer = getBlock
                        (afsbase, volume, OS_BE2LONG(tail[BLKO_PARENT]));
                if (blockbuffer == NULL)
                        return ERROR_UNKNOWN;
        }
        /* last block is not written yet - it's the rootblock */
        /* we have to change BLK_VOLUME_xxx */
        putStamp(&tail[BLKO_VOLUME_DAYS], &ds);
        writeBlockDeferred(afsbase, volume, blockbuffer, BLK_CHECKSUM);
        return 0;
}

/*******************************************
 Name  : getDiskInfo
 Descr.: fills a InfoData structure with some
         Disk info;
         answer on ACTION_DISK_INFO
 Input : id - InfoData structure to fill
 Output: 0 = no error
********************************************/
LONG getDiskInfo(struct Volume *volume, struct InfoData *id) {

        id->id_NumSoftErrors = 0;
        id->id_UnitNumber = volume->unit;
        id->id_DiskState = volume->state;
        id->id_NumBlocks = volume->countblocks-volume->bootblocks;
        id->id_NumBlocksUsed = volume->usedblockscount;
        id->id_BytesPerBlock = volume->dosflags==0 ? BLOCK_SIZE(volume)-24 : BLOCK_SIZE(volume);
        id->id_DiskType = mediumPresent(&volume->ioh) ? (volume->dostype | volume->dosflags) : ID_NO_DISK_PRESENT;
        id->id_VolumeNode = MKBADDR(volume->volumenode);
        id->id_InUse = (LONG)TRUE; /* if we are here the device should be in use! */
        return 0;
}

/*******************************************
 Name  : inhibit
 Descr.: forbid/permit access for this volume
 Input : volume  - the volume
         forbid  - DOSTRUE to forbid
                   DOSFALSE to permit access
 Output: 0 = no error
********************************************/
LONG inhibit(struct AFSBase *afsbase, struct Volume *volume, ULONG forbid) {
        D(bug("[afs 0x%p] inhibit(%d)\n", volume, forbid));
        if (forbid)
        {
                if (volume->inhibitcounter++ == 0)
                {
                        D(bug("[afs 0x%p] inhibiting\n", volume));
/*              if (exclusiveLocks(&volume->locklist)) return DOSFALSE; */
                        if (mediumPresent(&volume->ioh))
                        {
                                if (!flushForRelease(afsbase, volume))
                                {
                                        volume->inhibitcounter--;
                                        return ERROR_UNKNOWN;
                                }
                                osMediumFree(afsbase, volume, FALSE);
                                clearCleanCache(volume);
                        }
                }
        }
        else if (volume->inhibitcounter)
        {
                if (--volume->inhibitcounter == 0)
                {
                        D(bug("[afs 0x%p] uninhibiting\n", volume));
                        if (diskPresent(afsbase, &volume->ioh))
                        {
                                D(bug("[afs 0x%p] media inserted\n", volume));
                                volume->ioh.ioflags |= IOHF_DISK_IN;
                                {
                                        LONG error = newMedium(afsbase, volume);
                                        /* NDOS and unreadable media are still present.
                                         * Only pending writes prevent us from accepting
                                         * this medium after a failed remount. */
                                        if (error != 0 && volume->cachepending)
                                        {
                                                volume->ioh.ioflags &= ~IOHF_DISK_IN;
                                                return error;
                                        }
                                }
                        }
                        else
                                volume->ioh.ioflags &= ~IOHF_DISK_IN;
                }
        }
        return 0;
}

/*******************************************
 Name  : markBitmaps
 Descr.: mark newly allocated bitmapblocks
         (for format)
 Input : volume  - the volume
 Output: -
********************************************/
BOOL markBitmaps(struct AFSBase *afsbase, struct Volume *volume) {
struct BlockCache *blockbuffer;
ULONG i,curblock;

        for (i=0; (i<=24) && (volume->bitmapblockpointers[i] != 0); i++)
                if (!markBlock(afsbase, volume, volume->bitmapblockpointers[i], 0))
                        return DOSFALSE;
        curblock = volume->bitmapextensionblock;
        while (curblock != 0)
        {
                blockbuffer = getBlock(afsbase, volume, curblock);
                if (blockbuffer == NULL)
                        return DOSFALSE;
                blockbuffer->flags |= BCF_USED;
                if (!markBlock(afsbase, volume, curblock, 0))
                        goto failed;
                for (i=0; i<volume->SizeBlock-1; i++)
                {
                        if (blockbuffer->buffer[i] == 0)
                                break;
                        if (!markBlock(afsbase, volume, OS_BE2LONG(blockbuffer->buffer[i]), 0))
                                goto failed;
                }
                curblock = OS_BE2LONG(blockbuffer->buffer[volume->SizeBlock-1]);
                blockbuffer->flags &= ~BCF_USED;
        }
        return DOSTRUE;
failed:
        blockbuffer->flags &= ~BCF_USED;
        return DOSFALSE;
}

/*******************************************
 Name  : format
 Descr.: initialize a volume
 Input : volume  - volume to initialize
         name    - name of volume
         dostype - DOS\0/...
 Output: 0 for success; error code otherwise
********************************************/
LONG format
        (struct AFSBase *afsbase, struct Volume *volume, CONST_STRPTR name,
        ULONG dostype)
{
struct BlockCache *blockbuffer;
struct DateStamp ds;
ULONG *root, *tail;
UWORD i;

        blockbuffer = getFreeCacheBlock(afsbase, volume, 0);
        if (blockbuffer != NULL)
        {
                blockbuffer->buffer[0] = OS_LONG2BE(dostype);
                blockbuffer->buffer[2] = OS_LONG2BE(volume->rootblock);
                if (!writeBlock(afsbase, volume, blockbuffer, -1))
                        return ERROR_UNKNOWN;
                blockbuffer = getFreeCacheBlock(afsbase, volume, volume->rootblock);
                if (blockbuffer != NULL)
                {
                        blockbuffer->flags |= BCF_USED;
                        DateStamp(&ds);
                        root = blockbuffer->buffer;
                        tail = BLK_TAILPTR(root, volume);
                        root[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_SHORT);
                        root[1] = 0;
                        root[2] = 0;
                        root[BLK_TABLE_SIZE] = OS_LONG2BE(volume->SizeBlock-56);
                        root[4] = 0;
                        for (i=BLK_TABLE_START; i<=BLK_TABLE_END(volume); i++)
                                root[i] = 0;
                        tail[BLKO_BITMAP_VALID_FLAG] = -1;
                        if (!createNewBitmapBlocks(afsbase, volume))
                        {
                                blockbuffer->flags &= ~BCF_USED;
                                return ERROR_UNKNOWN;
                        }
                        for (i=0; i<=BLKO_BITMAP_POINTERS_END-BLKO_BITMAP_POINTERS_START; i++)
                        {
                                tail[BLKO_BITMAP_POINTERS_START+i] = OS_LONG2BE
                                        (
                                                volume->bitmapblockpointers[i]
                                        );
                        }
                        tail[BLKO_BITMAP_EXTENSION] =
                                OS_LONG2BE(volume->bitmapextensionblock);
                        putStamp(&tail[BLKO_ROOT_DAYS], &ds);
                        StrCpyToBstr
                                (
                                        name,
                                        (APTR)&tail[BLKO_DISKNAME_START],
                                        MAX_NAME_LENGTH
                                );
                        tail[-12] = 0;
                        tail[-11] = 0;
                        putStamp(&tail[BLKO_VOLUME_DAYS], &ds);
                        putStamp(&tail[BLKO_CREATION_DAYS], &ds);
                        tail[-4] = 0;
                        tail[-3] = 0;
                        tail[-2] = 0;
                        tail[BLKO_SECONDARY_TYPE] = OS_LONG2BE(ST_ROOT);
                        if (!writeBlock(afsbase, volume, blockbuffer, BLK_CHECKSUM))
                        {
                                blockbuffer->flags &= ~BCF_USED;
                                return ERROR_UNKNOWN;
                        }
                        blockbuffer->flags &= ~BCF_USED;
                        if (!invalidBitmap(afsbase, volume))
                                return ERROR_UNKNOWN;
                        if (!markBlock(afsbase, volume, volume->rootblock, 0)
                                || !markBitmaps(afsbase, volume))
                        {
                                if (volume->bitmapblock != NULL)
                                        volume->bitmapblock->flags &= ~BCF_USED;
                                volume->writefailed = TRUE;
                                volume->state = ID_VALIDATING;
                                setBitmapFlag(afsbase, volume, 0);
                                return ERROR_UNKNOWN;
                        }
                        if (!validBitmap(afsbase, volume))
                                return ERROR_UNKNOWN;
                        return 0;
                }
        }
        return ERROR_UNKNOWN;
}

/*******************************************
 Name  : relabel
 Descr.: rename a volume
 Input : volume  - volume to rename
         name    - new name for volume
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG relabel(struct AFSBase *afsbase, struct Volume *volume,
        CONST_STRPTR name, SIPTR *error) {
struct BlockCache *blockbuffer;
struct DateStamp ds;
ULONG *tail;

        if (0 == checkValid(afsbase, volume))
        {
                *error = ERROR_DISK_WRITE_PROTECTED;
                return DOSFALSE;
        }
        osMediumFree(afsbase, volume, FALSE);
        blockbuffer=getBlock(afsbase, volume,volume->rootblock);
        if (blockbuffer == NULL)
                return DOSFALSE;
    if (strlen(name) > MAX_NAME_LENGTH)
    {
        *error = ERROR_LINE_TOO_LONG;
        return DOSFALSE;
    }
        tail = BLK_TAILPTR(blockbuffer->buffer, volume);
        StrCpyToBstr
                (
                        name,
                        (APTR)&tail[BLKO_DISKNAME_START],
                        MAX_NAME_LENGTH
                );
        DateStamp(&ds);
        putStamp(&tail[BLKO_ROOT_DAYS], &ds);
        putStamp(&tail[BLKO_VOLUME_DAYS], &ds);
        if (!writeBlock(afsbase, volume, blockbuffer, BLK_CHECKSUM))
        {
                *error = ERROR_UNKNOWN;
                return DOSFALSE;
        }
        /* update os specific information of the medium */
        osMediumInit(afsbase, volume, blockbuffer);
        return DOSTRUE;
}

