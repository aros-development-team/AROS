/*
   $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <stdio.h>

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

/********************************************************
 Name  : initCache
 Descr.: initialzes block cache for a volume
 Input : afsbase -
         volume  - the volume to initialzes cache for
         numBuffers - number of buffers for cache
 Output: first buffer (main cache pointer)
*********************************************************/
struct BlockCache *initCache
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		ULONG numBuffers
	)
{
struct BlockCache *head;
struct BlockCache *cache;
ULONG i;

	if (
			(
				head=AllocVec
					(
						numBuffers*
						(
							sizeof(struct BlockCache)+BLOCK_SIZE(volume)
						)
						,MEMF_PUBLIC | MEMF_CLEAR
					)
			)
		)
	{
		cache=head;
		for (i=0;i<(numBuffers-1);i++) {
			cache->buffer=(ULONG *)((ULONG)cache+sizeof(struct BlockCache));
			cache->next=
				(struct BlockCache *)((ULONG)cache->buffer+BLOCK_SIZE(volume));
			cache=cache->next;
		}
		cache->buffer=(ULONG *)((ULONG)cache+sizeof(struct BlockCache));
		cache->next=0;
	}
	D(bug
		(
			"initCache: my Mem is %lx size %lx\n",
			head,
			numBuffers*(sizeof(struct BlockCache)+BLOCK_SIZE(volume))
		));
	return head;
}

void freeCache(struct afsbase *afsbase, struct BlockCache *cache) {
	if (cache)
		FreeVec(cache);
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

ULONG readDisk
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		ULONG start,
		ULONG count,
		APTR mem,
		ULONG cmd
	)
{
ULONG retval;
UQUAD offset;

	D(bug("afs.handler:    readDisk: reading block %ld\n",start));
	volume->iorequest->iotd_Req.io_Command=cmd;
	volume->iorequest->iotd_Req.io_Length=count*BLOCK_SIZE(volume);
	volume->iorequest->iotd_Req.io_Data=mem;

	offset  = start+volume->startblock;
	offset *= BLOCK_SIZE(volume);

	volume->iorequest->iotd_Req.io_Offset=0xFFFFFFFF & offset;
	volume->iorequest->iotd_Req.io_Actual=offset>>32;
	retval=DoIO((struct IORequest *)&volume->iorequest->iotd_Req);
	if (retval)
		showError(afsbase, ERR_READWRITE, retval);
	if (volume->istrackdisk)
		sendDeviceCmd(afsbase, volume,TD_MOTOR);
	return retval;
}

struct BlockCache *getFreeCacheBlock
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		ULONG blocknum
	)
{
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
		if (!(cache->flags & BCF_USED))
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
		showText(afsbase, "Oh, ohhhhh, where is all the cache gone? BUG!!!");
	return smallest;
}

void checkCache(struct afsbase *afsbase, struct BlockCache *bc) {

	while (bc)
	{
		if (bc->flags & BCF_USED)
		{
			kprintf("not released block: %ld!\n", bc->blocknum);
			showText(afsbase, "not released block: %ld!", bc->blocknum);
		}
		bc = bc->next;
	}
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

	blockbuffer=getFreeCacheBlock(afsbase, volume, blocknum);
	if (blockbuffer)
	{
		if (blockbuffer->acc_count==1)
		{
			if (
					readDisk
						(
							afsbase,
							volume,
							blocknum,
							1,
							blockbuffer->buffer,
							volume->cmdread
						)
				)
			{
				blockbuffer=0;
			}
		}
	}
	return blockbuffer;
}

LONG writeBlock
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		struct BlockCache *blockbuffer
	)
{

	readDisk
		(
			afsbase,
			volume,
			blockbuffer->blocknum,
			1,
			blockbuffer->buffer,
			volume->cmdwrite
		);
	return DOSTRUE;
}
