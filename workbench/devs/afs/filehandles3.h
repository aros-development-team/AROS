#ifndef FILEHANDLES3_H
#define FILEHANDLES3_H

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/exall.h>

#include "filehandles.h"
#include "afshandler.h"

LONG sameLock(struct AfsHandle *, struct AfsHandle *);
ULONG examine(struct afsbase *, struct AfsHandle *, struct ExAllData *, ULONG, ULONG, ULONG *);
ULONG examineAll(struct afsbase *, struct AfsHandle *, struct ExAllData *, ULONG, ULONG);
ULONG examineNext(struct afsbase *, struct AfsHandle *, struct FileInfoBlock *);

#endif

