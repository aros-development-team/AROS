/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <exec/types.h>
#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include "os.h"
#include "afsblocks.h"
#include "error.h"
#include "extstrings.h"
#include "volumes.h"
#include "baseredef.h"

/************************** DOS ******************************************/
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

LONG osMediumInit(struct afsbase *afsbase, struct Volume *volume, struct BlockCache *block) {

	if (!initDeviceList(afsbase, volume, block))
		return ERROR_NO_FREE_STORE;
	if (!addDosVolume(afsbase, volume))
	{
		showError(afsbase, ERR_DOSENTRY);
		remDosVolume(afsbase, volume);
		return ERROR_UNKNOWN;
	}
	return 0;
}

void osMediumFree(struct afsbase *afsbase, struct Volume *volume, LONG all) {
	remDosVolume(afsbase, volume);
	if (all)
		if (volume->devicelist.dl_OldName)
			FreeVec(BADDR(volume->devicelist.dl_OldName));
}

/************************** I/O ******************************************/
struct IOExtTD *openDevice
	(
		struct afsbase *afsbase,
		struct MsgPort *mp,
		STRPTR device,
		ULONG unit,
		ULONG flags
	)
{
struct IOExtTD *ioreq;

	ioreq=(struct IOExtTD *)CreateIORequest(mp, sizeof(struct IOExtTD));
	if (ioreq != NULL)
	{
		if (OpenDevice(device, unit, (struct IORequest *)ioreq, flags) == 0)
		{
			return ioreq;
		}
		else
			showError(afsbase, ERR_DEVICE, device);
		DeleteIORequest((struct IORequest *)ioreq);
	}
	return NULL;
}

void closeDevice(struct afsbase *afsbase, struct IOExtTD *ioreq) {
	if (ioreq->iotd_Req.io_Device != NULL)
		CloseDevice((struct IORequest *)&ioreq->iotd_Req);
	DeleteIORequest((APTR)ioreq);
}

LONG getGeometry
	(struct afsbase *afsbase, struct IOHandle *ioh, struct DriveGeometry *dg)
{
	ioh->ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
	ioh->ioreq->iotd_Req.io_Data = dg;
	ioh->ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
	return DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
}

VOID changeIntCode(struct IOHandle *, APTR, struct ExecBase *);

LONG addChangeInt(struct afsbase *afsbase, struct IOHandle *ioh) {

	ioh->mc_int.is_Code = (void(*)())&changeIntCode;
	ioh->mc_int.is_Data = ioh;
	ioh->iochangeint->iotd_Req.io_Command = TD_ADDCHANGEINT;
	ioh->iochangeint->iotd_Req.io_Flags = 0;
	ioh->iochangeint->iotd_Req.io_Length = sizeof(struct Interrupt);
	ioh->iochangeint->iotd_Req.io_Data = &ioh->mc_int;
	ioh->iochangeint->iotd_Req.io_Error = 0;
	SendIO((struct IORequest *)&ioh->iochangeint->iotd_Req);
	return ioh->iochangeint->iotd_Req.io_Error;
}

void remChangeInt(struct afsbase *afsbase, struct IOHandle *ioh) {

	if (
			(ioh->iochangeint != 0) &&
			(ioh->iochangeint->iotd_Req.io_Error == 0)
		)
	{
		ioh->iochangeint->iotd_Req.io_Command = TD_REMCHANGEINT;
		DoIO((struct IORequest *)&ioh->iochangeint->iotd_Req);
	}
}

void checkAddChangeInt(struct afsbase *afsbase, struct IOHandle *ioh) {
struct DriveGeometry dg;

	if (!getGeometry(afsbase, ioh, &dg))
	{
		if (dg.dg_Flags & DGF_REMOVABLE)
		{
			ioh->iochangeint = openDevice
				(afsbase, ioh->mp, ioh->blockdevice, ioh->unit, ioh->flags);
			if (ioh->iochangeint)
			{
				addChangeInt(afsbase, ioh);
			}
		}
	}
}

UBYTE diskPresent(struct afsbase *afsbase, struct IOHandle *ioh) {

	ioh->ioreq->iotd_Req.io_Command=TD_CHANGESTATE;
	DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
	return ioh->ioreq->iotd_Req.io_Actual==0;
}

int timercode(struct Custom *, struct IOHandle *, APTR, struct ExecBase *);

