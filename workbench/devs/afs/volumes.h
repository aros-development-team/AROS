#ifndef VOLUMES_H
#define VOLUMES_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include "filehandles.h"
#include "blockaccess.h"
#include "afshandler.h"

struct Volume {
	struct Node ln;
	struct Device *device;		// the handler this volume uses
	struct DeviceList devicelist;
	ULONG SizeBlock;				// Block size in words
	
	struct AfsHandle *locklist;
	struct AfsHandle ah;	// root handle (unfilled except header_block)
	char *blockdevice;
	ULONG unit;
	struct MsgPort *ioport;
	struct IOExtTD *iorequest;
	struct IOExtTD *iochangeint;
	struct BlockCache *blockcache;
	UWORD cmdread;
	UWORD cmdwrite;
	UWORD cmdseek;
	UWORD cmdformat;

	ULONG usedblockscount;
	ULONG rootblock;
	ULONG startblock;
	ULONG lastblock;
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
	UWORD flags;
	struct Interrupt vbl_int;
	struct Interrupt mc_int;
	struct afsbase *afsbase;		/* for interrupt code */
};

#define VOLF_TRACKDISK    (1 <<  8)
#define VOLF_MOTOR_OFF    (1 <<  9)
#define VOLF_MEDIA_CHANGE (1 << 10)
#define VOLF_DISK_IN      (1 << 11)

struct Volume *initVolume(struct afsbase *, struct Device *, STRPTR, ULONG, struct DosEnvec *, ULONG *);
void uninitVolume(struct afsbase *, struct Volume *);
LONG newMedium(struct afsbase *, struct Volume *);
LONG addDosVolume(struct afsbase *, struct Volume *);
void remDosVolume(struct afsbase *, struct Volume *);
LONG initDeviceList(struct afsbase *, struct Volume *,struct BlockCache *);
void checkDeviceFlags(struct afsbase *);

#endif

