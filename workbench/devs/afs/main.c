#define DEBUG 1

#include <proto/dos.h>

#include <dos/filesystem.h>
#include <intuition/intuitionbase.h>

#include <aros/machine.h>
#include <aros/macros.h>
#include <aros/debug.h>

#include "afshandler.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "filehandles3.h"
#include "blockaccess.h"
#include "bitmap.h"
#include "checksums.h"
#include "error.h"
#include "afsblocks.h"
#include "volumes.h"
#include "baseredef.h"

ULONG error=0;								//global errors (ERROR_xyz)

/*******************************************
 Name  : getDiskInfo
 Descr.: fills a InfoData structure with some
         Disk info;
         answer on ACTION_DISK_INFO
 Input : id - InfoData structure to fill
 Output: (currently) DOSTRUE
********************************************/
LONG getDiskInfo(struct Volume *volume, struct InfoData *id) {

	id->id_NumSoftErrors=0;
	id->id_UnitNumber=volume->unit;
	id->id_DiskState=volume->usedblockscount ? ID_VALIDATED : ID_VALIDATING;
	id->id_NumBlocks=volume->rootblock*2-volume->bootblocks;
	id->id_NumBlocksUsed=volume->usedblockscount;
	id->id_BytesPerBlock=volume->flags==0 ? BLOCK_SIZE(volume)-24 : BLOCK_SIZE(volume);
	id->id_DiskType=volume->dostype;
	id->id_VolumeNode=0;		/* I think this is useless in AROS */
	id->id_InUse=(LONG)TRUE;	/* if we are here the device should be in use! */
	return 0;
}

ULONG flush(struct afsbase *afsbase, struct Volume *volume) {

	flushCache(volume->blockcache);
	sendDeviceCmd(afsbase, volume, CMD_UPDATE);
	//turn off motor
	return DOSTRUE;
}

LONG inhibit(struct afsbase *afsbase, struct Volume *volume, ULONG forbid) {

	if (forbid) {
#warning inhibit: always ok!!!
//		if (exclusiveLocks(&volume->locklist)) return DOSFALSE;
		flush(afsbase, volume);
		remDosVolume(afsbase, volume);
	}
	else {
		newMedium(afsbase, volume);
	}
	return DOSTRUE;
}

void markBitmaps(struct afsbase *afsbase, struct Volume *volume) {
struct BlockCache *blockbuffer;
ULONG i,curblock;

	for (i=0;i<=24;i++)
		markBlock(afsbase, volume, volume->bitmapblockpointers[i], 0);
	curblock=volume->bitmapextensionblock;
	while (curblock) {
		blockbuffer=getBlock(afsbase, volume, curblock);
		if (!blockbuffer) return;
		for (i=0;i<volume->SizeBlock-1;i++) {
			if (!blockbuffer->buffer[i]) break;
			markBlock(afsbase, volume, AROS_BE2LONG(blockbuffer->buffer[i]), 0);
		}
		curblock=AROS_BE2LONG(blockbuffer->buffer[volume->SizeBlock-1]);
	}
}

void format(struct afsbase *afsbase, struct Volume *volume, char *name, ULONG dostype) {
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
				blockbuffer->buffer[i]=AROS_LONG2BE(volume->bitmapblockpointers[i-BLK_BITMAP_POINTERS_START(volume)]);
			blockbuffer->buffer[BLK_BITMAP_EXTENSION(volume)]=AROS_LONG2BE(volume->bitmapextensionblock);
			blockbuffer->buffer[BLK_ROOT_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
			blockbuffer->buffer[BLK_ROOT_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
			blockbuffer->buffer[BLK_ROOT_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
			CopyMem(name,(APTR)((ULONG)blockbuffer->buffer+(BLK_DISKNAME_START(volume)*4)),name[0]+1);
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
			blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume->SizeBlock,blockbuffer->buffer));
			writeBlock(afsbase, volume,blockbuffer);
			blockbuffer->flags &= ~BCF_USED;
			invalidBitmap(afsbase, volume);
			markBlock(afsbase, volume, volume->rootblock,0);
			markBitmaps(afsbase, volume);
			validBitmap(afsbase, volume);
		}
		error=ERROR_UNKNOWN;
	}
}

