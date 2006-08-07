#ifndef OS_SUPPORT_H
#define OS_SUPPORT_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef __AROS__
#include "os_aros_support.h"
#elif defined(unix) || defined(__MACH__)
#include "os_unix_support.h"
#else
#error "Operating System not supported"
#endif

struct BlockCache;
struct Volume;

LONG osMediumInit(struct AFSBase *, struct Volume *, struct BlockCache *);
void osMediumFree(struct AFSBase *, struct Volume *, LONG);
LONG readDisk(struct AFSBase *, struct Volume *, ULONG, ULONG, APTR);
LONG writeDisk(struct AFSBase *, struct Volume *, ULONG, ULONG, APTR);
BOOL flush(struct AFSBase *, struct Volume *);
struct IOHandle *openBlockDevice(struct AFSBase *, struct IOHandle *);
void closeBlockDevice(struct AFSBase *, struct IOHandle *);
void check64BitSupport(struct AFSBase *, struct Volume *);

#endif
