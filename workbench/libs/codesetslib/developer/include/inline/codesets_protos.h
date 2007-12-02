#ifndef _VBCCINLINE_CODESETS_H
#define _VBCCINLINE_CODESETS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

ULONG __CodesetsConvertUTF32toUTF16(__reg("a6") struct Library *, __reg("a0") void * sourceStart, __reg("a1") void * sourceEnd, __reg("a2") UTF16 ** targetStart, __reg("a3") UTF16 * targetEnd, __reg("d0") ULONG flags)="\tjsr\t-36(a6)";
#define CodesetsConvertUTF32toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags) __CodesetsConvertUTF32toUTF16(CodesetsBase, (void *)(sourceStart), (void *)(sourceEnd), (targetStart), (targetEnd), (flags))

ULONG __CodesetsConvertUTF16toUTF32(__reg("a6") struct Library *, __reg("a0") void * sourceStart, __reg("a1") void * sourceEnd, __reg("a2") UTF32 ** targetStart, __reg("a3") UTF32 * targetEnd, __reg("d0") ULONG flags)="\tjsr\t-42(a6)";
#define CodesetsConvertUTF16toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags) __CodesetsConvertUTF16toUTF32(CodesetsBase, (void *)(sourceStart), (void *)(sourceEnd), (targetStart), (targetEnd), (flags))

ULONG __CodesetsConvertUTF16toUTF8(__reg("a6") struct Library *, __reg("a0") void * sourceStart, __reg("a1") void * sourceEnd, __reg("a2") UTF8 ** targetStart, __reg("a3") UTF8 * targetEnd, __reg("d0") ULONG flags)="\tjsr\t-48(a6)";
#define CodesetsConvertUTF16toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags) __CodesetsConvertUTF16toUTF8(CodesetsBase, (void *)(sourceStart), (void *)(sourceEnd), (targetStart), (targetEnd), (flags))

BOOL __CodesetsIsLegalUTF8(__reg("a6") struct Library *, __reg("a0") void * source, __reg("d0") ULONG length)="\tjsr\t-54(a6)";
#define CodesetsIsLegalUTF8(source, length) __CodesetsIsLegalUTF8(CodesetsBase, (void *)(source), (length))

BOOL __CodesetsIsLegalUTF8Sequence(__reg("a6") struct Library *, __reg("a0") void * source, __reg("a1") void * sourceEnd)="\tjsr\t-60(a6)";
#define CodesetsIsLegalUTF8Sequence(source, sourceEnd) __CodesetsIsLegalUTF8Sequence(CodesetsBase, (void *)(source), (void *)(sourceEnd))

ULONG __CodesetsConvertUTF8toUTF16(__reg("a6") struct Library *, __reg("a0") void * sourceStart, __reg("a1") void * sourceEnd, __reg("a2") UTF16 ** targetStart, __reg("a3") UTF16 * targetEnd, __reg("d0") ULONG flags)="\tjsr\t-66(a6)";
#define CodesetsConvertUTF8toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags) __CodesetsConvertUTF8toUTF16(CodesetsBase, (void *)(sourceStart), (void *)(sourceEnd), (targetStart), (targetEnd), (flags))

ULONG __CodesetsConvertUTF32toUTF8(__reg("a6") struct Library *, __reg("a0") void * sourceStart, __reg("a1") void * sourceEnd, __reg("a2") UTF8 ** targetStart, __reg("a3") UTF8 * targetEnd, __reg("d0") ULONG flags)="\tjsr\t-72(a6)";
#define CodesetsConvertUTF32toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags) __CodesetsConvertUTF32toUTF8(CodesetsBase, (void *)(sourceStart), (void *)(sourceEnd), (targetStart), (targetEnd), (flags))

ULONG __CodesetsConvertUTF8toUTF32(__reg("a6") struct Library *, __reg("a0") void * sourceStart, __reg("a1") void * sourceEnd, __reg("a2") UTF32 ** targetStart, __reg("a3") UTF32 * targetEnd, __reg("d0") ULONG flags)="\tjsr\t-78(a6)";
#define CodesetsConvertUTF8toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags) __CodesetsConvertUTF8toUTF32(CodesetsBase, (void *)(sourceStart), (void *)(sourceEnd), (targetStart), (targetEnd), (flags))

