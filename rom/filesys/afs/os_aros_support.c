/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <exec/types.h>
#include <devices/input.h>
#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <intuition/intuition.h>

#include "os.h"
#include "afsblocks.h"
#include "error.h"
#include "extstrings.h"
#include "volumes.h"
#include "baseredef.h"

/*
 * The deadlock bug caused by calling LockDosList() when adding avolume node
 * has now been fixed, but a deadlock still occurs when trying to mount an
 * AFS disk image with fdsk.device if the image file is on an AFS volume.
 * This is because of there only being one filesystem process that is shared
 * between all AFS devices.
 */

// pushes an IECLASS event in the input stream.
static void SendEvent(struct AFSBase *afsbase, LONG event) {
    struct IOStdReq *InputRequest;
    struct MsgPort *InputPort;
    struct InputEvent *ie;
    if ((InputPort = (struct MsgPort*)CreateMsgPort())) {

        if ((InputRequest = (struct IOStdReq*)CreateIORequest(InputPort, sizeof(struct IOStdReq)))) {

            if (!OpenDevice("input.device", 0, (struct IORequest*)InputRequest, 0)) {

                if ((ie = AllocVec(sizeof(struct InputEvent), MEMF_PUBLIC|MEMF_CLEAR))) {
                    ie->ie_Class = event;
                    InputRequest->io_Command = IND_WRITEEVENT;
                    InputRequest->io_Data = ie;
                    InputRequest->io_Length = sizeof(struct InputEvent);

                    DoIO((struct IORequest*)InputRequest);

                    FreeVec(ie);
                }
                CloseDevice((struct IORequest*)InputRequest);
            }
            DeleteIORequest ((APTR)InputRequest);
        }
        DeleteMsgPort (InputPort);
    }
}

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
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *rootblock
	)
{
STRPTR name;
BSTR newname;
UBYTE i;

	name=(char *)rootblock->buffer+(BLK_DISKNAME_START(volume)*4);
	volume->devicelist.dl_Next = 0;
	volume->devicelist.dl_Type = DLT_VOLUME;
	volume->devicelist.dl_Lock = 0;
	volume->devicelist.dl_VolumeDate.ds_Days =
		AROS_BE2LONG(rootblock->buffer[BLK_ROOT_DAYS(volume)]);
	volume->devicelist.dl_VolumeDate.ds_Minute =
		AROS_BE2LONG(rootblock->buffer[BLK_ROOT_MINS(volume)]);
	volume->devicelist.dl_VolumeDate.ds_Tick =
		AROS_BE2LONG(rootblock->buffer[BLK_ROOT_TICKS(volume)]);
	volume->devicelist.dl_LockList = 0;
	volume->devicelist.dl_DiskType = volume->dostype;
	if (volume->devicelist.dl_Name != BNULL)
	{
		newname = volume->devicelist.dl_Name;
	}
	else
	{
		newname = MKBADDR(AllocVec(32,MEMF_CLEAR | MEMF_PUBLIC));
		if (newname == BNULL)
			return DOSFALSE;
	}
	for (i=0; i<name[0]; i++)
		AROS_BSTR_putchar(newname, i, name[i+1]);
	AROS_BSTR_setstrlen(newname, name[0]);
	volume->devicelist.dl_Name = newname;
	return DOSTRUE;
}

/*******************************************
 Name  : attemptAddDosVolume
 Descr.: adds a new volume to dos
 Input : volume - volume to add
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG attemptAddDosVolume(struct AFSBase *afsbase, struct Volume *volume) {
struct DosList *doslist;
struct DosList *dl=NULL;
char string[32];
BSTR bname;
UBYTE i;

	if (volume->volumenode) {
	    D(bug("[afs 0x%08lX] VolumeNode is already present!\n", volume));
	    return DOSTRUE;
	}
	bname = volume->devicelist.dl_Name;
	for (i=0; i<AROS_BSTR_strlen(bname); i++)
		string[i] = AROS_BSTR_getchar(bname,i);
	string[AROS_BSTR_strlen(bname)] = 0;
	D(bug("[afs 0x%08lX] Processing inserted volume %s\n", volume, string));
	/* is the volume already in the list? */
	doslist = AttemptLockDosList(LDF_WRITE | LDF_VOLUMES);
	if (doslist != NULL)
	{
		dl = FindDosEntry(doslist,string,LDF_VOLUMES);
		UnLockDosList(LDF_WRITE | LDF_VOLUMES);
	}
	else
		return TRUE;

	/* if not create a new doslist */
	if (dl == NULL)
	{
		D(bug("[afs 0x%08lX] Creating new VolumeNode\n", volume));
		doslist = MakeDosEntry(string,DLT_VOLUME);
		if (doslist == NULL)
			return DOSFALSE;
		doslist->dol_Task = &((struct Process *)FindTask(NULL))->pr_MsgPort;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Days =
			volume->devicelist.dl_VolumeDate.ds_Days;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Minute =
			volume->devicelist.dl_VolumeDate.ds_Minute;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Tick =
			volume->devicelist.dl_VolumeDate.ds_Tick;
		AddDosEntry(doslist);
		/* if we re-use "volume" clear locklist */
		volume->locklist = NULL;
		dl = doslist;
	}
	volume->volumenode = dl;
	SendEvent(afsbase, IECLASS_DISKINSERTED);
	return DOSTRUE;
}

