#ifndef FILEHANDLES1_H
#define FILEHANDLES1_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "cache.h"

struct AfsHandle	*openf(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, ULONG, SIPTR *error);
void	closef(struct AFSBase *, struct AfsHandle *);
LONG	readf(struct AFSBase *, struct AfsHandle *, void *, ULONG, SIPTR *error);
LONG	writef(struct AFSBase *, struct AfsHandle *, void *, ULONG, SIPTR *error);
LONG	seek(struct AFSBase *, struct AfsHandle *, LONG, LONG, SIPTR *error);
LONG	setFileSize(struct AFSBase *, struct AfsHandle *, LONG, LONG, SIPTR *error);
struct AfsHandle *openfile(struct AFSBase *, struct AfsHandle *, CONST_STRPTR, ULONG, ULONG, SIPTR *error);

struct BlockCache *getHeaderBlock(struct AFSBase *, struct Volume *,CONST_STRPTR, struct BlockCache *, ULONG *, SIPTR *error);
struct BlockCache *findBlock(struct AFSBase *, struct AfsHandle *, CONST_STRPTR name, ULONG *, SIPTR *error);
struct AfsHandle *getHandle(struct AFSBase *, struct Volume *, struct BlockCache *, ULONG, SIPTR *error);
struct AfsHandle *findHandle(struct Volume *, ULONG);
#endif
