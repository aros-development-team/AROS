#ifndef _CACHEBUFFERS_PROTOS_H
#define _CACHEBUFFERS_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"
#include "cachebuffers.h"

LONG initcachebuffers(void);

LONG readcachebuffer(struct CacheBuffer **, BLCK);
LONG writecachebuffer(struct CacheBuffer *cb);

LONG readoriginalcachebuffer(struct CacheBuffer **returned_cb,BLCK blckno);
void emptyoriginalcachebuffer(BLCK blckno);

void lockcachebuffer(struct CacheBuffer *cb);
void unlockcachebuffer(struct CacheBuffer *cb);

void preparecachebuffer(struct CacheBuffer *cb);
LONG storecachebuffer(struct CacheBuffer *cb);
LONG storecachebuffer_nochecksum(struct CacheBuffer *cb);
struct CacheBuffer *newcachebuffer(BLCK blckno);
LONG changecachebuffer(struct CacheBuffer *cb, UBYTE *modifiedblocks);

struct CacheBuffer *findoriginalcachebuffer(BLCK blckno);
struct CacheBuffer *findlatestcachebuffer(BLCK blckno);

struct CacheBuffer *saveoriginalcachebuffer(struct CacheBuffer *cb);
struct CacheBuffer *createnewcachebuffer(BLCK block);

struct CacheBuffer *getcachebuffer(void);

void dumpcachebuffer(struct CacheBuffer *cb);
void emptycachebuffer(struct CacheBuffer *cb);
void clearcachebuffer(struct CacheBuffer *cb);
void resetcachebuffer(struct CacheBuffer *cb);

LONG addcachebuffers(LONG buffers);
void invalidatecachebuffers(void);

void dumpcachebuffers(void);

#endif // _CACHEBUFFERS_PROTOS_H
