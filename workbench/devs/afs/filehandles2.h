#ifndef FILEHANDLES2_H
#define FILEHANDLES2_H

#include <exec/types.h>
#include <dos/datetime.h>

#include "blockaccess.h"
#include "volumes.h"

struct AfsHandle *createDir(struct AfsHandle *, STRPTR, ULONG);
ULONG	rename(struct AfsHandle *, STRPTR, STRPTR);
ULONG	deleteObject(struct AfsHandle *, STRPTR);
ULONG	setHeaderDate(struct Volume *, struct BlockCache *, struct DateStamp *);
ULONG	setComment(struct AfsHandle *, STRPTR, STRPTR);
ULONG	setProtect(struct AfsHandle *, STRPTR, ULONG);
ULONG	setDate(struct AfsHandle *, STRPTR, struct DateStamp *);

struct BlockCache *getDirBlockBuffer(struct AfsHandle *, STRPTR, STRPTR);
struct BlockCache *createNewEntry(struct Volume *, ULONG, STRPTR, struct BlockCache *, ULONG);

#endif

