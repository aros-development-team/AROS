#include <proto/dos.h>

#include <dos/dos.h>
#include <aros/macros.h>

#include "misc.h"
#include "afsblocks.h"
#include "blockaccess.h"
#include "checksums.h"

ULONG writeHeader(struct Volume *volume, struct BlockCache *blockbuffer) {
struct DateStamp ds;

	DateStamp(&ds);
	for (;;) {
		blockbuffer->buffer[BLK_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
		blockbuffer->buffer[BLK_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
		blockbuffer->buffer[BLK_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
		blockbuffer->buffer[BLK_CHECKSUM]=0;
		if (!blockbuffer->buffer[BLK_PARENT(volume)])
			break;
		blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume, blockbuffer->buffer));
		writeBlock(volume,blockbuffer);
		blockbuffer=getBlock(volume, AROS_LONG2BE(blockbuffer->buffer[BLK_PARENT(volume)]));
		if (!blockbuffer)
			return ERROR_UNKNOWN;
	}
	// last block is not written->its the rootblock
	// we have to change BLK_VOLUME_xxx
	blockbuffer->buffer[BLK_VOLUME_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
	blockbuffer->buffer[BLK_VOLUME_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
	blockbuffer->buffer[BLK_VOLUME_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
	blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume, blockbuffer->buffer));
	writeBlock(volume,blockbuffer);
	return 0;
}
