#ifndef VOLUMES_H
#define VOLUMES_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "filehandles.h"
#include "cache.h"

struct Volume {
	struct Node ln;
	struct Device *device;		// the handler this volume uses
	struct DeviceList devicelist;
	ULONG SizeBlock;				// Block size in words
	
	struct AfsHandle *locklist;
	struct AfsHandle ah;	// root handle (unfilled except header_block)
	char *blockdevice;
	ULONG unit;
	struct IOHandle ioh;
	struct BlockCache *blockcache;

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
	UBYTE dosflags;
};

#define VOLF_TRACKDISK    (1 <<  8)

LONG mediumPresent(struct IOHandle *);
struct Volume *initVolume(struct afsbase *, struct Device *, STRPTR, ULONG, struct DosEnvec *, ULONG *);
void uninitVolume(struct afsbase *, struct Volume *);
LONG newMedium(struct afsbase *, struct Volume *);

#endif

