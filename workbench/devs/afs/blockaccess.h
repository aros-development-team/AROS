#ifndef BLOCKACCESS_H
#define BLOCKACCESS_H

#include <exec/types.h>
#include <devices/trackdisk.h>

#include "volumes.h"
#include "afshandler.h"

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
void motorOff(struct afsbase *, struct Volume *);
UBYTE diskPresent(struct afsbase *, struct Volume *);
LONG getGeometry(struct afsbase *, struct Volume *, struct DriveGeometry *);
ULONG flush(struct afsbase *, struct Volume *);
LONG addChangeInt(struct afsbase *, struct Volume *);
void remChangeInt(struct afsbase *, struct Volume *);
void checkCache(struct afsbase *, struct BlockCache *);

#endif
