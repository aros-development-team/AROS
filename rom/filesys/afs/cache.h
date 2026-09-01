#ifndef BLOCKACCESS_H
#define BLOCKACCESS_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "volumes.h"

struct BlockCache {
	struct BlockCache *next;
	ULONG newness;
	ULONG blocknum;         /* zero means block is empty */
	ULONG *buffer;
	ULONG flags;
};

#define BCF_USED 1
#define BCF_WRITE 2

struct BlockCache *initCache(struct AFSBase *, struct Volume *volume, ULONG);
void freeCache(struct AFSBase *, struct BlockCache *);
struct BlockCache *getFreeCacheBlock(struct AFSBase *, struct Volume *, ULONG);
struct BlockCache *getBlock(struct AFSBase *, struct Volume *, ULONG);
ULONG cleanRun(struct Volume *, ULONG, ULONG);
BOOL initBulkBuffer(struct AFSBase *, struct Volume *);
void freeBulkBuffer(struct AFSBase *, struct Volume *);
LONG writeBlock(struct AFSBase *, struct Volume *, struct BlockCache *, LONG);
VOID writeBlockDeferred(struct AFSBase *, struct Volume *, struct BlockCache *, LONG);
void clearCache(struct AFSBase *, struct BlockCache *);
VOID flushCache(struct AFSBase *, struct Volume *);
void checkCache(struct AFSBase *, struct Volume *);

#endif
