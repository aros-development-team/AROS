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
 Input : volume  - the volume to initialzes cache for
         numBuffers - number of buffers for cache
 Output: first buffer (main cache pointer)
*********************************************************/
struct BlockCache *initCache
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG numBuffers
	)
{
struct BlockCache *head;
struct BlockCache *cache;
ULONG i;

	head = AllocVec
		(
			numBuffers*(sizeof(struct BlockCache)+BLOCK_SIZE(volume)),
			MEMF_PUBLIC | MEMF_CLEAR
		);
	if (head != NULL)
	{
		cache = head;
		for (i=0; i<(numBuffers-1); i++)
		{
			cache->buffer = (ULONG *)((char *)cache+sizeof(struct BlockCache));
			cache->next =
				(struct BlockCache *)((char *)cache->buffer+BLOCK_SIZE(volume));
			cache = cache->next;
		}
		cache->buffer = (ULONG *)((char *)cache+sizeof(struct BlockCache));
		cache->next = 0;
	}
	D(bug
		(
			"initCache: my Mem is %p size %lx\n",
			head,
			numBuffers*(sizeof(struct BlockCache)+BLOCK_SIZE(volume))
		));
	return head;
}

void freeCache(struct AFSBase *afsbase, struct BlockCache *cache) {
	if (cache != NULL)
		FreeVec(cache);
}

void flushCache(struct BlockCache *cache) {

	while (cache != NULL)
	{
		cache->volume = 0;
		cache->blocknum = 0;
		cache->acc_count = 0;
		//if (cache->flags & BCF_WRITE) writeBlock(...)
		cache->flags = 0;
		cache = cache->next;
	}
}

struct BlockCache *getFreeCacheBlock
	(struct AFSBase *afsbase, struct Volume *volume, ULONG blocknum)
{
struct BlockCache *cache;
struct BlockCache *smallest=NULL;

	D(bug("[afs]    getFreeCacheBlock: getting cacheblock %ld\n",blocknum));
	cache = volume->blockcache;
	while (cache != NULL)
	{
		if (cache->blocknum == blocknum)
		{
			if (!(cache->flags & BCF_USED))
			{
				D(bug("[afs]    getFreeCacheBlock: already cached %ld\n", cache->acc_count));
				cache->acc_count += 1;
				return cache;
			}
			else
			{
				if (blocknum != volume->rootblock)
				{
					/*	should only occur while using setBitmap()
						->that's ok (see setBitmap()) */
					D(bug("Concurrent access on block %ld!\n",blocknum));
				}
			}
		}
		if (!(cache->flags & BCF_USED))
		{
			if (smallest != NULL)
			{
				if (smallest->acc_count > cache->acc_count)
					smallest = cache;
			}
			else
			{
				smallest = cache;
			}
		}
		cache = cache->next;
	}
	/* block not cached */
	if (smallest != NULL)
	{
		smallest->acc_count = 1;
		smallest->blocknum = blocknum;
		smallest->volume = volume;
	}
	else
		showText(afsbase, "Oh, ohhhhh, where is all the cache gone? BUG!!!");
	return smallest;
}

void checkCache(struct AFSBase *afsbase, struct BlockCache *bc) {

	while (bc != NULL)
	{
		if (bc->flags & BCF_USED)
		{
			showText(afsbase, "not released block: %ld!", bc->blocknum);
		}
		bc = bc->next;
	}
}

#ifdef DEBUG
void umpBlock(struct AFSBase *afsbase, struct BlockCache *block) {
UWORD i,j;

	for (i=0; i<=31; i++) {
		D(bug("0x%x: ",i*16));
		for (j=0; j<=3; j++)
			D(bug(" %x", OS_BE2LONG(block->buffer[i*4+j])));
		D(bug("\n"));
	}
}
#endif

struct BlockCache *getBlock
	(struct AFSBase *afsbase, struct Volume *volume, ULONG blocknum)
{
struct BlockCache *blockbuffer;

	blockbuffer = getFreeCacheBlock(afsbase, volume, blocknum);
	if (blockbuffer != NULL)
	{
		if (blockbuffer->acc_count == 1)
		{
			if (readDisk(afsbase, volume, blocknum, 1, blockbuffer->buffer) != 0)
			{
				blockbuffer = NULL;
			}
		}
	}
	return blockbuffer;
}

LONG writeBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *blockbuffer
	)
{

	writeDisk(afsbase, volume, blockbuffer->blocknum, 1, blockbuffer->buffer);
	return DOSTRUE;
}

