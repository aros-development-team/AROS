#ifndef _VBCCINLINE_XPKMASTER_H
#define _VBCCINLINE_XPKMASTER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

LONG __XpkExamine(__reg("a6") struct Library *, __reg("a0") struct XpkFib * fib, __reg("a1") struct TagItem * tags)="\tjsr\t-36(a6)";
#define XpkExamine(fib, tags) __XpkExamine(XpkBase, (fib), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __XpkExamineTags(__reg("a6") struct Library *, __reg("a0") struct XpkFib * fib, ULONG tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-36(a6)\n\tmovea.l\t(a7)+,a1";
#define XpkExamineTags(fib, ...) __XpkExamineTags(XpkBase, (fib), __VA_ARGS__)
#endif

LONG __XpkPack(__reg("a6") struct Library *, __reg("a0") struct TagItem * tags)="\tjsr\t-42(a6)";
#define XpkPack(tags) __XpkPack(XpkBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __XpkPackTags(__reg("a6") struct Library *, ULONG tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-42(a6)\n\tmovea.l\t(a7)+,a0";
#define XpkPackTags(...) __XpkPackTags(XpkBase, __VA_ARGS__)
#endif

LONG __XpkUnpack(__reg("a6") struct Library *, __reg("a0") struct TagItem * tags)="\tjsr\t-48(a6)";
#define XpkUnpack(tags) __XpkUnpack(XpkBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __XpkUnpackTags(__reg("a6") struct Library *, ULONG tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-48(a6)\n\tmovea.l\t(a7)+,a0";
#define XpkUnpackTags(...) __XpkUnpackTags(XpkBase, __VA_ARGS__)
#endif

LONG __XpkOpen(__reg("a6") struct Library *, __reg("a0") struct XpkFib ** xbuf, __reg("a1") struct TagItem * tags)="\tjsr\t-54(a6)";
#define XpkOpen(xbuf, tags) __XpkOpen(XpkBase, (xbuf), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __XpkOpenTags(__reg("a6") struct Library *, __reg("a0") struct XpkFib ** xbuf, ULONG tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-54(a6)\n\tmovea.l\t(a7)+,a1";
#define XpkOpenTags(xbuf, ...) __XpkOpenTags(XpkBase, (xbuf), __VA_ARGS__)
#endif

LONG __XpkRead(__reg("a6") struct Library *, __reg("a0") struct XpkFib * xbuf, __reg("a1") STRPTR buf, __reg("d0") ULONG len)="\tjsr\t-60(a6)";
#define XpkRead(xbuf, buf, len) __XpkRead(XpkBase, (xbuf), (buf), (len))

LONG __XpkWrite(__reg("a6") struct Library *, __reg("a0") struct XpkFib * xbuf, __reg("a1") STRPTR buf, __reg("d0") LONG len)="\tjsr\t-66(a6)";
#define XpkWrite(xbuf, buf, len) __XpkWrite(XpkBase, (xbuf), (buf), (len))

LONG __XpkSeek(__reg("a6") struct Library *, __reg("a0") struct XpkFib * xbuf, __reg("d0") LONG len, __reg("d1") LONG mode)="\tjsr\t-72(a6)";
#define XpkSeek(xbuf, len, mode) __XpkSeek(XpkBase, (xbuf), (len), (mode))

LONG __XpkClose(__reg("a6") struct Library *, __reg("a0") struct XpkFib * xbuf)="\tjsr\t-78(a6)";
#define XpkClose(xbuf) __XpkClose(XpkBase, (xbuf))

LONG __XpkQuery(__reg("a6") struct Library *, __reg("a0") struct TagItem * tags)="\tjsr\t-84(a6)";
#define XpkQuery(tags) __XpkQuery(XpkBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __XpkQueryTags(__reg("a6") struct Library *, ULONG tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-84(a6)\n\tmovea.l\t(a7)+,a0";
#define XpkQueryTags(...) __XpkQueryTags(XpkBase, __VA_ARGS__)
#endif

APTR __XpkAllocObject(__reg("a6") struct Library *, __reg("d0") ULONG type, __reg("a0") struct TagItem * tags)="\tjsr\t-90(a6)";
#define XpkAllocObject(type, tags) __XpkAllocObject(XpkBase, (type), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
APTR __XpkAllocObjectTags(__reg("a6") struct Library *, __reg("d0") ULONG type, ULONG tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-90(a6)\n\tmovea.l\t(a7)+,a0";
#define XpkAllocObjectTags(type, ...) __XpkAllocObjectTags(XpkBase, (type), __VA_ARGS__)
#endif

void __XpkFreeObject(__reg("a6") struct Library *, __reg("d0") ULONG type, __reg("a0") APTR object)="\tjsr\t-96(a6)";
#define XpkFreeObject(type, object) __XpkFreeObject(XpkBase, (type), (object))

BOOL __XpkPrintFault(__reg("a6") struct Library *, __reg("d0") LONG code, __reg("a0") STRPTR header)="\tjsr\t-102(a6)";
#define XpkPrintFault(code, header) __XpkPrintFault(XpkBase, (code), (header))

ULONG __XpkFault(__reg("a6") struct Library *, __reg("d0") LONG code, __reg("a0") STRPTR header, __reg("a1") STRPTR buffer, __reg("d1") ULONG size)="\tjsr\t-108(a6)";
#define XpkFault(code, header, buffer, size) __XpkFault(XpkBase, (code), (header), (buffer), (size))

LONG __XpkPassRequest(__reg("a6") struct Library *, __reg("a0") struct TagItem * tags)="\tjsr\t-114(a6)";
#define XpkPassRequest(tags) __XpkPassRequest(XpkBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __XpkPassRequestTags(__reg("a6") struct Library *, ULONG tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-114(a6)\n\tmovea.l\t(a7)+,a0";
#define XpkPassRequestTags(...) __XpkPassRequestTags(XpkBase, __VA_ARGS__)
#endif

#endif /*  _VBCCINLINE_XPKMASTER_H  */