struct IOHandle *openBlockDevice(struct afsbase *afsbase, struct IOHandle *ioh)
{

	ioh->mp = CreateMsgPort();
	if (ioh->mp != NULL)
	{
		ioh->ioreq = openDevice(afsbase, ioh->mp, ioh->blockdevice, ioh->unit, ioh->flags);
		if (ioh->ioreq != NULL)
		{
			ioh->cmdread = CMD_READ;
			ioh->cmdwrite = CMD_WRITE;
			ioh->cmdseek = TD_SEEK;
			ioh->cmdformat = TD_FORMAT;
			ioh->vbl_int.is_Code = (void(*)())&timercode;
			ioh->vbl_int.is_Data = ioh;
			ioh->afsbase = afsbase;
			if (StrCmp(ioh->blockdevice, "trackdisk.device"))
				ioh->ioflags |= IOHF_TRACKDISK;
			if (diskPresent(afsbase, ioh))
				ioh->ioflags |= IOHF_DISK_IN;
			checkAddChangeInt(afsbase, ioh);
			return ioh;
		}
		DeleteMsgPort(ioh->mp);
	}
	return NULL;
}

void closeBlockDevice(struct afsbase *afsbase, struct IOHandle *ioh) {

	remChangeInt(afsbase, ioh);
	if (ioh->iochangeint != NULL)
		closeDevice(afsbase, ioh->iochangeint);
	if (ioh->ioreq != NULL)
		closeDevice(afsbase, ioh->ioreq);
	if (ioh->mp != NULL)
		DeleteMsgPort(ioh->mp);
}

void motorOff(struct afsbase *afsbase, struct IOHandle *ioh) {

	ioh->ioreq->iotd_Req.io_Command=TD_MOTOR;
	ioh->ioreq->iotd_Req.io_Length=0;
	DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
}

void checkDeviceFlags(struct afsbase *afsbase) {
struct Volume *volume;
struct IOHandle *ioh;

	volume = (struct Volume *)afsbase->device_list.lh_Head;
	while (volume->ln.ln_Succ)
	{
		ioh = &volume->ioh;
		if (ioh->ioflags & IOHF_MOTOR_OFF)
		{
			motorOff(afsbase, ioh);
			ioh->ioflags &= ~IOHF_MOTOR_OFF;
		}
		else if (ioh->ioflags & IOHF_MEDIA_CHANGE)
		{
			if (diskPresent(afsbase, ioh))
			{
				newMedium(afsbase, volume);
				ioh->ioflags |= IOHF_DISK_IN;
			}
			else
			{
				flush(afsbase, volume);
				remDosVolume(afsbase, volume);
				ioh->ioflags &= ~IOHF_DISK_IN;
			}
			ioh->ioflags &= ~IOHF_MEDIA_CHANGE;
		}
		volume = (struct Volume *)volume->ln.ln_Succ;
	}
}

void check64BitSupport(struct afsbase *afsbase, struct Volume *volume) {
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
		volume->ioh.ioreq->iotd_Req.io_Command=NSCMD_DEVICEQUERY;
		volume->ioh.ioreq->iotd_Req.io_Data=&nsdq;
		volume->ioh.ioreq->iotd_Req.io_Length=sizeof(struct NSDeviceQueryResult);
		if (DoIO((struct IORequest *)volume->ioh.ioreq)==IOERR_NOCMD)
		{
			D(bug("[afs] initVolume-NSD: device doesn't understand NSD-Query\n"));
		}
		else
		{
			if (
					(volume->ioh.ioreq->iotd_Req.io_Actual>sizeof(struct NSDeviceQueryResult)) ||
					(volume->ioh.ioreq->iotd_Req.io_Actual==0) ||
					(volume->ioh.ioreq->iotd_Req.io_Actual!=nsdq.SizeAvailable)
				)
			{
				D(bug("[afs] initVolume-NSD: WARNING wrong io_Actual using NSD\n"));
			}
			else
			{
				D(bug("[afs] initVolume-NSD: using NSD commands\n"));
				if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
					D(bug("[afs] initVolume-NSD: WARNING no trackdisk type\n"));
				for (cmdcheck=nsdq.SupportedCommands;*cmdcheck;cmdcheck++)
				{
					if (*cmdcheck == NSCMD_TD_READ64)
						volume->ioh.cmdread = NSCMD_TD_READ64;
					if (*cmdcheck == NSCMD_TD_WRITE64);
						volume->ioh.cmdwrite = NSCMD_TD_WRITE64;
					if (*cmdcheck == NSCMD_TD_SEEK64)
						volume->ioh.cmdseek = NSCMD_TD_SEEK64;
					if (*cmdcheck == NSCMD_TD_FORMAT64)
						volume->ioh.cmdformat = NSCMD_TD_FORMAT64;
				}
				if (
						(volume->ioh.cmdread!=NSCMD_TD_READ64) ||
						(volume->ioh.cmdwrite!=NSCMD_TD_WRITE64)
					)
					D(bug("[afs] initVolume-NSD: WARNING no READ64/WRITE64\n")); 
			}
		}
	}
	else
	{
			D(bug("[afs] initVolume-NSD: no need for NSD\n"));
	}
}

