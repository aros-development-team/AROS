/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 1
#endif

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <string.h>

#include "volumes.h"
#include "bitmap.h"
#include "extstrings.h"
#include "error.h"
#include "afsblocks.h"
#include "baseredef.h"

extern void timercode();

/*******************************************
 Name  : initDeviceList
 Descr.: initializes a devicelist structure
 Input : devicelist - devicelist structure to initialize
         rootblock  - cacheblock of the rootblock
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG initDeviceList
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		struct BlockCache *rootblock
	)
{
STRPTR name;
UBYTE i;

	name=(STRPTR)((ULONG)rootblock->buffer+(BLK_DISKNAME_START(volume)*4));
	volume->devicelist.dl_Next=0;
	volume->devicelist.dl_Type=DLT_VOLUME;
	volume->devicelist.dl_Device=volume->device;
	volume->devicelist.dl_Lock=0;
	volume->devicelist.dl_VolumeDate.ds_Days=AROS_BE2LONG
		(
			rootblock->buffer[BLK_ROOT_DAYS(volume)]
		);
	volume->devicelist.dl_VolumeDate.ds_Minute=AROS_BE2LONG
		(
			rootblock->buffer[BLK_ROOT_MINS(volume)]
		);
	volume->devicelist.dl_VolumeDate.ds_Tick=AROS_BE2LONG
		(
			rootblock->buffer[BLK_ROOT_TICKS(volume)]
		);
	volume->devicelist.dl_LockList=0;
	volume->devicelist.dl_DiskType=volume->dostype;
	if (volume->devicelist.dl_OldName)
	{
		volume->devicelist.dl_OldName=(BSTR)BADDR(volume->devicelist.dl_OldName);
	}
	else
	{
		volume->devicelist.dl_OldName=(BSTR)AllocVec(32,MEMF_CLEAR | MEMF_PUBLIC);
		if (volume->devicelist.dl_OldName==0)
			return DOSFALSE;
	}
	for (i=0;i<name[0];i++)
		AROS_BSTR_putchar(volume->devicelist.dl_OldName, i, name[i+1]);
	AROS_BSTR_setstrlen(volume->devicelist.dl_OldName, name[0]);
	volume->devicelist.dl_OldName=MKBADDR(volume->devicelist.dl_OldName);
	return DOSTRUE;
}

/*******************************************
 Name  : addDosVolume
 Descr.: adds a new volume to dos
 Input : volume - volume to add
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG addDosVolume(struct afsbase *afsbase, struct Volume *volume) {
struct DosList *doslist, *dl=0;
char string[32];
char *bname;
UBYTE i;

	bname=BADDR(volume->devicelist.dl_OldName);
	for (i=0;i<AROS_BSTR_strlen(bname);i++)
		string[i]=AROS_BSTR_getchar(bname,i);
	string[AROS_BSTR_strlen(bname)]=0;
	/* is the volume already in the list? */
	if ((doslist=LockDosList(LDF_WRITE | LDF_VOLUMES)))
	{
		if ((dl=FindDosEntry(doslist,string,LDF_VOLUMES)))
		{
			if (((struct AfsHandle *)dl->dol_Unit)->volume==volume)
			{
				if (dl->dol_misc.dol_volume.dol_LockList)
				{
					volume->locklist=dl->dol_misc.dol_volume.dol_LockList;
				}
			}
			else
			{
				dl=0;
			}
		}
		UnLockDosList(LDF_WRITE | LDF_VOLUMES);
	}
	/* if not create a new doslist */
	if (!dl)
	{
		if (!(doslist=MakeDosEntry(string,DLT_VOLUME)))
			return DOSFALSE;
		doslist->dol_Unit=(struct Unit *)&volume->ah;
		doslist->dol_Device=volume->device;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Days=
			volume->devicelist.dl_VolumeDate.ds_Days;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Minute=
			volume->devicelist.dl_VolumeDate.ds_Minute;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Tick=
			volume->devicelist.dl_VolumeDate.ds_Tick;
		AddDosEntry(doslist);
		/* if we re-use "volume" clear locklist */
		volume->locklist = 0;
	}
	return DOSTRUE;
}

