/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 1
#endif

#include <proto/dos.h>

#include <dos/filesystem.h>
#include <intuition/intuitionbase.h>

#include <aros/macros.h>
#include <aros/debug.h>

#include "afsblocks.h"
#include "afshandler.h"
#include "blockaccess.h"
#include "bitmap.h"
#include "checksums.h"
#include "error.h"
#include "extstrings.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "filehandles3.h"
#include "volumes.h"

#include "baseredef.h"

/*global errors (dos error code ERROR_xyz) */
ULONG error=0;

/*******************************************
 Name  : getDiskInfo
 Descr.: fills a InfoData structure with some
         Disk info;
         answer on ACTION_DISK_INFO
 Input : id - InfoData structure to fill
 Output: 0 = no error
********************************************/
LONG getDiskInfo(struct Volume *volume, struct InfoData *id) {

	id->id_NumSoftErrors=0;
	id->id_UnitNumber=volume->unit;
	id->id_DiskState=volume->usedblockscount ? ID_VALIDATED : ID_VALIDATING;
	id->id_NumBlocks=volume->rootblock*2-volume->bootblocks;
	id->id_NumBlocksUsed=volume->usedblockscount;
	id->id_BytesPerBlock=volume->flags==0 ? BLOCK_SIZE(volume)-24 : BLOCK_SIZE(volume);
	id->id_DiskType=volume->dostype | (volume->flags & 0xFF);
	id->id_VolumeNode=0;		/* I think this is useless in AROS */
	id->id_InUse=(LONG)TRUE;	/* if we are here the device should be in use! */
	return 0;
}