/*******************************************
 Name  : remDosVolume
 Descr.: removes a volume added by addDosVolume
         or if there are some locks active
         set dol_LockList
 Input : volume - volume to remove
 Output: -
********************************************/
void remDosVolume(struct AFSBase *afsbase, struct Volume *volume) {
struct DosList *dl;

	dl = volume->volumenode;
	if (dl) {
		if (volume->locklist != NULL)
		{
			D(bug("[afs 0x%08lX] VolumeNode in use, keeping as offline\n", volume));
			dl->dol_misc.dol_volume.dol_LockList = MKBADDR(volume->locklist);
		}
		else
		{
			D(bug("[afs 0x%08lX] Removing VolumeNode\n", volume));
			remDosNode(afsbase, dl);
		}
		volume->volumenode = NULL;
	}
}

/*******************************************
 Name  : remDosNode
 Descr.: removes a DOS volume node
 Input : dl - volume to remove
 Output: -
********************************************/
void remDosNode(struct AFSBase *afsbase, struct DosList *dl)
{
	RemDosEntry(dl);
	FreeDosEntry(dl);
	SendEvent(afsbase, IECLASS_DISKREMOVED);
}

LONG osMediumInit
	(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *block)
{
	if (!initDeviceList(afsbase, volume, block))
		return ERROR_NO_FREE_STORE;
	if (!attemptAddDosVolume(afsbase, volume))
	{
		showError(afsbase, ERR_DOSENTRY);
		remDosVolume(afsbase, volume);
		return ERROR_UNKNOWN;
	}
	return 0;
}

void osMediumFree(struct AFSBase *afsbase, struct Volume *volume, LONG all) {
	remDosVolume(afsbase, volume);
	if (all)
		if (volume->devicelist.dl_Name != BNULL)
			FreeVec(BADDR(volume->devicelist.dl_Name));
}

