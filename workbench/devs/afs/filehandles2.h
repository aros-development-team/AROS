#ifndef FILEHANDLES2_H
#define FILEHANDLES2_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "cache.h"
#include "volumes.h"

struct AfsHandle *createDir(struct AFSBase *, struct AfsHandle *, STRPTR, ULONG);
ULONG	renameObject(struct AFSBase *, struct AfsHandle *, STRPTR, STRPTR);
ULONG	deleteObject(struct AFSBase *, struct AfsHandle *, STRPTR);
ULONG	setHeaderDate(struct AFSBase *, struct Volume *, struct BlockCache *, struct DateStamp *);
ULONG	setComment(struct AFSBase *, struct AfsHandle *, STRPTR, STRPTR);
ULONG	setProtect(struct AFSBase *, struct AfsHandle *, STRPTR, ULONG);
ULONG	setDate(struct AFSBase *, struct AfsHandle *, STRPTR, struct DateStamp *);

struct BlockCache *getDirBlockBuffer(struct AFSBase *, struct AfsHandle *, STRPTR, STRPTR);
struct BlockCache *createNewEntry(struct AFSBase *, struct Volume *, ULONG, STRPTR, struct BlockCache *, ULONG);

#endif

