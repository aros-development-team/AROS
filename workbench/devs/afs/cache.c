/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include "os.h"
#include "cache.h"
#include "checksums.h"
#include "error.h"
#include "afsblocks.h"
#include "baseredef.h"

/********************************************************
 Name  : initCache
 Descr.: initializes block cache for a volume
 Input : volume  - the volume to initializes cache for
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
		cache->next = NULL;
	}
	D(bug
		(
			"initCache: my Mem is 0x%p size 0x%lx\n",
			head,
			numBuffers*(sizeof(struct BlockCache)+BLOCK_SIZE(volume))
		));
	return head;
}

void freeCache(struct AFSBase *afsbase, struct BlockCache *cache) {
	if (cache != NULL)
		FreeVec(cache);
}

void clearCache(struct AFSBase *afsbase, struct BlockCache *cache) {

	while (cache != NULL)
	{
		if ((cache->flags & BCF_WRITE) == 0)
		{
			cache->blocknum = 0;
			cache->newness = 0;
			cache->flags = 0;
		}
		else
			showText(afsbase, "You MUST re-insert ejected volume");
		cache = cache->next;
	}
}

VOID flushCache
	(struct AFSBase *afsbase, struct Volume *volume)
{
struct BlockCache *block;

	for (block = volume->blockcache; block != NULL; block = block->next)
	{
		if ((block->flags & (BCF_WRITE | BCF_USED)) == BCF_WRITE)
		{
			writeDisk(afsbase, volume, block->blocknum, 1, block->buffer);
			block->flags &= ~BCF_WRITE;
		}
	}
}

struct BlockCache *getCacheBlock
	(struct AFSBase *afsbase, struct Volume *volume, ULONG blocknum)
{
struct BlockCache *cache;
struct BlockCache *bestcache=NULL;
BOOL found = FALSE;

	/* Check if block is already cached, or else reuse least-recently-used buffer */
	D(bug("[afs]    getCacheBlock: getting cacheblock %lu\n",blocknum));
	cache = volume->blockcache;
	while ((cache != NULL) && !found)
	{
		if (cache->blocknum == blocknum)
		{
			if (!(cache->flags & BCF_USED))
			{
				D(bug("[afs]    getCacheBlock: already cached (counter=%lu)\n",
					cache->newness));
				bestcache = cache;
				found = TRUE;
			}
			else
			{
				if (blocknum != volume->rootblock)
				{
					/*	should only occur while using setBitmap()
						->that's ok (see setBitmap()) */
					D(bug("Concurrent access on block %lu!\n",blocknum));
				}
				else
				{
					bestcache = cache;
					found = TRUE;
				}
			}
		}
		else if ((cache->flags & (BCF_USED | BCF_WRITE)) == 0)
		{
			if (bestcache != NULL)
			{
				if (bestcache->newness > cache->newness)
					bestcache = cache;
			}
			else
			{
				bestcache = cache;
			}
		}
		cache = cache->next;
	}

	if (bestcache != NULL)
	{
		if (!found)
			bestcache->blocknum = 0;

		/* Mark buffer as the most recently used */
		bestcache->newness = ++volume->cachecounter;

		/* Reset cache history if counter has overflowed */
		if (volume->cachecounter == 0)
		{
			for (cache = volume->blockcache; cache != NULL; cache = cache->next)
				cache->newness = 0;
		}
	}
	else
	{
		/* We should only run out of cache blocks if blocks need to be
		   written, so write them and try again */
		flushCache(afsbase, volume);
		bestcache = getCacheBlock(afsbase, volume, blocknum);
		if (bestcache == NULL)
			showText(afsbase, "Oh, ohhhhh, where is all the cache gone? BUG!!!");
	}

	return bestcache;
}

/***************************************************************************
 Name  : getFreeCacheBlock
 Descr.: Get a cache block to fill. The returned cache block's buffer will
         have arbitrary contents. However, to ensure cache integrity, an
         existing cache block for the specified block will be returned if
         present.
 Input : volume  - the volume the block is on.
         blocknum - the block number the cache block will be used for.
 Output: an unfilled cache block for the specified block.
***************************************************************************/
struct BlockCache *getFreeCacheBlock
	(struct AFSBase *afsbase, struct Volume *volume, ULONG blocknum)
{
struct BlockCache *cache;

	cache = getCacheBlock(afsbase, volume, blocknum);
	cache->blocknum = blocknum;
	cache->newness = 0;
	return cache;
}

void checkCache(struct AFSBase *afsbase, struct Volume *volume) {
struct BlockCache *bc;

	bc = volume->blockcache;
	while (bc != NULL)
	{
		if (((bc->flags & BCF_USED) != 0) && (bc->blocknum != volume->rootblock))
		{
			showText(afsbase, "Unreleased block: %lu!", bc->blocknum);
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

	blockbuffer = getCacheBlock(afsbase, volume, blocknum);
	if (blockbuffer != NULL)
	{
		if (blockbuffer->blocknum == 0)
		{
			blockbuffer->blocknum = blocknum;
			if (readDisk(afsbase, volume, blocknum, 1, blockbuffer->buffer) != 0)
			{
				blockbuffer = NULL;
			}
		}
	}
	D(bug("[afs]    getBlock: using cache block with address 0x%p\n", blockbuffer));
	return blockbuffer;
}

LONG writeBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *blockbuffer,
		LONG checksumoffset
	)
{
	/* Update checksum if requested by caller */
	if(checksumoffset != -1)
	{
		blockbuffer->buffer[checksumoffset] = 0;
		blockbuffer->buffer[checksumoffset] =
			OS_LONG2BE(0 - calcChkSum(volume->SizeBlock,blockbuffer->buffer));
	}

	/* Ensure bitmap isn't marked valid while there are dirty blocks in the cache */
	if (blockbuffer->blocknum == volume->rootblock)
		flushCache(afsbase, volume);

	/* Write block to disk */
	writeDisk(afsbase, volume, blockbuffer->blocknum, 1, blockbuffer->buffer);
	blockbuffer->flags &= ~BCF_WRITE;
	return DOSTRUE;
}

VOID writeBlockDeferred
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *blockbuffer,
		LONG checksumoffset
	)
{
	/* Update checksum if requested by caller */
	if(checksumoffset != -1)
	{
		blockbuffer->buffer[checksumoffset] = 0;
		blockbuffer->buffer[checksumoffset] =
			OS_LONG2BE(0 - calcChkSum(volume->SizeBlock,blockbuffer->buffer));
	}

	/* Mark block as needing to be written when the time comes */
	blockbuffer->flags |= BCF_WRITE;
	return;
}

