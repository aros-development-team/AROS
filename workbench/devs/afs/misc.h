#ifndef MISC_H
#define MISC_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "volumes.h"

ULONG writeHeader(struct afsbase *, struct Volume *, struct BlockCache *);
LONG getDiskInfo(struct Volume *, struct InfoData *);
LONG inhibit(struct afsbase *, struct Volume *, ULONG forbid);
LONG format(struct afsbase *, struct Volume *, STRPTR, ULONG);
LONG relabel(struct afsbase *, struct Volume *, STRPTR);

#endif