/*******************************************
 Name  : remDosVolume
 Descr.: removes a volume added by addDosVolume
         or if there are some locks active
         set dol_LockList
 Input : volume - volume to remove
 Output: -
 Note  : displays a message if volume couldn't
         be found in the system
********************************************/
void remDosVolume(struct afsbase *afsbase, struct Volume *volume) {
struct DosList *doslist,*dl;
char *bname;
char string[32];
UBYTE i;

	if (volume->dostype == 0x444F5300)
	{
		bname=BADDR(volume->devicelist.dl_OldName);
		if (bname)
		{
			for (i=0;i<AROS_BSTR_strlen(bname);i++)
				string[i]=AROS_BSTR_getchar(bname,i);
			string[AROS_BSTR_strlen(bname)]=0;
			if ((doslist=LockDosList(LDF_WRITE | LDF_VOLUMES)))
			{
				if ((dl=FindDosEntry(doslist,string,LDF_VOLUMES)))
				{
					if (volume->locklist)
					{
						dl->dol_misc.dol_volume.dol_LockList=volume->locklist;
					}
					else
					{
						RemDosEntry(dl);
						FreeDosEntry(dl);
					}
				}
				else
					showText(afsbase, "doslist not in chain");
				UnLockDosList(LDF_WRITE | LDF_VOLUMES);
			}
		}
	}
}

/*******************************************
 Name  : newMedium
 Descr.: build infos for a new medium
 Input : afsbase -
         volume  -
 Output: 0 for success; error code otherwise
********************************************/
LONG newMedium(struct afsbase *afsbase, struct Volume *volume) {
struct BlockCache *blockbuffer;
UWORD i;

	blockbuffer=getBlock(afsbase, volume,0);
	if (blockbuffer == NULL)
		return ERROR_UNKNOWN;
	volume->dostype=AROS_BE2LONG(blockbuffer->buffer[0]) & 0xFFFFFF00;
	if (volume->dostype != 0x444F5300)
	{
		blockbuffer=getBlock(afsbase, volume, 1);
		volume->dostype=AROS_BE2LONG(blockbuffer->buffer[0]) & 0xFFFFFF00;
	}
	volume->flags |= AROS_BE2LONG(blockbuffer->buffer[0]) & 0xFF;
	if (volume->dostype != 0x444F5300)
		return ERROR_NOT_A_DOS_DISK;
	blockbuffer=getBlock(afsbase, volume,volume->rootblock);
	if (blockbuffer == NULL)
		return ERROR_UNKNOWN;
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]) != ST_ROOT)
		return ERROR_NOT_A_DOS_DISK;
	for (i=0;i<=24;i++)
	{
		volume->bitmapblockpointers[i]=AROS_BE2LONG
			(
				blockbuffer->buffer[BLK_BITMAP_POINTERS_START(volume)+i]
			);
	}
	volume->bitmapextensionblock=AROS_BE2LONG
		(
			blockbuffer->buffer[BLK_BITMAP_EXTENSION(volume)]
		);
	if (!blockbuffer->buffer[BLK_BITMAP_VALID_FLAG(volume)])
	{
		volume->usedblockscount=0;
		showError(afsbase, ERR_DISKNOTVALID);
	}
	else
	{
		blockbuffer->flags |= BCF_USED;
		volume->usedblockscount=countUsedBlocks(afsbase, volume);
		blockbuffer->flags &= ~BCF_USED;
	}
	if (!initDeviceList(afsbase, volume,blockbuffer))
		return ERROR_NO_FREE_STORE;
	if (!addDosVolume(afsbase, volume))
	{
		showError(afsbase, ERR_DOSENTRY);
		remDosVolume(afsbase, volume);
		return ERROR_UNKNOWN;
	}
	/* for free block searching */
	volume->lastaccess=volume->rootblock;
	return 0;
}