/************************** I/O ******************************************/
struct IOExtTD *openDevice
	(
		struct AFSBase *afsbase,
		struct MsgPort *mp,
		STRPTR device,
		ULONG unit,
		ULONG flags
	)
{
struct IOExtTD *ioreq;

	ioreq = (struct IOExtTD *)CreateIORequest(mp, sizeof(struct IOExtTD));
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

void closeDevice(struct AFSBase *afsbase, struct IOExtTD *ioreq) {
	if (ioreq->iotd_Req.io_Device != NULL)
		CloseDevice((struct IORequest *)&ioreq->iotd_Req);
	DeleteIORequest((APTR)ioreq);
}

LONG getGeometry
	(struct AFSBase *afsbase, struct IOHandle *ioh, struct DriveGeometry *dg)
{
	ioh->ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
	ioh->ioreq->iotd_Req.io_Data = dg;
	ioh->ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
	return DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
}

AROS_UFIP(changeIntCode);

LONG addChangeInt(struct AFSBase *afsbase, struct IOHandle *ioh) {

	ioh->mc_int.is_Code = (VOID_FUNC)changeIntCode;
	ioh->mc_int.is_Data = ioh;
	ioh->iochangeint->iotd_Req.io_Command = TD_ADDCHANGEINT;
	ioh->iochangeint->iotd_Req.io_Flags = 0;
	ioh->iochangeint->iotd_Req.io_Length = sizeof(struct Interrupt);
	ioh->iochangeint->iotd_Req.io_Data = &ioh->mc_int;
	ioh->iochangeint->iotd_Req.io_Error = 0;
	SendIO((struct IORequest *)&ioh->iochangeint->iotd_Req);
	return ioh->iochangeint->iotd_Req.io_Error;
}

void remChangeInt(struct AFSBase *afsbase, struct IOHandle *ioh) {

	if (
			(ioh->iochangeint != NULL) &&
			(ioh->iochangeint->iotd_Req.io_Error == 0)
		)
	{
		ioh->iochangeint->iotd_Req.io_Command = TD_REMCHANGEINT;
		DoIO((struct IORequest *)&ioh->iochangeint->iotd_Req);
	}
}

void checkAddChangeInt(struct AFSBase *afsbase, struct IOHandle *ioh) {
struct DriveGeometry dg;

	if (!getGeometry(afsbase, ioh, &dg))
	{
		if (dg.dg_Flags & DGF_REMOVABLE)
		{
			ioh->iochangeint = openDevice
				(afsbase, ioh->mp, ioh->blockdevice, ioh->unit, ioh->flags);
			if (ioh->iochangeint != NULL)
			{
				addChangeInt(afsbase, ioh);
			}
		}
		ioh->sectorsize = dg.dg_SectorSize;
	}
}

UBYTE diskPresent(struct AFSBase *afsbase, struct IOHandle *ioh) {

	ioh->ioreq->iotd_Req.io_Command = TD_CHANGESTATE;
	DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
	return ioh->ioreq->iotd_Req.io_Actual == 0;
}

BOOL diskWritable(struct AFSBase *afsbase, struct IOHandle *ioh)
{
	ioh->ioreq->iotd_Req.io_Command = TD_PROTSTATUS;
	DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
	return ioh->ioreq->iotd_Req.io_Actual == 0;
}

ULONG sectorSize(struct AFSBase *afsbase, struct IOHandle *ioh)
{
	return ioh->sectorsize;
}

struct IOHandle *openBlockDevice(struct AFSBase *afsbase, struct IOHandle *ioh)
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
			ioh->afsbase = afsbase;
			if (diskPresent(afsbase, ioh))
				ioh->ioflags |= IOHF_DISK_IN;
			checkAddChangeInt(afsbase, ioh);
			return ioh;
		}
		DeleteMsgPort(ioh->mp);
	}
	return NULL;
}

void closeBlockDevice(struct AFSBase *afsbase, struct IOHandle *ioh) {

	remChangeInt(afsbase, ioh);
	if (ioh->iochangeint != NULL)
		closeDevice(afsbase, ioh->iochangeint);
	if (ioh->ioreq != NULL)
		closeDevice(afsbase, ioh->ioreq);
	if (ioh->mp != NULL)
		DeleteMsgPort(ioh->mp);
}

void motorOff(struct AFSBase *afsbase, struct IOHandle *ioh) {

	ioh->ioreq->iotd_Req.io_Command = TD_MOTOR;
	ioh->ioreq->iotd_Req.io_Length = 0;
	DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
}

void checkDeviceFlags(struct AFSBase *afsbase) {
struct Volume *volume;
struct IOHandle *ioh;

	volume = (struct Volume *)afsbase->device_list.lh_Head;
	while (volume->ln.ln_Succ != NULL)
	{
		ioh = &volume->ioh;
		if ((ioh->ioflags & IOHF_MEDIA_CHANGE) && (!volume->inhibitcounter))
		{
			D(bug("[afs 0x%08lX] Media change signalled\n", volume));
			if (diskPresent(afsbase, ioh))
			{
			    if (!volume->inhibitcounter)
			    {
				D(bug("[afs 0x%08lX] Media inserted\n", volume));
				newMedium(afsbase, volume);
			    }
			    ioh->ioflags |= IOHF_DISK_IN;
			}
			else
			{
			    if (!volume->inhibitcounter)
			    {
				flush(afsbase, volume);
				remDosVolume(afsbase, volume);
			    }
			    ioh->ioflags &= ~IOHF_DISK_IN;
			}
			ioh->ioflags &= ~IOHF_MEDIA_CHANGE;
		}
		volume = (struct Volume *)volume->ln.ln_Succ;
	}
}

