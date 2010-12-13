#ifndef _CACHEDIO_PROTOS_H
#define _CACHEDIO_PROTOS_H

#include <exec/types.h>
#include <dos/filehandler.h>
#include "blockstructure.h"

LONG read(BLCK block, UBYTE *buffer, ULONG blocks);
LONG write(BLCK block, UBYTE *buffer, ULONG blocks);

LONG readbytes(BLCK block, UBYTE *buffer, UWORD offsetinblock, UWORD bytes);
LONG writebytes(BLCK block, UBYTE *buffer, UWORD offsetinblock, UWORD bytes);

LONG initcachedio(UBYTE *devicename, ULONG unit, ULONG flags, struct DosEnvec *de);
void cleanupcachedio(void);

LONG setiocache(ULONG lines, ULONG readahead, BYTE copyback);
ULONG queryiocache_lines(void);
ULONG queryiocache_readaheadsize(void);
BYTE queryiocache_copyback(void);

LONG flushiocache(void);
void invalidateiocaches(void);

#endif // _CACHEDIO_PROTOS_H
