/* 
   $Id$
*/

#include <stdio.h>

#define DEBUG 1

#include <proto/exec.h>

#include <exec/memory.h>
#include <exec/io.h>
#include <devices/trackdisk.h>

#include <aros/debug.h>
#include <aros/macros.h>

#include "blockaccess.h"
#include "error.h"
#include "afsblocks.h"
#include "afshandler.h"
#include "baseredef.h"

struct BlockCache *initCache(struct afsbase *afsbase, struct Volume *volume, ULONG numBuffers) {
struct BlockCache *head;
struct BlockCache *cache;
ULONG i;

	if ((head=AllocVec(numBuffers*(sizeof(struct BlockCache)+BLOCK_SIZE(volume)),MEMF_PUBLIC | MEMF_CLEAR))) {
		cache=head;
		for (i=0;i<(numBuffers-1);i++) {
			cache->buffer=(ULONG *)((ULONG)cache+sizeof(struct BlockCache));
			cache->next=(struct BlockCache *)((ULONG)cache->buffer+BLOCK_SIZE(volume));
			cache=cache->next;
		}
		cache->buffer=(ULONG *)((ULONG)cache+sizeof(struct BlockCache));
		cache->next=0;
	}
	D(bug("initCache: my Mem is %lx size %lx\n",head,numBuffers*(sizeof(struct BlockCache)+BLOCK_SIZE(volume))));
	return head;
}

void freeCache(struct afsbase *afsbase, struct BlockCache *cache) {
	if (cache) FreeVec(cache);
}

void flushCache(struct BlockCache *cache) {

	while (cache) {
		cache->volume=0;
		cache->blocknum=0;
		cache->acc_count=0;
		//if (cache->flags & BCF_WRITE) writeBlock(...)
		cache->flags=0;
		cache=cache->next;
	}
}

/********************* for trackdisk reading ***********************/
void sendDeviceCmd(struct afsbase *afsbase, struct Volume *volume, UWORD command) {

	volume->iorequest->iotd_Req.io_Command=command;
	volume->iorequest->iotd_Req.io_Length=0;
	DoIO((struct IORequest *)&volume->iorequest->iotd_Req);
}
/*******************************************************************/

ULONG readDisk(struct afsbase *afsbase, struct Volume *volume, ULONG start, ULONG count, APTR mem, ULONG cmd) {
ULONG retval;

	D(bug("afs.handler:    readDisk: reading block %ld\n",start));
	volume->iorequest->iotd_Req.io_Command=cmd;
	volume->iorequest->iotd_Req.io_Length=count*BLOCK_SIZE(volume);
	volume->iorequest->iotd_Req.io_Data=mem;
	volume->iorequest->iotd_Req.io_Offset=(start+volume->startblock)*BLOCK_SIZE(volume);	// bootblock is first disk on device and we always get offsets from start
	retval=DoIO((struct IORequest *)&volume->iorequest->iotd_Req);
	if (retval)
		showError(afsbase, ERR_READWRITE);
	if (volume->istrackdisk)
		sendDeviceCmd(afsbase, volume,ETD_MOTOR);
	return retval;
}

struct BlockCache *getFreeCacheBlock(struct afsbase *afsbase, struct Volume *volume, ULONG blocknum) {
struct BlockCache *cache;
struct BlockCache *smallest=0;

	D(bug("afs.handler:    getFreeCacheBlock: getting cacheblock %ld\n",blocknum));
	cache=volume->blockcache;
	while (cache)
	{
		if (cache->blocknum==blocknum)
		{
			if (!(cache->flags & BCF_USED))
			{
				D(bug("afs.handler:    getFreeCacheBlock: already cached %ld\n",cache->acc_count));
				cache->acc_count += 1;
				return cache;
			}
			else
			{
				if (blocknum!=volume->rootblock) {		// should only occur while using setBitmap() and changing root ->thats ok (see setBitmap())
					D(bug("Concurrent access on block %ld!\n",blocknum));
				}
			}
		}
		if (!(cache->flags & BCF_USED))	// its not ok - I know, but for test purposes I want to know if there may be more concurrent blockaccesses
		{
			if (smallest)
			{
				if (smallest->acc_count>cache->acc_count)
					smallest=cache;
			}
			else
			{
				smallest=cache;
			}
		}
		cache=cache->next;
	}
	// block not cached
	if (smallest) {
		smallest->acc_count=1;
		smallest->blocknum=blocknum;
		smallest->volume=volume;
	}
	else
		showText(afsbase, "No free cache available!?\nAdd more cache!!!");
	return smallest;
}

#ifdef DEBUG
void umpBlock(struct afsbase *afsbase, struct BlockCache *block) {
UWORD i,j;

	for (i=0;i<=31;i++) {
		D(bug("0x%lx: ",i*16));
		for (j=0;j<=3;j++)
			D(bug(" %lx",AROS_BE2LONG(block->buffer[i*4+j])));
		D(bug("\n"));
	}
}
#endif

struct BlockCache *getBlock(struct afsbase *afsbase, struct Volume *volume, ULONG blocknum) {
struct BlockCache *blockbuffer;

	if ((blockbuffer=getFreeCacheBlock(afsbase, volume, blocknum))) {
		if (blockbuffer->acc_count==1)		//do we have to read that block ?
			if (readDisk(afsbase, volume,blocknum,1,blockbuffer->buffer,CMD_READ))
				blockbuffer=0;
	}
	return blockbuffer;
}

LONG writeBlock(struct afsbase *afsbase, struct Volume *volume, struct BlockCache *blockbuffer) {

	readDisk(afsbase, volume, blockbuffer->blocknum,1,blockbuffer->buffer,CMD_WRITE);
	return DOSTRUE;
}
