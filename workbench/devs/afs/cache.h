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

struct BlockCache *initCache(struct afsbase *, struct Volume *volume, ULONG);
void freeCache(struct afsbase *, struct BlockCache *);
struct BlockCache *getFreeCacheBlock(struct afsbase *, struct Volume *, ULONG);
struct BlockCache *getBlock(struct afsbase *, struct Volume *, ULONG);
LONG writeBlock(struct afsbase *, struct Volume *, struct BlockCache *);
void flushCache(struct BlockCache *);
void checkCache(struct afsbase *, struct BlockCache *);

#endif
