#ifndef BITMAP_H
#define BITMAP_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "volumes.h"

ULONG	countUsedBlocks(struct afsbase *, struct Volume *);
ULONG createNewBitmapBlocks(struct afsbase *, struct Volume *);
LONG setBitmapFlag(struct afsbase *, struct Volume *, LONG);
LONG invalidBitmap(struct afsbase *, struct Volume *);
LONG validBitmap(struct afsbase *, struct Volume *);
LONG markBlock(struct afsbase *, struct Volume *, ULONG, ULONG);
ULONG allocBlock(struct afsbase *, struct Volume *);

#endif
