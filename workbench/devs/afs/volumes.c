#define DEBUG 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/trackdisk.h>

#include <string.h>
#include <aros/debug.h>
#include <aros/macros.h>

#include "volumes.h"
#include "bitmap.h"
#include "extstrings.h"
#include "error.h"
#include "afsblocks.h"
#include "baseredef.h"

/*******************************************
 Name  : initDeviceList
 Descr.: initializes a devicelist structure
 Input : devicelist - devicelist structure to initialize
         rootblock  - cacheblock of the rootblock
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG initDeviceList(struct afsbase *afsbase, struct Volume *volume,struct BlockCache *rootblock) {
char *name;

	name=(char *)((ULONG)rootblock->buffer+(BLK_DISKNAME_START(volume)*4));
	volume->devicelist.dl_Next=0;
	volume->devicelist.dl_Type=DLT_VOLUME;
	volume->devicelist.dl_Device=volume->device;
	volume->devicelist.dl_Lock=0;
	volume->devicelist.dl_VolumeDate.ds_Days=AROS_BE2LONG(rootblock->buffer[BLK_ROOT_DAYS(volume)]);
	volume->devicelist.dl_VolumeDate.ds_Minute=AROS_BE2LONG(rootblock->buffer[BLK_ROOT_MINS(volume)]);
	volume->devicelist.dl_VolumeDate.ds_Tick=AROS_BE2LONG(rootblock->buffer[BLK_ROOT_TICKS(volume)]);
	volume->devicelist.dl_LockList=0;
	volume->devicelist.dl_DiskType=volume->dostype;
	if (volume->devicelist.dl_OldName) {
		volume->devicelist.dl_OldName=(BSTR)BADDR(volume->devicelist.dl_OldName);
	}
	else {
		volume->devicelist.dl_OldName=(BSTR)AllocVec(32,MEMF_CLEAR | MEMF_PUBLIC);
		if (volume->devicelist.dl_OldName==0) return DOSFALSE;
	}
	CopyMem(name,(APTR)volume->devicelist.dl_OldName,name[0]+1);				//there is no NULL-BYTE!
	volume->devicelist.dl_OldName=MKBADDR(volume->devicelist.dl_OldName);
	return DOSTRUE;
}

/*******************************************
 Name  : addDosVolume
 Descr.: adds a new volume to dos
 Input : volume - volume to add
 Output: DOSTRUE for success; DOSFALSE otherwise
********************************************/
LONG addDosVolume(struct afsbase *afsbase, struct Volume *volume, LONG init) {
struct DosList *doslist, *dl=0;
char string[32];
char *bname;

	bname=BADDR(volume->devicelist.dl_OldName);
	CopyMem(bname+1,string,bname[0]);
	string[(LONG)bname[0]]=0;
	// do we have that volume ?
	if (!init)
	{
		if ((doslist=LockDosList(LDF_WRITE | LDF_VOLUMES))) {
			if ((dl=FindDosEntry(doslist,string,LDF_VOLUMES)))
				if (dl->dol_misc.dol_volume.dol_LockList) {
					volume->locklist=dl->dol_misc.dol_volume.dol_LockList;
				}
			UnLockDosList(LDF_WRITE | LDF_VOLUMES);
		}
	}
	// if not create a new doslist
	if (!dl)
	{
		if (!(doslist=MakeDosEntry(string,DLT_VOLUME)))
			return DOSFALSE;
		doslist->dol_Unit=(struct Unit *)&volume->ah;
		doslist->dol_Device=volume->device;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Days=volume->devicelist.dl_VolumeDate.ds_Days;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Minute=volume->devicelist.dl_VolumeDate.ds_Minute;
		doslist->dol_misc.dol_volume.dol_VolumeDate.ds_Tick=volume->devicelist.dl_VolumeDate.ds_Tick;
		AddDosEntry(doslist);
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

	bname=BADDR(volume->devicelist.dl_OldName);
	if (bname) {
		CopyMem(bname+1,string,bname[0]);
		string[(LONG)bname[0]]=0;
		if ((doslist=LockDosList(LDF_WRITE | LDF_VOLUMES))) {
			if ((dl=FindDosEntry(doslist,string,LDF_VOLUMES)))
				if (volume->locklist) {
					dl->dol_misc.dol_volume.dol_LockList=volume->locklist;
				}
				else {
					RemDosEntry(dl);
					FreeDosEntry(dl);
				}
			else
				showText(afsbase, "doslist not in chain");
			UnLockDosList(LDF_WRITE | LDF_VOLUMES);
		}
	}
}

LONG newMedium(struct afsbase *afsbase, struct Volume *volume) {
struct BlockCache *blockbuffer;
UWORD i;

#warning TODO disk in drive check
	if ((blockbuffer=getBlock(afsbase, volume,0))==0)
		return ERROR_UNKNOWN;
	volume->flags=AROS_BE2LONG(blockbuffer->buffer[0]) & 0xFF;
	volume->dostype=AROS_BE2LONG(blockbuffer->buffer[0]) & 0xFFFFFF00;
	if (volume->dostype!=0x444F5300) {
		return ERROR_NOT_A_DOS_DISK;
	}
	if ((blockbuffer=getBlock(afsbase, volume,volume->rootblock))==0)
		return ERROR_UNKNOWN;
	for (i=0;i<=24;i++)
		volume->bitmapblockpointers[i]=AROS_BE2LONG(blockbuffer->buffer[BLK_BITMAP_POINTERS_START(volume)+i]);
	volume->bitmapextensionblock=AROS_BE2LONG(blockbuffer->buffer[BLK_BITMAP_EXTENSION(volume)]);
	if (!blockbuffer->buffer[BLK_BITMAP_VALID_FLAG(volume)]) {
		volume->usedblockscount=0;
		showError(afsbase, ERR_DISKNOTVALID);
	}
	else
		volume->usedblockscount=countUsedBlocks(afsbase, volume);
	if (initDeviceList(afsbase, volume,blockbuffer)==0) {
		return ERROR_NO_FREE_STORE;
	}
	if (!(addDosVolume(afsbase, volume, 1))) {
		showError(afsbase, ERR_DOSENTRY);
		return ERROR_UNKNOWN;
	}
	volume->lastaccess=volume->rootblock;	//for free block searching
	return 0;
}

struct Volume *initVolume(struct afsbase *afsbase, struct Device *device, STRPTR blockdevice, ULONG unit, struct DosEnvec *devicedef, ULONG *error) {
struct Volume *volume;

	if (!(volume=AllocMem(sizeof(struct Volume),MEMF_PUBLIC | MEMF_CLEAR))) {
		*error=ERROR_NO_FREE_STORE;
		return 0;
	}
	volume->device=device;
	volume->blockdevice=blockdevice;
	volume->unit=unit;
	volume->SizeBlock=devicedef->de_SizeBlock;
	if (devicedef->de_TableSize>=20)
		volume->bootblocks=devicedef->de_BootBlocks;
	else
		volume->bootblocks=devicedef->de_Reserved;
	if (!(volume->blockcache=initCache(afsbase, volume, devicedef->de_NumBuffers))) {
		*error=ERROR_NO_FREE_STORE;
		FreeMem(volume,sizeof(struct Volume));
		return 0;
	}
	if (!(volume->ioport=CreateMsgPort())) {
		showError(afsbase, ERR_IOPORT);
		FreeMem(volume,sizeof(struct Volume));
		return 0;
	}
	if (!(volume->iorequest=(struct IOExtTD *)CreateIORequest(volume->ioport,sizeof(struct IOExtTD)))) {
		*error=ERROR_NO_FREE_STORE;
		FreeMem(volume,sizeof(struct Volume));
		return 0;
	}
	volume->istrackdisk=StrCmp(volume->blockdevice,"trackdisk.device");
	if (OpenDevice(volume->blockdevice,volume->unit,(struct IORequest *)&volume->iorequest->iotd_Req,0)!=0)
	{
		showError(afsbase, ERR_DEVICE);
		FreeMem(volume,sizeof(struct Volume));
		return 0;
	}
	volume->rootblock=
		(
			(
				(
					devicedef->de_HighCyl-devicedef->de_LowCyl+1
				)*devicedef->de_Surfaces*devicedef->de_BlocksPerTrack
			)-1+devicedef->de_Reserved
		)/2;	//root int the middle of a partition
	volume->startblock=
			devicedef->de_LowCyl*
			devicedef->de_Surfaces*
			devicedef->de_BlocksPerTrack;
	*error=newMedium(afsbase, volume);
	if (*error)
	{
		uninitVolume(afsbase, volume);
		volume=0;
	}
	else
	{
		D(bug("afs.handler: initVolume: BootBlocks=%ld\n",volume->bootblocks));
		D(bug("afs.handler: initVolume: RootBlock=%ld\n",volume->rootblock));
		volume->ah.header_block=volume->rootblock;
		volume->ah.volume=volume;
	}
	return volume;
}

void uninitVolume(struct afsbase *afsbase, struct Volume *volume) {

	remDosVolume(afsbase, volume);
	if (volume->blockcache)
		freeCache(afsbase, volume->blockcache);
	if (volume->devicelist.dl_OldName)
		FreeVec(BADDR(volume->devicelist.dl_OldName));
	if (volume->iorequest) {
		if (volume->iorequest->iotd_Req.io_Device) CloseDevice((struct IORequest *)&volume->iorequest->iotd_Req);
		DeleteIORequest((APTR)volume->iorequest);
	}
	if (volume->ioport)
		DeleteMsgPort(volume->ioport);
	FreeMem(volume,sizeof(struct Volume));
}
