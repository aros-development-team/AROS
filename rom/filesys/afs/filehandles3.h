#ifndef FILEHANDLES3_H
#define FILEHANDLES3_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "filehandles.h"

LONG sameLock(struct AfsHandle *, struct AfsHandle *);
ULONG examine(struct AFSBase *, struct AfsHandle *, struct ExAllData *, ULONG, ULONG, IPTR *);
ULONG examineNext(struct AFSBase *, struct AfsHandle *, struct FileInfoBlock *);

#endif

