#ifndef VOLUMES_H
#define VOLUMES_H

#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include "filehandles.h"
#include "blockaccess.h"

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
	ULONG usedblockscount;
	ULONG rootblock;
	ULONG startblock;
	ULONG dostype;
	ULONG bitmapblockpointers[25];
	ULONG bitmapextensionblock;
	UWORD bootblocks;
	UBYTE flags;
};

struct Volume *initVolume(struct Device *, STRPTR, ULONG, struct DosEnvec *);
void uninitVolume(struct Volume *);
void newMedium(struct Volume *);
LONG addDosVolume(struct Volume *);
void remDosVolume(struct Volume *);
LONG initDeviceList(struct Volume *,struct BlockCache *);

#endif

