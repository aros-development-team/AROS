#ifndef FILEHANDLES1_H
#define FILEHANDLES1_H

#include <exec/types.h>
#include <dos/dosextens.h>

#include "blockaccess.h"

struct AfsHandle	*openf(struct AfsHandle *, STRPTR, ULONG);
void	closef(struct AfsHandle *);
LONG	read(struct AfsHandle *, void *, ULONG);
LONG	write(struct AfsHandle *, void *, ULONG);
LONG	seek(struct AfsHandle *, LONG, LONG);
struct AfsHandle *openfile(struct AfsHandle *, STRPTR, ULONG, ULONG);

struct BlockCache *getHeaderBlock(struct Volume *,STRPTR, struct BlockCache *, ULONG *);
struct BlockCache *findBlock(struct AfsHandle *, STRPTR name, ULONG *);
struct AfsHandle *getHandle(struct Volume *, struct BlockCache *, ULONG);
struct AfsHandle *findHandle(struct Volume *, ULONG);
#endif
