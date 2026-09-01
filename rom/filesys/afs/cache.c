/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#undef DEBUG
#define DEBUG 0
#if defined(__AROS__)
#include <aros/debug.h>
#endif

#include "os.h"
#include "cache.h"
#include "checksums.h"
#include "error.h"
#include "afsblocks.h"
#include "baseredef.h"

#define CACHE_MAX_BUFFERS 1024
#define BULK_MAX_BYTES    65536

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

/* Mark a buffer as the most recently used */
static void markRecent(struct Volume *volume, struct BlockCache *bc)
{
struct BlockCache *cache;
        bc->newness = ++volume->cachecounter;
        /* Reset cache history if counter has overflowed */
        if (volume->cachecounter == 0)
        {
                for (cache = volume->blockcache; cache != NULL; cache = cache->next)
                        cache->newness = 0;
        }
}

struct BlockCache *getCacheBlock
        (struct AFSBase *afsbase, struct Volume *volume, ULONG blocknum)
{
struct BlockCache *cache;
struct BlockCache *bestcache=NULL;
BOOL found = FALSE;

        /* Check if block is already cached, or else reuse least-recently-used buffer */
        D(bug("[afs]    getCacheBlock: getting cacheblock %u\n",blocknum));
        cache = volume->blockcache;
        while ((cache != NULL) && !found)
        {
                if (cache->blocknum == blocknum)
                {
                        if (!(cache->flags & BCF_USED))
                        {
                                D(bug("[afs]    getCacheBlock: already cached (counter=%u)\n",
                                        cache->newness));
                                bestcache = cache;
                                found = TRUE;
                        }
                        else
                        {
                                if (blocknum != volume->rootblock)
                                {
                                        /*      should only occur while using setBitmap()
                                                ->that's ok (see setBitmap()) */
                                        D(bug("Concurrent access on block %u!\n",blocknum));
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

                markRecent(volume, bestcache);
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

/* Least recently used buffer that is free to take, or NULL if the block is cached already */
static struct BlockCache *readAheadSlot(struct Volume *volume, ULONG blocknum)
{
struct BlockCache *cache;
struct BlockCache *best = NULL;
        for (cache = volume->blockcache; cache != NULL; cache = cache->next)
        {
                if (cache->blocknum == blocknum)
                        return NULL;
                if ((cache->flags & (BCF_USED | BCF_WRITE)) == 0
                        && (best == NULL || cache->newness < best->newness))
                        best = cache;
        }
        return best;
}

/*
 * Fill the missed block and, from the same request, the blocks following it
 * into free buffers. Returns TRUE if the block was filled.
 */
static BOOL readAhead(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *first)
{
struct BlockCache *bc;
ULONG count, i;
char *src;
        if (first->blocknum >= volume->countblocks)
                return FALSE;
        count = (ULONG)volume->numbuffers / 4;
        if (count > volume->bulkblocks)
                count = volume->bulkblocks;
        if (first->blocknum + count > volume->countblocks)
                count = volume->countblocks - first->blocknum;
        if (count < 2)
                return FALSE;
        if (readDisk(afsbase, volume, first->blocknum, count, volume->bulkbuffer) != 0)
                return FALSE;
        src = (char *)volume->bulkbuffer;
        CopyMem(src, first->buffer, BLOCK_SIZE(volume));
        first->flags |= BCF_USED;
        for (i = 1; i < count; i++)
        {
                src += BLOCK_SIZE(volume);
                bc = readAheadSlot(volume, first->blocknum + i);
                if (bc == NULL)
                        continue;
                bc->blocknum = first->blocknum + i;
                CopyMem(src, bc->buffer, BLOCK_SIZE(volume));
                markRecent(volume, bc);
        }
        first->flags &= ~BCF_USED;
        markRecent(volume, first);
        return TRUE;
}

/* Shorten a run of blocks so that it stops before any block with unwritten changes */
ULONG cleanRun(struct Volume *volume, ULONG start, ULONG count)
{
struct BlockCache *cache;
        for (cache = volume->blockcache; cache != NULL; cache = cache->next)
        {
                if ((cache->flags & BCF_WRITE) != 0
                        && cache->blocknum >= start && cache->blocknum - start < count)
                        count = cache->blocknum - start;
        }
        return count;
}

/*
 * Number of cache buffers for a volume. The DosEnvec value is the floor; where
 * memory allows, the cache grows with it, except on m68k where memory is scarce.
 */
LONG cacheBuffers(struct Volume *volume, LONG requested)
{
#if defined(__AROS__) && !defined(__mc68000__)
        IPTR want = AvailMem(MEMF_ANY) / ((IPTR)BLOCK_SIZE(volume) * 2048);
        if (want > CACHE_MAX_BUFFERS)
                want = CACHE_MAX_BUFFERS;
        if ((LONG)want > requested)
                requested = (LONG)want;
#endif
        if (requested < 1)
                requested = 1;
        return requested;
}

BOOL initBulkBuffer(struct AFSBase *afsbase, struct Volume *volume)
{
ULONG blocks = BULK_MAX_BYTES / BLOCK_SIZE(volume);
        if (blocks > (ULONG)volume->numbuffers)
                blocks = volume->numbuffers;
        if (blocks < 2)
                blocks = 2;
        volume->bulkbuffer = AllocVec((IPTR)blocks * BLOCK_SIZE(volume), MEMF_PUBLIC);
        volume->bulkblocks = volume->bulkbuffer != NULL ? blocks : 0;
        return volume->bulkbuffer != NULL;
}

void freeBulkBuffer(struct AFSBase *afsbase, struct Volume *volume)
{
        FreeVec(volume->bulkbuffer);
        volume->bulkbuffer = NULL;
        volume->bulkblocks = 0;
}

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
                        if (!readAhead(afsbase, volume, blockbuffer)
                                && readDisk(afsbase, volume, blocknum, 1, blockbuffer->buffer) != 0)
                        {
                                /* don't leave an unreadable block looking cached */
                                blockbuffer->blocknum = 0;
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

