#ifndef VOLUMES_H
#define VOLUMES_H

#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include "filehandles.h"
#include "blockaccess.h"
#include "afshandler.h"

struct Volume {
	struct Device *device;		// the handler this volume uses
	struct DeviceList devicelist;
	ULONG SizeBlock;				// Block size in words
	
	struct AfsHandle *locklist;
	struct AfsHandle ah;	// root handle (unfilled except header_block)
	char *blockdevice;
	ULONG unit;
	struct MsgPort *ioport;
	struct IOExtTD *iorequest;
	struct BlockCache *blockcache;
	UWORD istrackdisk;			// do we use trackdisk.device ? if yes then we have to use some extra TD commands
	UWORD cmdread;
	UWORD cmdwrite;
	UWORD cmdseek;
	UWORD cmdformat;

	ULONG usedblockscount;
	ULONG rootblock;
	ULONG startblock;
	ULONG dostype;
	ULONG bitmapblockpointers[25];
	ULONG bitmapextensionblock;

	struct BlockCache *bitmapblock;	// last bitmap block used for marking
	ULONG bstartblock;						// first block marked in "bitmapblock"
	ULONG lastextensionblock;			// last used extensionblock (0=volume->bitmapblocks)
	ULONG lastposition;					// last position in extensionblock
	ULONG lastaccess;					// last marked block

	UWORD bootblocks;
	UBYTE moff_time;					/* time to wait until motor is turned off */
	UBYTE flags;
	struct Interrupt vbl_int;
};

struct Volume *initVolume(struct afsbase *, struct Device *, STRPTR, ULONG, struct DosEnvec *, ULONG *);
void uninitVolume(struct afsbase *, struct Volume *);
LONG newMedium(struct afsbase *, struct Volume *);
LONG addDosVolume(struct afsbase *, struct Volume *);
void remDosVolume(struct afsbase *, struct Volume *);
LONG initDeviceList(struct afsbase *, struct Volume *,struct BlockCache *);

#endif

