#ifndef OS_SUPPORT_H
#define OS_SUPPORT_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef __AROS__
#include "os_aros_support.h"
#elif defined(unix)
#include "os_unix_support.h"
#else
#error "Operating System not supported"
#endif

struct BlockCache {};
struct Volume {};

LONG osMediumInit(struct afsbase *, struct Volume *, struct BlockCache *);
void osMediumFree(struct afsbase *, struct Volume *, LONG);
ULONG readDisk(struct afsbase *, struct Volume *, ULONG, ULONG, APTR);
ULONG writeDisk(struct afsbase *, struct Volume *, ULONG, ULONG, APTR);
ULONG flush(struct afsbase *, struct Volume *);
struct IOHandle *openBlockDevice(struct afsbase *, struct IOHandle *);
void closeBlockDevice(struct afsbase *, struct IOHandle *);
void check64BitSupport(struct afsbase *, struct Volume *);

#endif