LONG relabel(struct afsbase *afsbase, struct Volume *volume, char *name) {
struct BlockCache *blockbuffer;
struct DateStamp ds;

	error=0;
	remDosVolume(afsbase, volume);	//remove dos entry
	if (!(blockbuffer=getBlock(afsbase, volume,volume->rootblock)))
		return DOSFALSE;
	CopyMem(name,(APTR)((ULONG)blockbuffer->buffer+(BLK_DISKNAME_START(volume)*4)),name[0]+1);
	DateStamp(&ds);
	blockbuffer->buffer[BLK_ROOT_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_ROOT_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_ROOT_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
	blockbuffer->buffer[BLK_VOLUME_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_VOLUME_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_VOLUME_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
	blockbuffer->buffer[BLK_CHECKSUM]=0;
	blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume->SizeBlock,blockbuffer->buffer));
	writeBlock(afsbase, volume, blockbuffer);
	initDeviceList(afsbase, volume, blockbuffer);	// update devicelist info (name)
	addDosVolume(afsbase, volume, 0);						// add new volume entry
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
struct Volume *volume;
LONG retval;

	afsbase->port.mp_SigBit = SIGBREAKB_CTRL_F;
	afsbase->port.mp_Flags = PA_SIGNAL;
	for (;;) {
		while ((iofs=(struct IOFileSys *)GetMsg(&afsbase->port))!=NULL)
		{
			D(bug("afs.handler: got command %ld\n",iofs->IOFS.io_Command));
			error=0;
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
				volume=(struct Volume *)iofs->IOFS.io_Unit;
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
			case FSA_OPEN : //locateObject, findupdate, findinput
				iofs->IOFS.io_Unit=(struct Unit *)openf
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_OPEN.io_Filename,
						iofs->io_Union.io_OPEN.io_FileMode
					);
				break;
			case FSA_CLOSE :
				closef
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit
					);
				break;
			case FSA_READ :
				iofs->io_Union.io_READ.io_Length=read
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_READ.io_Buffer,
						iofs->io_Union.io_READ.io_Length
					);
				break;
			case FSA_WRITE :
				iofs->io_Union.io_WRITE.io_Length=write
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_WRITE.io_Buffer,
						iofs->io_Union.io_WRITE.io_Length
					);
				break;
			case FSA_SEEK :
				iofs->io_Union.io_SEEK.io_Offset=seek
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
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
			case FSA_SAME_LOCK :
				iofs->io_Union.io_SAME_LOCK.io_Same=sameLock
					(
						iofs->io_Union.io_SAME_LOCK.io_Lock[0],
						iofs->io_Union.io_SAME_LOCK.io_Lock[1]
					);
				break;
			case FSA_EXAMINE :
				error=examine
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
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
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_EXAMINE_ALL.io_ead,
						iofs->io_Union.io_EXAMINE_ALL.io_Size,
						iofs->io_Union.io_EXAMINE_ALL.io_Mode
					);
				break;
			case FSA_EXAMINE_NEXT :
				error=examineNext
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_EXAMINE_NEXT.io_fib
					);
				break;
			case FSA_OPEN_FILE :
				iofs->IOFS.io_Unit=(struct Unit *)openfile
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_OPEN_FILE.io_Filename,
						iofs->io_Union.io_OPEN_FILE.io_FileMode,
						iofs->io_Union.io_OPEN_FILE.io_Protection
					);
				break;
			case FSA_CREATE_DIR :
				iofs->IOFS.io_Unit=(struct Unit *)createDir
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
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
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_RENAME.io_Filename,
						iofs->io_Union.io_RENAME.io_NewName
					);
				break;
			case FSA_DELETE_OBJECT :
				error=deleteObject
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_DELETE_OBJECT.io_Filename
					);
				break;
			case FSA_SET_COMMENT :
				error=setComment
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_SET_COMMENT.io_Filename,
						iofs->io_Union.io_SET_COMMENT.io_Comment
					);
				break;
			case FSA_SET_PROTECT :
				error=setProtect
					(
						afsbase,
						(struct AfsHandle *)iofs->IOFS.io_Unit,
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
						(struct AfsHandle *)iofs->IOFS.io_Unit,
						iofs->io_Union.io_SET_DATE.io_Filename,
						&iofs->io_Union.io_SET_DATE.io_Date
					);
				break;
			case FSA_IS_FILESYSTEM :
				iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem=TRUE;
				break;
/* morecache */
			case FSA_INHIBIT :
				inhibit
					(
						afsbase,
						((struct AfsHandle *)iofs->IOFS.io_Unit)->volume,
						iofs->io_Union.io_INHIBIT.io_Inhibit
					);
				break;
			case FSA_FORMAT :
				format
					(
						afsbase,
						((struct AfsHandle *)iofs->IOFS.io_Unit)->volume,
						iofs->io_Union.io_FORMAT.io_VolumeName,
						iofs->io_Union.io_FORMAT.io_DosType
					);
				break;
			case FSA_RELABEL :
				iofs->io_Union.io_RELABEL.io_Result=relabel
					(
						afsbase,
						((struct AfsHandle *)iofs->IOFS.io_Unit)->volume,
						iofs->io_Union.io_RELABEL.io_NewName
					);
					break;
			case FSA_DISK_INFO :
				error=getDiskInfo
					(
						((struct AfsHandle *)iofs->IOFS.io_Unit)->volume,
						iofs->io_Union.io_INFO.io_Info
					);
				break;
			default :
				D(bug("afs.handler: unknown fsa %d\n", iofs->IOFS.io_Command));
				retval=DOSFALSE;
				error=ERROR_ACTION_NOT_KNOWN;
			}
			iofs->io_DosError=error;
			ReplyMsg(&iofs->IOFS.io_Message);
		}
		WaitPort(&afsbase->port);
	}
}