/*******************************************
 Name  : flush
 Descr.: flush buffers and update disk (sync)
 Input : afsbase -
         volume  - volume to flush
 Output: DOSTRUE
********************************************/
ULONG flush(struct afsbase *afsbase, struct Volume *volume) {

	flushCache(volume->blockcache);
	volume->ioh.ioreq->iotd_Req.io_Command=CMD_UPDATE;
	DoIO((struct IORequest *)&volume->ioh.ioreq->iotd_Req);
	//turn off motor
	return DOSTRUE;
}

ULONG readwriteDisk
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		ULONG start,
		ULONG count,
		APTR mem,
		ULONG cmd
	)
{
ULONG retval;
struct IOHandle *ioh = &volume->ioh;
UQUAD offset;

	D(bug("[afs]    readDisk: reading block %ld\n",start));
	if (
			((volume->startblock+start)<=volume->lastblock) &&
			((volume->startblock+start+count-1)<=volume->lastblock)
		)
	{
		ioh->ioreq->iotd_Req.io_Command=cmd;
		ioh->ioreq->iotd_Req.io_Length=count*BLOCK_SIZE(volume);
		ioh->ioreq->iotd_Req.io_Data=mem;

		offset  = start+volume->startblock;
		offset *= BLOCK_SIZE(volume);

		ioh->ioreq->iotd_Req.io_Offset=0xFFFFFFFF & offset;
		ioh->ioreq->iotd_Req.io_Actual=offset>>32;
		retval=DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
		if (retval)
			showError(afsbase, ERR_READWRITE, retval);
		if (ioh->ioflags & IOHF_TRACKDISK)
		{
			if (ioh->moff_time>0)
			{
				ioh->moff_time=100;
			}
			else
			{
				ioh->moff_time=100;
				AddIntServer(INTB_VERTB, &ioh->vbl_int);
			}
		}
	}
	else
	{
		showText(afsbase, "Attempted to read/write block outside range! (range: %ld-%ld, start: %ld, count: %ld", volume->startblock, volume->lastblock, start, count);
		retval = 1;
	}
	return retval;
}

ULONG readDisk(struct afsbase *afsbase, struct Volume *volume, ULONG start, ULONG count, APTR data) {
	return readwriteDisk(afsbase, volume, start, count, data, volume->ioh.cmdread);
}

ULONG writeDisk(struct afsbase *afsbase, struct Volume *volume, ULONG start, ULONG count, APTR data) {
	return readwriteDisk(afsbase, volume, start, count, data, volume->ioh.cmdwrite);
}
#undef SysBase

AROS_UFH4(int, timercode,
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(struct IOHandle *, ioh, A1),
    AROS_UFHA(APTR, is_Code, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT
	if (--ioh->moff_time == 0)
	{
		ioh->ioflags |= IOHF_MOTOR_OFF;
		Signal
		(
			ioh->afsbase->port.mp_SigTask,
			1<<ioh->afsbase->port.mp_SigBit
		);
		RemIntServer(INTB_VERTB, &ioh->vbl_int);
	}
	return 0;
	AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, changeIntCode,
    AROS_UFHA(struct IOHandle *, ioh, A1),
    AROS_UFHA(APTR, is_Code, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	ioh->ioflags |= IOHF_MEDIA_CHANGE;
	Signal(ioh->afsbase->port.mp_SigTask, 1<<ioh->afsbase->port.mp_SigBit);

	AROS_USERFUNC_EXIT
}

