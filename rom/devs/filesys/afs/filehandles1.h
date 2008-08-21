#ifndef FILEHANDLES1_H
#define FILEHANDLES1_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "cache.h"

struct AfsHandle	*openf(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, ULONG);
void	closef(struct AFSBase *, struct AfsHandle *);
LONG	readf(struct AFSBase *, struct AfsHandle *, void *, ULONG);
LONG	writef(struct AFSBase *, struct AfsHandle *, void *, ULONG);
LONG	seek(struct AFSBase *, struct AfsHandle *, LONG, LONG);
LONG	setFileSize(struct AFSBase *, struct AfsHandle *, LONG, LONG);
struct AfsHandle *openfile(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, ULONG, ULONG);

struct BlockCache *getHeaderBlock(struct AFSBase *, struct Volume *,CONST_STRPTR, struct BlockCache *, ULONG *);
struct BlockCache *findBlock(struct AFSBase *, struct AfsHandle *, CONST_STRPTR name, ULONG *);
struct AfsHandle *getHandle(struct AFSBase *, struct Volume *, struct BlockCache *, ULONG);
struct AfsHandle *findHandle(struct Volume *, ULONG);
#endif