void nsdCheck(struct afsbase *afsbase, struct Volume *volume) {
struct NSDeviceQueryResult nsdq;
UWORD *cmdcheck;

	if (
			(
				(volume->startblock+(volume->rootblock*2))*  /* last block */
				(volume->SizeBlock*4/512)	/* 1 portion (block) equals 512 (bytes) */
			)>8388608)
	{
		nsdq.SizeAvailable=0;
		nsdq.DevQueryFormat=0;
		volume->iorequest->iotd_Req.io_Command=NSCMD_DEVICEQUERY;
		volume->iorequest->iotd_Req.io_Data=&nsdq;
		volume->iorequest->iotd_Req.io_Length=sizeof(struct NSDeviceQueryResult);
		if (DoIO((struct IORequest *)volume->iorequest)==IOERR_NOCMD)
		{
			D(bug("afs.handler: initVolume-NSD: device doesn't understand NSD-Query\n"));
		}
		else
		{
			if (
					(volume->iorequest->iotd_Req.io_Actual>sizeof(struct NSDeviceQueryResult)) ||
					(volume->iorequest->iotd_Req.io_Actual==0) ||
					(volume->iorequest->iotd_Req.io_Actual!=nsdq.SizeAvailable)
				)
			{
				D(bug("afs.handler: initVolume-NSD: WARNING wrong io_Actual using NSD\n"));
			}
			else
			{
				D(bug("afs.handler: initVolume-NSD: using NSD commands\n"));
				if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
					D(bug("afs.handler: initVolume-NSD: WARNING no trackdisk type\n"));
				for (cmdcheck=nsdq.SupportedCommands;*cmdcheck;cmdcheck++)
				{
					if (*cmdcheck == NSCMD_TD_READ64)
						volume->cmdread = NSCMD_TD_READ64;
					if (*cmdcheck == NSCMD_TD_WRITE64);
						volume->cmdwrite = NSCMD_TD_WRITE64;
					if (*cmdcheck == NSCMD_TD_SEEK64)
						volume->cmdseek = NSCMD_TD_SEEK64;
					if (*cmdcheck == NSCMD_TD_FORMAT64)
						volume->cmdformat = NSCMD_TD_FORMAT64;
				}
				if (
						(volume->cmdread!=NSCMD_TD_READ64) ||
						(volume->cmdwrite!=NSCMD_TD_WRITE64)
					)
					D(bug("afs.handler: initVolume-NSD: WARNING no READ64/WRITE64\n")); 
			}
		}
	}
	else
	{
			D(bug("afs.handler: initVolume-NSD: no need for NSD\n"));
	}
}

struct IOExtTD *openBlockDevice
	(
		struct afsbase *afsbase,
		struct MsgPort *mp,
		char *blockdevice,
		ULONG unit,
		ULONG flags
	)
{
struct IOExtTD *ioreq;

	ioreq=(struct IOExtTD *)CreateIORequest(mp, sizeof(struct IOExtTD));
	if (ioreq)
	{
		if (OpenDevice(blockdevice, unit, (struct IORequest *)ioreq, flags))
		{
			showError(afsbase, ERR_DEVICE, blockdevice);
			DeleteIORequest((struct IORequest *)ioreq);
			ioreq = 0;
		}
	}
	return ioreq;
}

void closeBlockDevice(struct afsbase *afsbase, struct IOExtTD *ioreq) {

	if (ioreq->iotd_Req.io_Device)
		CloseDevice((struct IORequest *)&ioreq->iotd_Req);
	DeleteIORequest((APTR)ioreq);
}

void checkAddChangeInt(struct afsbase *afsbase, struct Volume *volume) {
struct DriveGeometry dg;

	if (!getGeometry(afsbase, volume, &dg))
	{
		if (dg.dg_Flags & DGF_REMOVABLE)
		{
			volume->iochangeint = openBlockDevice
				(
					afsbase,
					volume->ioport,
					volume->blockdevice,
					volume->unit,
					0
				);
			if (volume->iochangeint)
			{
				addChangeInt(afsbase, volume);
			}
		}
	}
}

