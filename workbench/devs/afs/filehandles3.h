#ifndef FILEHANDLES3_H
#define FILEHANDLES3_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "filehandles.h"

LONG sameLock(struct AfsHandle *, struct AfsHandle *);
ULONG examine(struct afsbase *, struct AfsHandle *, struct ExAllData *, ULONG, ULONG, ULONG *);
ULONG examineAll(struct afsbase *, struct AfsHandle *, struct ExAllData *, ULONG, ULONG);
ULONG examineNext(struct afsbase *, struct AfsHandle *, struct FileInfoBlock *);

#endif

