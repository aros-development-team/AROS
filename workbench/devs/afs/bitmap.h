#ifndef BITMAP_H
#define BITMAP_H

#include <exec/types.h>

#include "volumes.h"

ULONG	countUsedBlocks(struct Volume *);
ULONG createNewBitmapBlocks(struct Volume *);
LONG setBitmapFlag(struct Volume *, LONG);
LONG invalidBitmap(struct Volume *);
LONG validBitmap(struct Volume *);
LONG markBlock(struct Volume *, ULONG, ULONG);
ULONG allocBlock(struct Volume *);

#endif
