#ifndef VOLUMES_H
#define VOLUMES_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "filehandles.h"
#include "cache.h"

struct Volume {
	struct Node ln;
	struct Device *device;       /* the handler this volume uses */
	struct DeviceList devicelist __attribute__((aligned(4))); /* BPTR compatible */
	struct DosList *volumenode;
	ULONG SizeBlock;             /* Block size in words */
	ULONG blocksectors;          /* nr of sectors per block */
	ULONG sectorsize;            /* nr of bytes per sector */
	
	struct AfsHandle *locklist;
	struct AfsHandle ah;         /* root handle (unfilled except header_block) */
	ULONG unit;
	struct IOHandle ioh;
	struct BlockCache *blockcache;
	LONG numbuffers;
	ULONG cachecounter;           /* Keeps track of cache usage */
	ULONG state;                 /* Read-only, read/write or validating */
        ULONG key;                   /* Lock key */
	ULONG inhibitcounter;

	ULONG usedblockscount;       /* nr of used blocks */
	ULONG countblocks;           /* nr of blocks in filesystem */
	ULONG rootblock;             /* rootblock position */
	ULONG startblock;            /* absolute nr of start block on whole HD */
	ULONG lastblock;             /* absolute nr of last block on whole HD */
	ULONG dostype;
	ULONG bitmapblockpointers[25];
	ULONG bitmapextensionblock;

	struct BlockCache *bitmapblock; /* last bitmap block used for marking */
	ULONG bstartblock;              /* first block marked in "bitmapblock" */
	ULONG lastextensionblock; /* last used extensionblock (0=volume->bitmapblocks) */
	ULONG lastposition;             /* last position in extensionblock */
	ULONG lastaccess;               /* last marked block */

	UWORD bootblocks;
	UBYTE dosflags;
};

BOOL mediumPresent(struct IOHandle *);
struct Volume *initVolume(struct AFSBase *, struct Device *, CONST_STRPTR,
	ULONG, ULONG flags, struct DosEnvec *, LONG *error);
void uninitVolume(struct AFSBase *, struct Volume *);
LONG newMedium(struct AFSBase *, struct Volume *);
LONG writeprotectVolume(struct AFSBase *, struct Volume *, BOOL on, ULONG key);

#endif

