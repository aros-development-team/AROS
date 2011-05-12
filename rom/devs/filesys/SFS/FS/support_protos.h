#ifndef _SUPPORT_PROTOS_H
#define _SUPPORT_PROTOS_H

#include <dos/dos.h>
#include <exec/types.h>

void uncompress(UWORD *dest,UBYTE *data,UWORD length);
WORD compress(UWORD *org,UWORD *new,UBYTE *dest);
WORD compressfromzero(UWORD *new,UBYTE *dest);
UBYTE *stripcolon(UBYTE *);
UWORD hash(UBYTE *name, WORD casesensitive);
UBYTE upperchar(UBYTE);
UWORD bstrlen(BSTR);
UWORD copybstrasstr(BSTR,UBYTE *,UWORD);
void copystr(UBYTE *src,UBYTE *dest,UWORD maxlen);
void initlist(struct List *);
ULONG datestamptodate(struct DateStamp *datestamp);
void datetodatestamp(ULONG date,struct DateStamp *datestamp);
ULONG getdate(void);
UBYTE *validatepath(UBYTE *string);
BYTE isvalidcomponentname(UBYTE *name);
void ClearMemQuick(void *mem, LONG bytes);

void checksum_writelong(struct fsBlockHeader *bh, void *dest, ULONG data);
void checksum_writelong_be(struct fsBlockHeader *bh, void *dest, ULONG data);

UWORD mergediffs(UBYTE *olddiff, UBYTE *newdiff, UWORD length, ULONG *new, ULONG *org, UBYTE *modifiedblocks);

#endif // _SUPPORT_PROTOS_H
