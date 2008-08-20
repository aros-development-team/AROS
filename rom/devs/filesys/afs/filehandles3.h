#ifndef FILEHANDLES3_H
#define FILEHANDLES3_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id: filehandles3.h 19884 2003-09-30 22:36:07Z sheutlin $
*/

#include "os.h"
#include "filehandles.h"

LONG sameLock(struct AfsHandle *, struct AfsHandle *);
ULONG examine(struct AFSBase *, struct AfsHandle *, struct ExAllData *, ULONG, ULONG, ULONG *);
ULONG examineAll(struct AFSBase *, struct AfsHandle *, struct ExAllData *, struct ExAllControl *, ULONG, ULONG);
ULONG examineNext(struct AFSBase *, struct AfsHandle *, struct FileInfoBlock *);

#endif

