/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include "os.h"
#include "bitmap.h"
#include "error.h"
#include "afsblocks.h"
#include "baseredef.h"

LONG mediumPresent(struct IOHandle *ioh) {
	return (ioh->ioflags & IOHF_DISK_IN)==IOHF_DISK_IN;
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
LONG error;

	blockbuffer=getBlock(afsbase, volume,0);
	if (blockbuffer == NULL)
		return ERROR_UNKNOWN;
	volume->dostype=OS_BE2LONG(blockbuffer->buffer[0]) & 0xFFFFFF00;
	if (volume->dostype != 0x444F5300)
	{
		blockbuffer=getBlock(afsbase, volume, 1);
		volume->dostype=OS_BE2LONG(blockbuffer->buffer[0]) & 0xFFFFFF00;
	}
	volume->dosflags |= OS_BE2LONG(blockbuffer->buffer[0]) & 0xFF;
	if (volume->dostype != 0x444F5300)
		return ERROR_NOT_A_DOS_DISK;
	blockbuffer=getBlock(afsbase, volume,volume->rootblock);
	if (blockbuffer == NULL)
		return ERROR_UNKNOWN;
	if (OS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]) != ST_ROOT)
		return ERROR_NOT_A_DOS_DISK;
	for (i=0;i<=24;i++)
	{
		volume->bitmapblockpointers[i]=OS_BE2LONG
			(
				blockbuffer->buffer[BLK_BITMAP_POINTERS_START(volume)+i]
			);
	}
	volume->bitmapextensionblock=OS_BE2LONG
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
	error = osMediumInit(afsbase, volume, blockbuffer);
	if (error != 0)
		return error;
	/* for free block searching */
	volume->lastaccess=volume->rootblock;
	return 0;
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
	if (volume != NULL)
	{
		volume->device = device;
		volume->ioh.blockdevice = blockdevice;
		volume->ioh.unit = unit;
		volume->ioh.flags = 0;
		volume->SizeBlock=devicedef->de_SizeBlock;
		if (devicedef->de_TableSize>=20)
			volume->bootblocks=devicedef->de_BootBlocks;
		else
			volume->bootblocks=devicedef->de_Reserved;
		volume->blockcache=initCache(afsbase, volume, devicedef->de_NumBuffers);
		if (volume->blockcache)
		{
			if (openBlockDevice(afsbase, &volume->ioh)!= NULL)
			{
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
				check64BitSupport(afsbase, volume);
				volume->ah.volume=volume;
				if (mediumPresent(&volume->ioh))
				{
					*error = newMedium(afsbase, volume);
				}
				else
					*error = 0;
				if ((*error == 0) || (*error == ERROR_NOT_A_DOS_DISK))
				{
					D(bug("afs.handler: initVolume: BootBlocks=%d\n",volume->bootblocks));
					D(bug("afs.handler: initVolume: RootBlock=%ld\n",volume->rootblock));
					volume->ah.header_block = volume->rootblock;
					return volume;
				}
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

	osMediumFree(afsbase, volume, TRUE);
	if (volume->blockcache)
		freeCache(afsbase, volume->blockcache);
	closeBlockDevice(afsbase, &volume->ioh);
	FreeMem(volume,sizeof(struct Volume));
}

