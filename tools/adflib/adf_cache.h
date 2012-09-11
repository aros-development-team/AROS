#ifndef _ADF_CACHE_H
#define _ADF_CACHE_H 1
/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_cache.h
 *
 */


#include "adf_str.h"

void adfGetCacheEntry(struct bDirCacheBlock *dirc, int *p, struct CacheEntry *cEntry);
int adfPutCacheEntry( struct bDirCacheBlock *dirc, int *p, struct CacheEntry *cEntry);

struct List* adfGetDirEntCache(struct Volume *vol, SECTNUM dir, BOOL recurs);

RETCODE adfCreateEmptyCache(struct Volume *vol, struct bEntryBlock *parent, SECTNUM nSect);
RETCODE adfAddInCache(struct Volume *vol, struct bEntryBlock *parent, struct bEntryBlock *entry);
RETCODE adfUpdateCache(struct Volume *vol, struct bEntryBlock *parent, struct bEntryBlock *entry, BOOL);
RETCODE adfDelFromCache(struct Volume *vol, struct bEntryBlock *parent, SECTNUM);

RETCODE adfReadDirCBlock(struct Volume *vol, SECTNUM nSect, struct bDirCacheBlock *dirc);
RETCODE adfWriteDirCBlock(struct Volume*, ULONG, struct bDirCacheBlock* dirc);

#endif /* _ADF_CACHE_H */

/*##########################################################################*/
