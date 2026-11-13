#ifndef _OBJECTS_PROTOS_H
#define _OBJECTS_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"
#include "cachebuffers.h"
#include "nodes.h"
#include "objects.h"
#include "locks.h"

LONG removeobject(struct CacheBuffer *cb, struct fsObject *o);
LONG bumpobject(struct CacheBuffer *cb, struct fsObject *o);
LONG renameobject(struct CacheBuffer *cb,struct fsObject *o,struct ExtFileLock *lock,UBYTE *path);
LONG renameobject2(struct CacheBuffer *cb, struct fsObject *o, struct CacheBuffer *cbparent, struct fsObject *oparent, UBYTE *newname, WORD sendnotify);
LONG deleteobject(struct ExtFileLock *lock, UBYTE *path, WORD sendnotify);
LONG scandir(struct CacheBuffer **io_cb, struct fsObject **io_o, UBYTE *name);
LONG readobject(NODE objectnode,struct CacheBuffer **returned_cb,struct fsObject **returned_object);
LONG readobjectquick(BLCK objectcontainer,NODE objectnode,struct CacheBuffer **returned_cb,struct fsObject **returned_object);
struct fsObject *findobject(struct fsObjectContainer *oc, NODE objectnode);
struct fsObject *nextobject(struct fsObject *o);
struct fsObject *prevobject(struct fsObject *o, struct fsObjectContainer *oc);
struct fsObject *lastobject(struct fsObjectContainer *oc);
WORD isobject(struct fsObject *o, struct fsObjectContainer *oc);
LONG findcreate(struct ExtFileLock **returned_lock,UBYTE *path,LONG packettype,UBYTE *softlink);
LONG setcomment(struct ExtFileLock *lock,UBYTE *path,UBYTE *comment);
BOOL cleanupdeletedfiles(void);

LONG setrecycledinfo(ULONG deletedfiles, ULONG deletedblocks);
LONG setrecycledinfodiff(LONG deletedfiles, LONG deletedblocks);
LONG getrecycledinfo(ULONG *returned_deletedfiles, ULONG *returned_deletedblocks);

#endif // _OBJECTS_PROTOS_H
