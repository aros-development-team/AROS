/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <stdio.h>

#include "os.h"
#include "cache.h"
#include "error.h"
#include "afsblocks.h"
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
			"initCache: my Mem is %p size %lx\n",
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

struct BlockCache *getFreeCacheBlock
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		ULONG blocknum
	)
{
struct BlockCache *cache;
struct BlockCache *smallest=NULL;

	D(bug("afs.handler:    getFreeCacheBlock: getting cacheblock %ld\n",blocknum));
	cache=volume->blockcache;
	while (cache != NULL)
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
				if (blocknum!=volume->rootblock) {		// should only occur while using setBitmap() ->that's ok (see setBitmap())
					D(bug("Concurrent access on block %ld!\n",blocknum));
				}
			}
		}
		if (!(cache->flags & BCF_USED))
		{
			if (smallest != NULL)
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
	if (smallest != NULL)
	{
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
		D(bug("0x%x: ",i*16));
		for (j=0;j<=3;j++)
			D(bug(" %x",OS_BE2LONG(block->buffer[i*4+j])));
		D(bug("\n"));
	}
}
#endif

struct BlockCache *getBlock
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		ULONG blocknum
	)
{
struct BlockCache *blockbuffer;

	blockbuffer=getFreeCacheBlock(afsbase, volume, blocknum);
	if (blockbuffer != NULL)
	{
		if (blockbuffer->acc_count==1)
		{
			if (readDisk(afsbase, volume, blocknum, 1, blockbuffer->buffer))
			{
				blockbuffer=NULL;
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

	writeDisk(afsbase, volume, blockbuffer->blocknum, 1, blockbuffer->buffer);
	return DOSTRUE;
}

