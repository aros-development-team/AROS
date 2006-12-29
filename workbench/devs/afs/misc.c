/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 1
#endif

#include "os.h"
#include "misc.h"
#include "afsblocks.h"
#include "bitmap.h"
#include "cache.h"
#include "checksums.h"
#include "extstrings.h"
#include "baseredef.h"

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

	/* store current time as modification date for this object and all its ancestors */
	DateStamp(&ds);
	for (;;)
	{
		blockbuffer->buffer[BLK_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
		blockbuffer->buffer[BLK_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
		blockbuffer->buffer[BLK_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
		if (blockbuffer->buffer[BLK_PARENT(volume)] == 0)
			break;
		writeBlockDeferred(afsbase, volume, blockbuffer, BLK_CHECKSUM);
		blockbuffer = getBlock
			(afsbase, volume, OS_BE2LONG(blockbuffer->buffer[BLK_PARENT(volume)]));
		if (blockbuffer == NULL)
			return ERROR_UNKNOWN;
	}
	/* last block is not written yet - it's the rootblock */
	/* we have to change BLK_VOLUME_xxx */
	blockbuffer->buffer[BLK_VOLUME_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_VOLUME_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_VOLUME_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
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
	id->id_DiskState = volume->usedblockscount ? ID_VALIDATED : ID_VALIDATING;
	id->id_NumBlocks = volume->countblocks-volume->bootblocks;
	id->id_NumBlocksUsed = volume->usedblockscount;
	id->id_BytesPerBlock = volume->dosflags==0 ? BLOCK_SIZE(volume)-24 : BLOCK_SIZE(volume);
	id->id_DiskType = volume->dostype | volume->dosflags;
	id->id_VolumeNode = 0; /* I think this is useless in AROS */
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

	if (forbid)
	{
#warning inhibit: no nest checking!!!
/*		if (exclusiveLocks(&volume->locklist)) return DOSFALSE; */
		flush(afsbase, volume);
		osMediumFree(afsbase, volume, FALSE);
	}
	else
	{
		newMedium(afsbase, volume);
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
void markBitmaps(struct AFSBase *afsbase, struct Volume *volume) {
struct BlockCache *blockbuffer;
ULONG i,curblock;

	for (i=0; (i<=24) && (volume->bitmapblockpointers[i] != 0); i++)
		markBlock(afsbase, volume, volume->bitmapblockpointers[i], 0);
	curblock = volume->bitmapextensionblock;
	while (curblock != 0)
	{
		blockbuffer = getBlock(afsbase, volume, curblock);
		if (blockbuffer == NULL)
			return;
		markBlock(afsbase, volume, curblock, 0);
		for (i=0; i<volume->SizeBlock-1; i++)
		{
			if (blockbuffer->buffer[i] == 0)
				break;
			markBlock(afsbase, volume, OS_BE2LONG(blockbuffer->buffer[i]), 0);
		}
		curblock = OS_BE2LONG(blockbuffer->buffer[volume->SizeBlock-1]);
	}
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
	(struct AFSBase *afsbase, struct Volume *volume, STRPTR name, ULONG dostype)
{
struct BlockCache *blockbuffer;
struct DateStamp ds;
UWORD i;

	blockbuffer = getFreeCacheBlock(afsbase, volume, 0);
	if (blockbuffer != NULL)
	{
		blockbuffer->buffer[0] = OS_LONG2BE(dostype);
		blockbuffer->buffer[2] = OS_LONG2BE(volume->rootblock);
		writeBlock(afsbase, volume, blockbuffer, -1);
		blockbuffer = getFreeCacheBlock(afsbase, volume, volume->rootblock);
		if (blockbuffer != NULL)
		{
			blockbuffer->flags |= BCF_USED;
			DateStamp(&ds);
			blockbuffer->buffer[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_SHORT);
			blockbuffer->buffer[1] = 0;
			blockbuffer->buffer[2] = 0;
			blockbuffer->buffer[BLK_TABLE_SIZE] = OS_LONG2BE(volume->SizeBlock-56);
			blockbuffer->buffer[4] = 0;
			for (i=BLK_TABLE_START; i<=BLK_TABLE_END(volume); i++)
				blockbuffer->buffer[i] = 0;
			blockbuffer->buffer[BLK_BITMAP_VALID_FLAG(volume)] = -1;
			createNewBitmapBlocks(afsbase, volume);
			for (
					i=BLK_BITMAP_POINTERS_START(volume);
					i<=BLK_BITMAP_POINTERS_END(volume);
					i++
				)
			{
				blockbuffer->buffer[i] = OS_LONG2BE
					(
						volume->bitmapblockpointers[i-BLK_BITMAP_POINTERS_START(volume)]
					);
			}
			blockbuffer->buffer[BLK_BITMAP_EXTENSION(volume)] =
				OS_LONG2BE(volume->bitmapextensionblock);
			blockbuffer->buffer[BLK_ROOT_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_ROOT_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_ROOT_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
			StrCpyToBstr
				(
					name,
					(APTR)((char *)blockbuffer->buffer+(BLK_DISKNAME_START(volume)*4)),
					MAX_NAME_LENGTH
				);
			blockbuffer->buffer[volume->SizeBlock-12] = 0;
			blockbuffer->buffer[volume->SizeBlock-11] = 0;
			blockbuffer->buffer[BLK_VOLUME_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_VOLUME_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_VOLUME_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
			blockbuffer->buffer[BLK_CREATION_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_CREATION_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_CREATION_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
			blockbuffer->buffer[volume->SizeBlock-4] = 0;
			blockbuffer->buffer[volume->SizeBlock-3] = 0;
			blockbuffer->buffer[volume->SizeBlock-2] = 0;
			blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)] = OS_LONG2BE(ST_ROOT);
			writeBlock(afsbase, volume, blockbuffer, BLK_CHECKSUM);
			blockbuffer->flags &= ~BCF_USED;
			invalidBitmap(afsbase, volume);
			markBlock(afsbase, volume, volume->rootblock, 0);
			markBitmaps(afsbase, volume);
			validBitmap(afsbase, volume);
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
LONG relabel(struct AFSBase *afsbase, struct Volume *volume, STRPTR name) {
struct BlockCache *blockbuffer;
struct DateStamp ds;

	osMediumFree(afsbase, volume, FALSE);
	blockbuffer=getBlock(afsbase, volume,volume->rootblock);
	if (blockbuffer == NULL)
		return DOSFALSE;
	StrCpyToBstr
		(
			name,
			(APTR)((char *)blockbuffer->buffer+(BLK_DISKNAME_START(volume)*4)),
			MAX_NAME_LENGTH
		);
	DateStamp(&ds);
	blockbuffer->buffer[BLK_ROOT_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_ROOT_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_ROOT_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
	blockbuffer->buffer[BLK_VOLUME_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_VOLUME_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_VOLUME_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
	writeBlock(afsbase, volume, blockbuffer, BLK_CHECKSUM);
	/* update os specific information of the medium */
	osMediumInit(afsbase, volume, blockbuffer);
	return DOSTRUE;
}

