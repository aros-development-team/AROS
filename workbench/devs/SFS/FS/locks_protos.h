#ifndef _LOCKS_PROTOS_H
#define _LOCKS_PROTOS_H

#include <exec/types.h>
#include "cachebuffers.h"
#include "locks.h"
#include "nodes.h"
#include "objects.h"

LONG locateobject(UBYTE *path,struct CacheBuffer **io_cb,struct fsObject **io_o);
LONG locateobject2(UBYTE **io_path,struct CacheBuffer **io_cb,struct fsObject **io_o);
LONG locateobjectfromlock(struct ExtFileLock *lock,UBYTE *path,struct CacheBuffer **returned_cb,struct fsObject **returned_o);
LONG locatelockableobject(struct ExtFileLock *lock,UBYTE *path,struct CacheBuffer **returned_cb,struct fsObject **returned_o);
LONG lockobject(struct ExtFileLock *,UBYTE *,LONG,struct ExtFileLock **);
LONG lockobject2(struct fsObject *o, LONG accessmode, struct ExtFileLock **returned_efl);
LONG freelock(struct ExtFileLock *lock);
LONG lockable(NODE,LONG);
void settemporarylock(NODE objectnode);
void cleartemporarylock(void);
void updatelocksaftermove(BLCK source, BLCK dest, ULONG blocks);

LONG createglobalhandle(struct ExtFileLock *efl);
struct GlobalHandle *findglobalhandle(NODE objectnode);

#endif // _LOCKS_PROTOS_H