void check64BitSupport(struct AFSBase *afsbase, struct Volume *volume) {
struct NSDeviceQueryResult nsdq;
UWORD *cmdcheck;

	if (
			(
				(volume->startblock+(volume->countblocks))*  /* last block */
				(volume->SizeBlock*4/512)	/* 1 portion (block) equals 512 (bytes) */
			)>8388608)
	{
		nsdq.SizeAvailable = 0;
		nsdq.DevQueryFormat = 0;
		volume->ioh.ioreq->iotd_Req.io_Command = NSCMD_DEVICEQUERY;
		volume->ioh.ioreq->iotd_Req.io_Data = &nsdq;
		volume->ioh.ioreq->iotd_Req.io_Length = sizeof(struct NSDeviceQueryResult);
		if (DoIO((struct IORequest *)volume->ioh.ioreq) == IOERR_NOCMD)
		{
			D(bug("[afs] initVolume-NSD: device doesn't understand NSD-Query\n"));
		}
		else
		{
			if (
					(volume->ioh.ioreq->iotd_Req.io_Actual > sizeof(struct NSDeviceQueryResult)) ||
					(volume->ioh.ioreq->iotd_Req.io_Actual == 0) ||
					(volume->ioh.ioreq->iotd_Req.io_Actual != nsdq.SizeAvailable)
				)
			{
				D(bug("[afs] initVolume-NSD: WARNING wrong io_Actual using NSD\n"));
			}
			else
			{
				D(bug("[afs] initVolume-NSD: using NSD commands\n"));
				if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
					D(bug("[afs] initVolume-NSD: WARNING no trackdisk type\n"));
				for (cmdcheck=nsdq.SupportedCommands; *cmdcheck; cmdcheck++)
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
						(volume->ioh.cmdread != NSCMD_TD_READ64) ||
						(volume->ioh.cmdwrite != NSCMD_TD_WRITE64)
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
 Input : volume  - volume to flush
 Output: DOSTRUE
********************************************/
BOOL flush(struct AFSBase *afsbase, struct Volume *volume) {

	flushCache(afsbase, volume);
	volume->ioh.ioreq->iotd_Req.io_Command = CMD_UPDATE;
	DoIO((struct IORequest *)&volume->ioh.ioreq->iotd_Req);
	clearCache(afsbase, volume->blockcache);
	return DOSTRUE;
}

static ULONG readwriteDisk
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG start,
		ULONG count,
		APTR mem,
		ULONG cmd
	)
{
LONG retval;
struct IOHandle *ioh = &volume->ioh;
UQUAD offset;

	if (start + count <= volume->countblocks)
	{
		ioh->ioreq->iotd_Req.io_Command = cmd;
		ioh->ioreq->iotd_Req.io_Length = count*BLOCK_SIZE(volume);
		ioh->ioreq->iotd_Req.io_Data = mem;

		offset  = (UQUAD)volume->startblock * volume->sectorsize
			+ (UQUAD)start * BLOCK_SIZE(volume);

		ioh->ioreq->iotd_Req.io_Offset = 0xFFFFFFFF & offset;
		ioh->ioreq->iotd_Req.io_Actual = offset>>32;
		retval = DoIO((struct IORequest *)&ioh->ioreq->iotd_Req);
		ioh->flags |= IOHF_MOTOR_OFF;
	}
	else
	{
		showText(afsbase, "Attempted to read/write block outside range!\n\n"
			"range: %lu-%lu, start: %lu, count: %lu",
			volume->startblock, volume->lastblock,
			volume->startblock + start * volume->blocksectors,
			count * volume->blocksectors);
		retval = 1;
	}
	return retval;
}

LONG readDisk(struct AFSBase *afsbase, struct Volume *volume, ULONG start, ULONG count, APTR data) {
LONG result = 0;
BOOL retry = TRUE;

	while (retry)
	{
		DB2(bug("[afs]    readDisk: reading blocks %lu to %lu\n", start, start+count-1));
		result = readwriteDisk(afsbase, volume, start, count, data, volume->ioh.cmdread);
		if (result == 0)
			retry = FALSE;
		else
			retry = showRetriableError(afsbase, "Read error %ld on block %lu", result, start) == 1;
	}

	return result;
}

LONG writeDisk(struct AFSBase *afsbase, struct Volume *volume, ULONG start, ULONG count, APTR data) {
LONG result = 0;
BOOL retry = TRUE;

	while (retry)
	{
		DB2(bug("[afs]    writeDisk: writing blocks %lu to %lu\n", start, start+count-1));
		result = readwriteDisk(afsbase, volume, start, count, data, volume->ioh.cmdwrite);
		if (result == 0)
			retry = FALSE;
		else
			retry = showRetriableError(afsbase, "Write error %ld on block %lu", result, start) == 1;
	}

	return result;
}

#undef SysBase

AROS_UFIH1(changeIntCode, struct IOHandle *, ioh)
{
	AROS_USERFUNC_INIT

	ioh->ioflags |= IOHF_MEDIA_CHANGE;
	Signal(ioh->afsbase->port.mp_SigTask, 1<<ioh->afsbase->port.mp_SigBit);

	return FALSE;

	AROS_USERFUNC_EXIT
}
