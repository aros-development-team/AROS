#ifndef FILEHANDLES1_H
#define FILEHANDLES1_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include "cache.h"

struct AfsHandle	*openf(struct afsbase *, struct AfsHandle *, STRPTR, ULONG);
void	closef(struct afsbase *, struct AfsHandle *);
LONG	readf(struct afsbase *, struct AfsHandle *, void *, ULONG);
LONG	writef(struct afsbase *, struct AfsHandle *, void *, ULONG);
LONG	seek(struct afsbase *, struct AfsHandle *, LONG, LONG);
struct AfsHandle *openfile(struct afsbase *, struct AfsHandle *, STRPTR, ULONG, ULONG);

struct BlockCache *getHeaderBlock(struct afsbase *, struct Volume *,STRPTR, struct BlockCache *, ULONG *);
struct BlockCache *findBlock(struct afsbase *, struct AfsHandle *, STRPTR name, ULONG *);
struct AfsHandle *getHandle(struct afsbase *, struct Volume *, struct BlockCache *, ULONG);
struct AfsHandle *findHandle(struct Volume *, ULONG);
#endif
