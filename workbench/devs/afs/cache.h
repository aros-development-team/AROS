#ifndef BLOCKACCESS_H
#define BLOCKACCESS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "volumes.h"

struct BlockCache {
	struct BlockCache *next;
	ULONG acc_count;
	struct Volume *volume;
	ULONG blocknum;
	ULONG *buffer;
	ULONG flags;
};

#define BCF_USED 1
#define BCF_WRITE 2

struct BlockCache *initCache(struct AFSBase *, struct Volume *volume, ULONG);
void freeCache(struct AFSBase *, struct BlockCache *);
struct BlockCache *getFreeCacheBlock(struct AFSBase *, struct Volume *, ULONG);
struct BlockCache *getBlock(struct AFSBase *, struct Volume *, ULONG);
LONG writeBlock(struct AFSBase *, struct Volume *, struct BlockCache *);
void flushCache(struct BlockCache *);
void checkCache(struct AFSBase *, struct BlockCache *);

#endif
