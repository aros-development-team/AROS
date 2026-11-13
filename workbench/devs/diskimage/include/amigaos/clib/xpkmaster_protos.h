#ifndef CLIB_XPKMASTER_PROTOS_H
#define CLIB_XPKMASTER_PROTOS_H


/*
**	$VER: xpkmaster_protos.h 1.0 (30.03.2010)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2010 
**	All Rights Reserved
*/

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef  XPK_XPK_H
#include <xpk/xpk.h>
#endif

LONG XpkExamine(struct XpkFib * fib, struct TagItem * tags);
LONG XpkExamineTags(struct XpkFib * fib, ULONG tags, ...);
LONG XpkPack(struct TagItem * tags);
LONG XpkPackTags(ULONG tags, ...);
LONG XpkUnpack(struct TagItem * tags);
LONG XpkUnpackTags(ULONG tags, ...);
LONG XpkOpen(struct XpkFib ** xbuf, struct TagItem * tags);
LONG XpkOpenTags(struct XpkFib ** xbuf, ULONG tags, ...);
LONG XpkRead(struct XpkFib * xbuf, STRPTR buf, ULONG len);
LONG XpkWrite(struct XpkFib * xbuf, STRPTR buf, LONG len);
LONG XpkSeek(struct XpkFib * xbuf, LONG len, LONG mode);
LONG XpkClose(struct XpkFib * xbuf);
LONG XpkQuery(struct TagItem * tags);
LONG XpkQueryTags(ULONG tags, ...);
APTR XpkAllocObject(ULONG type, struct TagItem * tags);
APTR XpkAllocObjectTags(ULONG type, ULONG tags, ...);
void XpkFreeObject(ULONG type, APTR object);
BOOL XpkPrintFault(LONG code, STRPTR header);
ULONG XpkFault(LONG code, STRPTR header, STRPTR buffer, ULONG size);
LONG XpkPassRequest(struct TagItem * tags);
LONG XpkPassRequestTags(ULONG tags, ...);

#endif	/*  CLIB_XPKMASTER_PROTOS_H  */
