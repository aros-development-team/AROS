#ifndef FILEHANDLES3_H
#define FILEHANDLES3_H

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/exall.h>

#include "filehandles.h"

LONG sameLock(struct AfsHandle *, struct AfsHandle *);
ULONG examine(struct AfsHandle *, struct ExAllData *, ULONG, ULONG, ULONG *);
ULONG examineAll(struct AfsHandle *, struct ExAllData *, ULONG, ULONG);
ULONG examineNext(struct AfsHandle *, struct FileInfoBlock *);

#endif