struct codeset * __CodesetsSetDefaultA(__reg("a6") struct Library *, __reg("a0") STRPTR name, __reg("a1") struct TagItem * attrs)="\tjsr\t-84(a6)";
#define CodesetsSetDefaultA(name, attrs) __CodesetsSetDefaultA(CodesetsBase, (name), (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct codeset * __CodesetsSetDefault(__reg("a6") struct Library *, __reg("a0") STRPTR name, Tag attrs, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-84(a6)\n\tmovea.l\t(a7)+,a1";
#define CodesetsSetDefault(name, ...) __CodesetsSetDefault(CodesetsBase, (name), __VA_ARGS__)
#endif

void __CodesetsFreeA(__reg("a6") struct Library *, __reg("a0") APTR obj, __reg("a1") struct TagItem * attrs)="\tjsr\t-90(a6)";
#define CodesetsFreeA(obj, attrs) __CodesetsFreeA(CodesetsBase, (obj), (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
void __CodesetsFree(__reg("a6") struct Library *, __reg("a0") APTR obj, Tag attrs, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-90(a6)\n\tmovea.l\t(a7)+,a1";
#define CodesetsFree(obj, ...) __CodesetsFree(CodesetsBase, (obj), __VA_ARGS__)
#endif

STRPTR * __CodesetsSupportedA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-96(a6)";
#define CodesetsSupportedA(attrs) __CodesetsSupportedA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
STRPTR * __CodesetsSupported(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-96(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsSupported(...) __CodesetsSupported(CodesetsBase, __VA_ARGS__)
#endif

struct codeset * __CodesetsFindA(__reg("a6") struct Library *, __reg("a0") STRPTR name, __reg("a1") struct TagItem * attrs)="\tjsr\t-102(a6)";
#define CodesetsFindA(name, attrs) __CodesetsFindA(CodesetsBase, (name), (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct codeset * __CodesetsFind(__reg("a6") struct Library *, __reg("a0") STRPTR name, Tag attrs, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-102(a6)\n\tmovea.l\t(a7)+,a1";
#define CodesetsFind(name, ...) __CodesetsFind(CodesetsBase, (name), __VA_ARGS__)
#endif

struct codeset * __CodesetsFindBestA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-108(a6)";
#define CodesetsFindBestA(attrs) __CodesetsFindBestA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct codeset * __CodesetsFindBest(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-108(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsFindBest(...) __CodesetsFindBest(CodesetsBase, __VA_ARGS__)
#endif

ULONG __CodesetsUTF8Len(__reg("a6") struct Library *, __reg("a0") void * str)="\tjsr\t-114(a6)";
#define CodesetsUTF8Len(str) __CodesetsUTF8Len(CodesetsBase, (void *)(str))

STRPTR __CodesetsUTF8ToStrA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-120(a6)";
#define CodesetsUTF8ToStrA(attrs) __CodesetsUTF8ToStrA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
STRPTR __CodesetsUTF8ToStr(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-120(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsUTF8ToStr(...) __CodesetsUTF8ToStr(CodesetsBase, __VA_ARGS__)
#endif

UTF8 * __CodesetsUTF8CreateA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-126(a6)";
#define CodesetsUTF8CreateA(attrs) __CodesetsUTF8CreateA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
UTF8 * __CodesetsUTF8Create(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-126(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsUTF8Create(...) __CodesetsUTF8Create(CodesetsBase, __VA_ARGS__)
#endif

ULONG __CodesetsEncodeB64A(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-132(a6)";
#define CodesetsEncodeB64A(attrs) __CodesetsEncodeB64A(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __CodesetsEncodeB64(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-132(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsEncodeB64(...) __CodesetsEncodeB64(CodesetsBase, __VA_ARGS__)
#endif

ULONG __CodesetsDecodeB64A(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-138(a6)";
#define CodesetsDecodeB64A(attrs) __CodesetsDecodeB64A(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __CodesetsDecodeB64(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-138(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsDecodeB64(...) __CodesetsDecodeB64(CodesetsBase, __VA_ARGS__)
#endif

ULONG __CodesetsStrLenA(__reg("a6") struct Library *, __reg("a0") STRPTR str, __reg("a1") struct TagItem * attrs)="\tjsr\t-144(a6)";
#define CodesetsStrLenA(str, attrs) __CodesetsStrLenA(CodesetsBase, (str), (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __CodesetsStrLen(__reg("a6") struct Library *, __reg("a0") STRPTR str, Tag attrs, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-144(a6)\n\tmovea.l\t(a7)+,a1";
#define CodesetsStrLen(str, ...) __CodesetsStrLen(CodesetsBase, (str), __VA_ARGS__)
#endif

BOOL __CodesetsIsValidUTF8(__reg("a6") struct Library *, __reg("a0") STRPTR str)="\tjsr\t-150(a6)";
#define CodesetsIsValidUTF8(str) __CodesetsIsValidUTF8(CodesetsBase, (str))

void __CodesetsFreeVecPooledA(__reg("a6") struct Library *, __reg("a0") APTR pool, __reg("a1") APTR mem, __reg("a2") struct TagItem * attrs)="\tjsr\t-156(a6)";
#define CodesetsFreeVecPooledA(pool, mem, attrs) __CodesetsFreeVecPooledA(CodesetsBase, (pool), (mem), (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
void __CodesetsFreeVecPooled(__reg("a6") struct Library *, __reg("a0") APTR pool, __reg("a1") APTR mem, Tag attrs, ...)="\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-156(a6)\n\tmovea.l\t(a7)+,a2";
#define CodesetsFreeVecPooled(pool, mem, ...) __CodesetsFreeVecPooled(CodesetsBase, (pool), (mem), __VA_ARGS__)
#endif

STRPTR __CodesetsConvertStrA(__reg("a6") struct Library *, __reg("a0") struct TagItem * str)="\tjsr\t-162(a6)";
#define CodesetsConvertStrA(str) __CodesetsConvertStrA(CodesetsBase, (str))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
STRPTR __CodesetsConvertStr(__reg("a6") struct Library *, Tag str, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-162(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsConvertStr(...) __CodesetsConvertStr(CodesetsBase, __VA_ARGS__)
#endif

struct codesetList * __CodesetsListCreateA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-168(a6)";
#define CodesetsListCreateA(attrs) __CodesetsListCreateA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct codesetList * __CodesetsListCreate(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-168(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsListCreate(...) __CodesetsListCreate(CodesetsBase, __VA_ARGS__)
#endif

BOOL __CodesetsListDeleteA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-174(a6)";
#define CodesetsListDeleteA(attrs) __CodesetsListDeleteA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
BOOL __CodesetsListDelete(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-174(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsListDelete(...) __CodesetsListDelete(CodesetsBase, __VA_ARGS__)
#endif

BOOL __CodesetsListAddA(__reg("a6") struct Library *, __reg("a0") struct codesetList * list, __reg("a1") struct TagItem * attrs)="\tjsr\t-180(a6)";
#define CodesetsListAddA(list, attrs) __CodesetsListAddA(CodesetsBase, (list), (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
BOOL __CodesetsListAdd(__reg("a6") struct Library *, __reg("a0") struct codesetList * list, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-180(a6)\n\tmovea.l\t(a7)+,a1";
#define CodesetsListAdd(...) __CodesetsListAdd(CodesetsBase, __VA_ARGS__)
#endif

BOOL __CodesetsListRemoveA(__reg("a6") struct Library *, __reg("a0") struct TagItem * attrs)="\tjsr\t-186(a6)";
#define CodesetsListRemoveA(attrs) __CodesetsListRemoveA(CodesetsBase, (attrs))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
BOOL __CodesetsListRemove(__reg("a6") struct Library *, Tag attrs, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-186(a6)\n\tmovea.l\t(a7)+,a0";
#define CodesetsListRemove(...) __CodesetsListRemove(CodesetsBase, __VA_ARGS__)
#endif

#endif /*  _VBCCINLINE_CODESETS_H  */
