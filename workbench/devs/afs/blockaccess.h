#ifndef BLOCKACCESS_H
#define BLOCKACCESS_H

#include <exec/types.h>

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

struct BlockCache *initCache(struct Volume *volume, ULONG);
void freeCache(struct BlockCache *);
struct BlockCache *getFreeCacheBlock(struct Volume *, ULONG);
struct BlockCache *getBlock(struct Volume *, ULONG);
LONG writeBlock(struct Volume *, struct BlockCache *);
void flushCache(struct BlockCache *);
void sendDeviceCmd(struct Volume *volume, UWORD command);

#endif
