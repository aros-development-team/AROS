#ifndef OS_SUPPORT_H
#define OS_SUPPORT_H

/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef __AROS__
#include "os_aros_support.h"
#else
#include "os_unix_support.h"
#endif

struct BlockCache;
struct Volume;

LONG osMediumInit(struct AFSBase *, struct Volume *, struct BlockCache *);
void osMediumFree(struct AFSBase *, struct Volume *, LONG);
void remDosNode(struct AFSBase *afsbase, struct DosList *dl);
LONG readDisk(struct AFSBase *, struct Volume *, ULONG, ULONG, APTR);
LONG writeDisk(struct AFSBase *, struct Volume *, ULONG, ULONG, APTR);
UBYTE diskPresent(struct AFSBase *, struct IOHandle *);
BOOL diskWritable(struct AFSBase *, struct IOHandle *);
ULONG sectorSize(struct AFSBase *, struct IOHandle *);
BOOL flush(struct AFSBase *, struct Volume *);
struct IOHandle *openBlockDevice(struct AFSBase *, struct IOHandle *);
void closeBlockDevice(struct AFSBase *, struct IOHandle *);
void check64BitSupport(struct AFSBase *, struct Volume *);
LONG attemptAddDosVolume(struct AFSBase *afsbase, struct Volume *volume);

#endif
