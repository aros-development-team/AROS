#ifndef FILEHANDLES2_H
#define FILEHANDLES2_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/datetime.h>

#include "blockaccess.h"
#include "volumes.h"

struct AfsHandle *createDir(struct afsbase *, struct AfsHandle *, STRPTR, ULONG);
ULONG	rename(struct afsbase *, struct AfsHandle *, STRPTR, STRPTR);
ULONG	deleteObject(struct afsbase *, struct AfsHandle *, STRPTR);
ULONG	setHeaderDate(struct afsbase *, struct Volume *, struct BlockCache *, struct DateStamp *);
ULONG	setComment(struct afsbase *, struct AfsHandle *, STRPTR, STRPTR);
ULONG	setProtect(struct afsbase *, struct AfsHandle *, STRPTR, ULONG);
ULONG	setDate(struct afsbase *, struct AfsHandle *, STRPTR, struct DateStamp *);

struct BlockCache *getDirBlockBuffer(struct afsbase *, struct AfsHandle *, STRPTR, STRPTR);
struct BlockCache *createNewEntry(struct afsbase *, struct Volume *, ULONG, STRPTR, struct BlockCache *, ULONG);

#endif

