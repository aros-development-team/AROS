#ifndef MISC_H
#define MISC_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: misc.h 28656 2008-05-10 16:11:35Z neil $
*/

#include "os.h"
#include "volumes.h"

ULONG writeHeader(struct AFSBase *, struct Volume *, struct BlockCache *);
LONG getDiskInfo(struct Volume *, struct InfoData *);
LONG inhibit(struct AFSBase *, struct Volume *, ULONG forbid);
LONG format(struct AFSBase *, struct Volume *, CONST_STRPTR, ULONG);
LONG relabel(struct AFSBase *, struct Volume *, CONST_STRPTR);

#endif