/*******************************************
 Name  : initVolume
 Descr.: maybe a better name would be mountVolume
         allocate resources for a new mounted device
 Input : afsbase     - 
         device      - device pointer
         blockdevice - name of blockdevice
         unit        - unit number of blockdevice
         devicedef   - medium geometry data
         error       - return error code
 Output: 0 on error (error set dos dos error);
         pointer to struct Volume on success
********************************************/
struct Volume *initVolume
	(
		struct afsbase *afsbase,
		struct Device *device,
		STRPTR blockdevice,
		ULONG unit,
		struct DosEnvec *devicedef,
		ULONG *error
	)
{
struct Volume *volume;

	volume=AllocMem(sizeof(struct Volume),MEMF_PUBLIC | MEMF_CLEAR);
	if (volume)
	{
		volume->device=device;
		volume->blockdevice=blockdevice;
		volume->unit=unit;
		volume->SizeBlock=devicedef->de_SizeBlock;
		if (devicedef->de_TableSize>=20)
			volume->bootblocks=devicedef->de_BootBlocks;
		else
			volume->bootblocks=devicedef->de_Reserved;
		volume->blockcache=initCache(afsbase, volume, devicedef->de_NumBuffers);
		if (volume->blockcache)
		{
			volume->ioport=CreateMsgPort();
			if (volume->ioport)
			{
				volume->iorequest = openBlockDevice
					(
						afsbase,
						volume->ioport,
						volume->blockdevice,
						volume->unit,
						0
					);
				if (volume->iorequest)
				{
					checkAddChangeInt(afsbase, volume);
					if (StrCmp(volume->blockdevice,"trackdisk.device"))
						volume->flags |= VOLF_TRACKDISK;
					volume->rootblock=
						(
							(
								(
									devicedef->de_HighCyl-devicedef->de_LowCyl+1
								)*devicedef->de_Surfaces*devicedef->de_BlocksPerTrack
							)-1+devicedef->de_Reserved
						)/2;	/* root in the middle of a partition */
					volume->startblock=
							devicedef->de_LowCyl*
							devicedef->de_Surfaces*
							devicedef->de_BlocksPerTrack;
					volume->lastblock=
							(
								(devicedef->de_HighCyl+1)
								*devicedef->de_Surfaces
								*devicedef->de_BlocksPerTrack
							)-1;
					volume->cmdread=CMD_READ;
					volume->cmdwrite=CMD_WRITE;
					volume->cmdseek=TD_SEEK;
					volume->cmdformat=TD_FORMAT;
					nsdCheck(afsbase, volume);
					volume->vbl_int.is_Code = (void(*)())&timercode;
					volume->vbl_int.is_Data = volume;
					volume->ah.volume=volume;
					if (diskPresent(afsbase, volume))
					{
						volume->flags |= VOLF_DISK_IN;
						*error = newMedium(afsbase, volume);
					}
					else
						*error = 0;
					volume->ioport->mp_SigTask=afsbase->port.mp_SigTask;
					if ((!*error) || (*error=ERROR_NOT_A_DOS_DISK))
					{
						D(bug("afs.handler: initVolume: BootBlocks=%ld\n",volume->bootblocks));
						D(bug("afs.handler: initVolume: RootBlock=%ld\n",volume->rootblock));
						volume->afsbase = afsbase;
						volume->ah.header_block=volume->rootblock;
						return volume;
					}
				}
				else
				{
					*error=ERROR_NO_FREE_STORE;
				}
				DeleteMsgPort(volume->ioport);
			}
			else
			{
				*error=ERROR_NO_FREE_STORE;
			}
			freeCache(afsbase, volume->blockcache);
		}
		else
		{
			*error=ERROR_NO_FREE_STORE;
		}
		FreeMem(volume,sizeof(struct Volume));
	}
	else
		*error=ERROR_NO_FREE_STORE;
	return 0;
}

/*******************************************
 Name  : uninitVolume
 Descr.: maybe a better name would be unmountVolume
         free resources allocated by initVolume
 Input : afsbase - 
         volume  - volume to unmount
 Output: -
********************************************/
void uninitVolume(struct afsbase *afsbase, struct Volume *volume) {

	remDosVolume(afsbase, volume);
	remChangeInt(afsbase, volume);
	if (volume->blockcache)
		freeCache(afsbase, volume->blockcache);
	if (volume->devicelist.dl_OldName)
		FreeVec(BADDR(volume->devicelist.dl_OldName));
	if (volume->iorequest)
		closeBlockDevice(afsbase, volume->iorequest);
	if (volume->iochangeint)
		closeBlockDevice(afsbase, volume->iochangeint);
	if (volume->ioport)
		DeleteMsgPort(volume->ioport);
	FreeMem(volume,sizeof(struct Volume));
}

void checkDeviceFlags(struct afsbase *afsbase) {
struct Volume *volume;

	volume = (struct Volume *)afsbase->device_list.lh_Head;
	while (volume->ln.ln_Succ)
	{
		if (volume->flags & VOLF_MOTOR_OFF)
		{
			motorOff(afsbase, volume);
			volume->flags &= ~VOLF_MOTOR_OFF;
		}
		else if (volume->flags & VOLF_MEDIA_CHANGE)
		{
			if (diskPresent(afsbase, volume))
			{
				newMedium(afsbase, volume);
				volume->flags |= VOLF_DISK_IN;
			}
			else
			{
				flush(afsbase, volume);
				remDosVolume(afsbase, volume);
				volume->flags &= ~VOLF_DISK_IN;
			}
			volume->flags &= ~VOLF_MEDIA_CHANGE;
		}
		volume = (struct Volume *)volume->ln.ln_Succ;
	}
}