/*******************************************
 Name  : inhibit
 Descr.: forbid/permit access for this volume
 Input : afsbase -
         volume  - the volume
         forbid  - DOSTRUE to forbid
                   DOSFALSE to permit access
 Output: 0 = no error
********************************************/
LONG inhibit(struct afsbase *afsbase, struct Volume *volume, ULONG forbid) {

	if (forbid)
	{
#warning inhibit: no nest checking!!!
/*		if (exclusiveLocks(&volume->locklist)) return DOSFALSE; */
		flush(afsbase, volume);
		remDosVolume(afsbase, volume);
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
 Input : afsbase -
         volume  - the volume
 Output: -
********************************************/
void markBitmaps(struct afsbase *afsbase, struct Volume *volume) {
struct BlockCache *blockbuffer;
ULONG i,curblock;

	for (i=0;(i<=24) && (volume->bitmapblockpointers[i]);i++)
		markBlock(afsbase, volume, volume->bitmapblockpointers[i], 0);
	curblock=volume->bitmapextensionblock;
	while (curblock)
	{
		blockbuffer=getBlock(afsbase, volume, curblock);
		if (!blockbuffer)
			return;
		markBlock(afsbase, volume, curblock, 0);
		for (i=0;i<volume->SizeBlock-1;i++)
		{
			if (!blockbuffer->buffer[i])
				break;
			markBlock(afsbase, volume, AROS_BE2LONG(blockbuffer->buffer[i]), 0);
		}
		curblock=AROS_BE2LONG(blockbuffer->buffer[volume->SizeBlock-1]);
	}
}

/*******************************************
 Name  : format
 Descr.: initialize a volume
 Input : afsbase -
         volume  - volume to initialize
         name    - name of volume
         dostype - DOS\0/...
 Output: 0 for success; error code otherwise
********************************************/
LONG format
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		char *name, ULONG dostype
	)
{
struct BlockCache *blockbuffer;
struct DateStamp ds;
UWORD i;

	error=0;
	if ((blockbuffer=getFreeCacheBlock(afsbase, volume,0)))
	{
		blockbuffer->buffer[0]=AROS_LONG2BE(dostype);
		blockbuffer->buffer[2]=AROS_LONG2BE(volume->rootblock);
		writeBlock(afsbase, volume,blockbuffer);
		if ((blockbuffer=getFreeCacheBlock(afsbase, volume,volume->rootblock)))
		{
			blockbuffer->flags |= BCF_USED;
			DateStamp(&ds);
			blockbuffer->buffer[BLK_PRIMARY_TYPE]=AROS_LONG2BE(T_SHORT);
			blockbuffer->buffer[1]=0;
			blockbuffer->buffer[2]=0;
			blockbuffer->buffer[BLK_TABLE_SIZE]=AROS_LONG2BE(volume->SizeBlock-56);
			blockbuffer->buffer[4]=0;
			for (i=BLK_TABLE_START;i<=BLK_TABLE_END(volume);i++)
				blockbuffer->buffer[i]=0;
			blockbuffer->buffer[BLK_BITMAP_VALID_FLAG(volume)]=-1;
			createNewBitmapBlocks(afsbase, volume);
			for (i=BLK_BITMAP_POINTERS_START(volume);i<=BLK_BITMAP_POINTERS_END(volume);i++)
			{
				blockbuffer->buffer[i]=AROS_LONG2BE
					(
						volume->bitmapblockpointers[i-BLK_BITMAP_POINTERS_START(volume)]
					);
			}
			blockbuffer->buffer[BLK_BITMAP_EXTENSION(volume)]=AROS_LONG2BE
					(
						volume->bitmapextensionblock
					);
			blockbuffer->buffer[BLK_ROOT_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_ROOT_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_ROOT_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
			StrCpyToBstr
				(
					name,
					(APTR)((ULONG)blockbuffer->buffer+(BLK_DISKNAME_START(volume)*4))
				);
			blockbuffer->buffer[volume->SizeBlock-12]=0;
			blockbuffer->buffer[volume->SizeBlock-11]=0;
			blockbuffer->buffer[BLK_VOLUME_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_VOLUME_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_VOLUME_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
			blockbuffer->buffer[BLK_CREATION_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_CREATION_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_CREATION_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
			blockbuffer->buffer[volume->SizeBlock-4]=0;
			blockbuffer->buffer[volume->SizeBlock-3]=0;
			blockbuffer->buffer[volume->SizeBlock-2]=0;
			blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]=AROS_LONG2BE(ST_ROOT);
			blockbuffer->buffer[BLK_CHECKSUM]=0;
			blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE
				(
					0-calcChkSum(volume->SizeBlock,blockbuffer->buffer)
				);
			writeBlock(afsbase, volume,blockbuffer);
			blockbuffer->flags &= ~BCF_USED;
			invalidBitmap(afsbase, volume);
			markBlock(afsbase, volume, volume->rootblock,0);
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
 Input : afsbase -
         volume  - volume to rename
         name    - new name for volume
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG relabel(struct afsbase *afsbase, struct Volume *volume, char *name) {
struct BlockCache *blockbuffer;
struct DateStamp ds;

	remDosVolume(afsbase, volume);
	if (!(blockbuffer=getBlock(afsbase, volume,volume->rootblock)))
		return DOSFALSE;
	StrCpyToBstr
		(
			name,
			(APTR)((ULONG)blockbuffer->buffer+(BLK_DISKNAME_START(volume)*4))
		);
	DateStamp(&ds);
	blockbuffer->buffer[BLK_ROOT_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_ROOT_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_ROOT_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
	blockbuffer->buffer[BLK_VOLUME_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_VOLUME_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_VOLUME_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
	blockbuffer->buffer[BLK_CHECKSUM]=0;
	blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE
		(
			0-calcChkSum(volume->SizeBlock,blockbuffer->buffer)
		);
	writeBlock(afsbase, volume, blockbuffer);
	/* update devicelist info (name) */
	initDeviceList(afsbase, volume, blockbuffer);
	/* add new volume entry */
	addDosVolume(afsbase, volume);
	return DOSTRUE;
}

/*******************************************
 Name  : work
 Descr.: main loop (get packets and answer (or not))
 Input : proc - our process structure
 Output: -
********************************************/
void work(struct afsbase *afsbase) {
struct IOFileSys *iofs;
struct AfsHandle *afshandle;
LONG retval;

	afsbase->port.mp_SigBit = SIGBREAKB_CTRL_F;
	afsbase->port.mp_Flags = PA_SIGNAL;
	for (;;) {
		while ((iofs=(struct IOFileSys *)GetMsg(&afsbase->port))!=NULL)
		{
			D(bug("afs.handler: got command %ld\n",iofs->IOFS.io_Command));
			error=0;
			afshandle = (struct AfsHandle *)iofs->IOFS.io_Unit;
			switch (iofs->IOFS.io_Command)
			{
/*			case (UWORD)-1 :
				iofs->IOFS.io_Unit = (struct Unit *)(&initVolume
					(
						afsbase,
						iofs->IOFS.io_Device,
						iofs->io_Union.io_OpenDevice.io_DeviceName,
						iofs->io_Union.io_OpenDevice.io_Unit,
						(struct DosEnvec *)iofs->io_Union.io_OpenDevice.io_Environ,
						&iofs->io_DosError
					)->ah);
				PutMsg(&afsbase->rport, &iofs->IOFS.io_Message);
				continue;
			case (UWORD)-2 :
				volume=((struct AfsHandle *)iofs->IOFS.io_Unit)->volume;
				if (volume->locklist)
				{
					error = ERROR_OBJECT_IN_USE;
				}
				else
				{
					uninitVolume(afsbase, volume);
					error=0;
				}
				iofs->io_DosError = error;
				PutMsg(&afsbase->rport, &iofs->IOFS.io_Message);
				continue;*/
			case FSA_SAME_LOCK :
				iofs->io_Union.io_SAME_LOCK.io_Same=sameLock
					(
						iofs->io_Union.io_SAME_LOCK.io_Lock[0],
						iofs->io_Union.io_SAME_LOCK.io_Lock[1]
					);
				break;
			case FSA_IS_FILESYSTEM :
				iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem=TRUE;
				break;
			default:
				if (afshandle->volume->flags & VOLF_DISK_IN)
				{
					switch (iofs->IOFS.io_Command)
					{
					case FSA_OPEN : //locateObject, findupdate, findinput
						iofs->IOFS.io_Unit=(struct Unit *)openf
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_OPEN.io_Filename,
								iofs->io_Union.io_OPEN.io_FileMode
							);
						break;
					case FSA_CLOSE :
						closef
							(
								afsbase,
								afshandle
							);
						break;
					case FSA_READ :
						iofs->io_Union.io_READ.io_Length=read
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_READ.io_Buffer,
								iofs->io_Union.io_READ.io_Length
							);
						break;
					case FSA_WRITE :
						iofs->io_Union.io_WRITE.io_Length=write
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_WRITE.io_Buffer,
								iofs->io_Union.io_WRITE.io_Length
							);
						break;
					case FSA_SEEK :
						iofs->io_Union.io_SEEK.io_Offset=seek
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_SEEK.io_Offset,
								iofs->io_Union.io_SEEK.io_SeekMode
							);
						break;
					case FSA_SET_FILE_SIZE :
						D(bug("afs.handler: set file size nsy\n"));
						error=ERROR_ACTION_NOT_KNOWN;
						break;
					case FSA_FILE_MODE :
						D(bug("afs.handler: set file mode nsy\n"));
						error=ERROR_ACTION_NOT_KNOWN;
						break;
					case FSA_EXAMINE :
						error=examine
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_EXAMINE.io_ead,
								iofs->io_Union.io_EXAMINE.io_Size,
								iofs->io_Union.io_EXAMINE.io_Mode,
								&iofs->io_DirPos
							);
						break;
					case FSA_EXAMINE_ALL :
						error=examineAll
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_EXAMINE_ALL.io_ead,
								iofs->io_Union.io_EXAMINE_ALL.io_Size,
								iofs->io_Union.io_EXAMINE_ALL.io_Mode
							);
						break;
					case FSA_EXAMINE_NEXT :
						error=examineNext
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_EXAMINE_NEXT.io_fib
							);
						break;
					case FSA_OPEN_FILE :
						iofs->IOFS.io_Unit=(struct Unit *)openfile
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_OPEN_FILE.io_Filename,
								iofs->io_Union.io_OPEN_FILE.io_FileMode,
								iofs->io_Union.io_OPEN_FILE.io_Protection
							);
						break;
					case FSA_CREATE_DIR :
						iofs->IOFS.io_Unit=(struct Unit *)createDir
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_CREATE_DIR.io_Filename,
								iofs->io_Union.io_CREATE_DIR.io_Protection
							);
						break;
					case FSA_CREATE_HARDLINK :
						D(bug("afs.handler: create hardlinks nsy\n"));
						iofs->IOFS.io_Unit=0;
						error=ERROR_ACTION_NOT_KNOWN;
						break;
					case FSA_CREATE_SOFTLINK :
						D(bug("afs.handler: create softlinks nsy\n"));
						iofs->IOFS.io_Unit=0;
						error=ERROR_ACTION_NOT_KNOWN;
						break;
					case FSA_READ_SOFTLINK :
						D(bug("afs.handler: read softlinks nsy\n"));
						error=ERROR_ACTION_NOT_KNOWN;
						break;
					case FSA_RENAME :
						error=rename
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_RENAME.io_Filename,
								iofs->io_Union.io_RENAME.io_NewName
							);
						break;
					case FSA_DELETE_OBJECT :
						error=deleteObject
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_DELETE_OBJECT.io_Filename
							);
						break;
					case FSA_SET_COMMENT :
						error=setComment
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_SET_COMMENT.io_Filename,
								iofs->io_Union.io_SET_COMMENT.io_Comment
							);
						break;
					case FSA_SET_PROTECT :
						error=setProtect
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_SET_PROTECT.io_Filename,
								iofs->io_Union.io_SET_PROTECT.io_Protection
							);
						break;
					case FSA_SET_OWNER :
						D(bug("afs.handler: set owner nsy\n"));
						error=ERROR_ACTION_NOT_KNOWN;
						break;
					case FSA_SET_DATE :
						error=setDate
							(
								afsbase,
								afshandle,
								iofs->io_Union.io_SET_DATE.io_Filename,
								&iofs->io_Union.io_SET_DATE.io_Date
							);
						break;
/* morecache */
					case FSA_INHIBIT :
						error=inhibit
							(
								afsbase,
								afshandle->volume,
								iofs->io_Union.io_INHIBIT.io_Inhibit
							);
						break;
					case FSA_FORMAT :
						error=format
							(
								afsbase,
								afshandle->volume,
								iofs->io_Union.io_FORMAT.io_VolumeName,
								iofs->io_Union.io_FORMAT.io_DosType
							);
						break;
					case FSA_RELABEL :
						iofs->io_Union.io_RELABEL.io_Result=relabel
							(
								afsbase,
								afshandle->volume,
								iofs->io_Union.io_RELABEL.io_NewName
							);
							break;
					case FSA_DISK_INFO :
						error=getDiskInfo
							(
								afshandle->volume,
								iofs->io_Union.io_INFO.io_Info
							);
						break;
					default :
						D(bug("afs.handler: unknown fsa %d\n", iofs->IOFS.io_Command));
						retval=DOSFALSE;
						error=ERROR_ACTION_NOT_KNOWN;
					}
				}
				else
				{
					switch (iofs->IOFS.io_Command)
					{
					case FSA_OPEN : //locateObject, findupdate, findinput
					case FSA_CLOSE :
					case FSA_READ :
					case FSA_WRITE :
					case FSA_SEEK :
					case FSA_SET_FILE_SIZE :
					case FSA_FILE_MODE :
					case FSA_EXAMINE :
					case FSA_EXAMINE_ALL :
					case FSA_EXAMINE_NEXT :
					case FSA_OPEN_FILE :
					case FSA_CREATE_DIR :
					case FSA_CREATE_HARDLINK :
					case FSA_CREATE_SOFTLINK :
					case FSA_READ_SOFTLINK :
					case FSA_RENAME :
					case FSA_DELETE_OBJECT :
					case FSA_SET_COMMENT :
					case FSA_SET_PROTECT :
					case FSA_SET_OWNER :
					case FSA_SET_DATE :
					case FSA_INHIBIT :
					case FSA_FORMAT :
					case FSA_RELABEL :
					case FSA_DISK_INFO :
						retval= DOSFALSE;
						error = ERROR_NO_DISK;
						break;
					default :
						D(bug("afs.handler: unknown fsa %d\n", iofs->IOFS.io_Command));
						retval= DOSFALSE;
						error = ERROR_ACTION_NOT_KNOWN;
					}
				}
			}
			checkCache(afsbase, afshandle->volume->blockcache);
			iofs->io_DosError=error;
			ReplyMsg(&iofs->IOFS.io_Message);
		}
		checkDeviceFlags(afsbase);
		Wait(1<<afsbase->port.mp_SigBit);
	}
}
