#ifndef FILEHANDLES2_H
#define FILEHANDLES2_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "cache.h"
#include "volumes.h"

struct AfsHandle *createDir(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, ULONG);
ULONG	renameObject(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, CONST_STRPTR);
ULONG	deleteObject(struct AFSBase *, struct AfsHandle *, CONST_STRPTR);
ULONG	deleteFileRemainder(struct AFSBase *, struct AfsHandle *);
ULONG	setHeaderDate(struct AFSBase *, struct Volume *, struct BlockCache *, struct DateStamp *);
ULONG	setComment(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, CONST_STRPTR);
ULONG	setProtect(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, ULONG);
ULONG	setDate(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, struct DateStamp *);

struct BlockCache *getDirBlockBuffer(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, STRPTR);
struct BlockCache *createNewEntry(struct AFSBase *, struct Volume *, ULONG, CONST_STRPTR, struct BlockCache *, ULONG);

#endif

