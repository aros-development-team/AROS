#ifndef _VBCCINLINE_XADMASTER_H
#define _VBCCINLINE_XADMASTER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

APTR __xadAllocObjectA(__reg("a6") struct xadMasterBase *, __reg("d0") LONG type, __reg("a0") const struct TagItem * tags)="\tjsr\t-30(a6)";
#define xadAllocObjectA(type, tags) __xadAllocObjectA(xadMasterBase, (type), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
APTR __xadAllocObject(__reg("a6") struct xadMasterBase *, __reg("d0") LONG type, Tag tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-30(a6)\n\tmovea.l\t(a7)+,a0";
#define xadAllocObject(type, ...) __xadAllocObject(xadMasterBase, (type), __VA_ARGS__)
#endif

void __xadFreeObjectA(__reg("a6") struct xadMasterBase *, __reg("a0") APTR object, __reg("a1") const struct TagItem * tags)="\tjsr\t-36(a6)";
#define xadFreeObjectA(object, tags) __xadFreeObjectA(xadMasterBase, (object), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
void __xadFreeObject(__reg("a6") struct xadMasterBase *, __reg("a0") APTR object, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-36(a6)\n\tmovea.l\t(a7)+,a1";
#define xadFreeObject(object, ...) __xadFreeObject(xadMasterBase, (object), __VA_ARGS__)
#endif

struct xadClient * __xadRecogFileA(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG size, __reg("a0") APTR memory, __reg("a1") const struct TagItem * tags)="\tjsr\t-42(a6)";
#define xadRecogFileA(size, memory, tags) __xadRecogFileA(xadMasterBase, (size), (memory), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct xadClient * __xadRecogFile(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG size, __reg("a0") APTR memory, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-42(a6)\n\tmovea.l\t(a7)+,a1";
#define xadRecogFile(size, memory, ...) __xadRecogFile(xadMasterBase, (size), (memory), __VA_ARGS__)
#endif

LONG __xadGetInfoA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-48(a6)";
#define xadGetInfoA(ai, tags) __xadGetInfoA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadGetInfo(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-48(a6)\n\tmovea.l\t(a7)+,a1";
#define xadGetInfo(ai, ...) __xadGetInfo(xadMasterBase, (ai), __VA_ARGS__)
#endif

void __xadFreeInfo(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai)="\tjsr\t-54(a6)";
#define xadFreeInfo(ai) __xadFreeInfo(xadMasterBase, (ai))

LONG __xadFileUnArcA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-60(a6)";
#define xadFileUnArcA(ai, tags) __xadFileUnArcA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadFileUnArc(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-60(a6)\n\tmovea.l\t(a7)+,a1";
#define xadFileUnArc(ai, ...) __xadFileUnArc(xadMasterBase, (ai), __VA_ARGS__)
#endif

LONG __xadDiskUnArcA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-66(a6)";
#define xadDiskUnArcA(ai, tags) __xadDiskUnArcA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadDiskUnArc(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-66(a6)\n\tmovea.l\t(a7)+,a1";
#define xadDiskUnArc(ai, ...) __xadDiskUnArc(xadMasterBase, (ai), __VA_ARGS__)
#endif

STRPTR __xadGetErrorText(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG errnum)="\tjsr\t-72(a6)";
#define xadGetErrorText(errnum) __xadGetErrorText(xadMasterBase, (errnum))

struct xadClient * __xadGetClientInfo(__reg("a6") struct xadMasterBase *)="\tjsr\t-78(a6)";
#define xadGetClientInfo() __xadGetClientInfo(xadMasterBase)

LONG __xadHookAccess(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG command, __reg("d1") LONG data, __reg("a0") APTR buffer, __reg("a1") struct xadArchiveInfo * ai)="\tjsr\t-84(a6)";
#define xadHookAccess(command, data, buffer, ai) __xadHookAccess(xadMasterBase, (command), (data), (buffer), (ai))

LONG __xadConvertDatesA(__reg("a6") struct xadMasterBase *, __reg("a0") const struct TagItem * tags)="\tjsr\t-90(a6)";
#define xadConvertDatesA(tags) __xadConvertDatesA(xadMasterBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadConvertDates(__reg("a6") struct xadMasterBase *, Tag tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-90(a6)\n\tmovea.l\t(a7)+,a0";
#define xadConvertDates(...) __xadConvertDates(xadMasterBase, __VA_ARGS__)
#endif

UWORD __xadCalcCRC16(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG id, __reg("d1") ULONG init, __reg("d2") ULONG size, __reg("a0") STRPTR buffer)="\tjsr\t-96(a6)";
#define xadCalcCRC16(id, init, size, buffer) __xadCalcCRC16(xadMasterBase, (id), (init), (size), (buffer))

ULONG __xadCalcCRC32(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG id, __reg("d1") ULONG init, __reg("d2") ULONG size, __reg("a0") STRPTR buffer)="\tjsr\t-102(a6)";
#define xadCalcCRC32(id, init, size, buffer) __xadCalcCRC32(xadMasterBase, (id), (init), (size), (buffer))

APTR __xadAllocVec(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG size, __reg("d1") ULONG flags)="\tjsr\t-108(a6)";
#define xadAllocVec(size, flags) __xadAllocVec(xadMasterBase, (size), (flags))

void __xadCopyMem(__reg("a6") struct xadMasterBase *, __reg("a0") const void * src, __reg("a1") APTR dest, __reg("d0") ULONG size)="\tjsr\t-114(a6)";
#define xadCopyMem(src, dest, size) __xadCopyMem(xadMasterBase, (src), (dest), (size))

LONG __xadHookTagAccessA(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG command, __reg("d1") LONG data, __reg("a0") APTR buffer, __reg("a1") struct xadArchiveInfo * ai, __reg("a2") const struct TagItem * tags)="\tjsr\t-120(a6)";
#define xadHookTagAccessA(command, data, buffer, ai, tags) __xadHookTagAccessA(xadMasterBase, (command), (data), (buffer), (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadHookTagAccess(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG command, __reg("d1") LONG data, __reg("a0") APTR buffer, __reg("a1") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-120(a6)\n\tmovea.l\t(a7)+,a2";
#define xadHookTagAccess(command, data, buffer, ai, ...) __xadHookTagAccess(xadMasterBase, (command), (data), (buffer), (ai), __VA_ARGS__)
#endif

LONG __xadConvertProtectionA(__reg("a6") struct xadMasterBase *, __reg("a0") const struct TagItem * tags)="\tjsr\t-126(a6)";
#define xadConvertProtectionA(tags) __xadConvertProtectionA(xadMasterBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadConvertProtection(__reg("a6") struct xadMasterBase *, Tag tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-126(a6)\n\tmovea.l\t(a7)+,a0";
#define xadConvertProtection(...) __xadConvertProtection(xadMasterBase, __VA_ARGS__)
#endif

LONG __xadGetDiskInfoA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-132(a6)";
#define xadGetDiskInfoA(ai, tags) __xadGetDiskInfoA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadGetDiskInfo(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-132(a6)\n\tmovea.l\t(a7)+,a1";
#define xadGetDiskInfo(ai, ...) __xadGetDiskInfo(xadMasterBase, (ai), __VA_ARGS__)
#endif

LONG __xadDiskFileUnArcA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-138(a6)";
#define xadDiskFileUnArcA(ai, tags) __xadDiskFileUnArcA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadDiskFileUnArc(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-138(a6)\n\tmovea.l\t(a7)+,a1";
#define xadDiskFileUnArc(ai, ...) __xadDiskFileUnArc(xadMasterBase, (ai), __VA_ARGS__)
#endif

LONG __xadGetHookAccessA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-144(a6)";
#define xadGetHookAccessA(ai, tags) __xadGetHookAccessA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadGetHookAccess(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-144(a6)\n\tmovea.l\t(a7)+,a1";
#define xadGetHookAccess(ai, ...) __xadGetHookAccess(xadMasterBase, (ai), __VA_ARGS__)
#endif

LONG __xadFreeHookAccessA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, __reg("a1") const struct TagItem * tags)="\tjsr\t-150(a6)";
#define xadFreeHookAccessA(ai, tags) __xadFreeHookAccessA(xadMasterBase, (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadFreeHookAccess(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-150(a6)\n\tmovea.l\t(a7)+,a1";
#define xadFreeHookAccess(ai, ...) __xadFreeHookAccess(xadMasterBase, (ai), __VA_ARGS__)
#endif

LONG __xadAddFileEntryA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadFileInfo * fi, __reg("a1") struct xadArchiveInfo * ai, __reg("a2") const struct TagItem * tags)="\tjsr\t-156(a6)";
#define xadAddFileEntryA(fi, ai, tags) __xadAddFileEntryA(xadMasterBase, (fi), (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadAddFileEntry(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadFileInfo * fi, __reg("a1") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-156(a6)\n\tmovea.l\t(a7)+,a2";
#define xadAddFileEntry(fi, ai, ...) __xadAddFileEntry(xadMasterBase, (fi), (ai), __VA_ARGS__)
#endif

LONG __xadAddDiskEntryA(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadDiskInfo * di, __reg("a1") struct xadArchiveInfo * ai, __reg("a2") const struct TagItem * tags)="\tjsr\t-162(a6)";
#define xadAddDiskEntryA(di, ai, tags) __xadAddDiskEntryA(xadMasterBase, (di), (ai), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadAddDiskEntry(__reg("a6") struct xadMasterBase *, __reg("a0") struct xadDiskInfo * di, __reg("a1") struct xadArchiveInfo * ai, Tag tags, ...)="\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-162(a6)\n\tmovea.l\t(a7)+,a2";
#define xadAddDiskEntry(di, ai, ...) __xadAddDiskEntry(xadMasterBase, (di), (ai), __VA_ARGS__)
#endif

LONG __xadGetFilenameA(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG buffersize, __reg("a0") STRPTR buffer, __reg("a1") STRPTR path, __reg("a2") STRPTR name, __reg("a3") const struct TagItem * tags)="\tjsr\t-168(a6)";
#define xadGetFilenameA(buffersize, buffer, path, name, tags) __xadGetFilenameA(xadMasterBase, (buffersize), (buffer), (path), (name), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __xadGetFilename(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG buffersize, __reg("a0") STRPTR buffer, __reg("a1") STRPTR path, __reg("a2") STRPTR name, Tag tags, ...)="\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-168(a6)\n\tmovea.l\t(a7)+,a3";
#define xadGetFilename(buffersize, buffer, path, name, ...) __xadGetFilename(xadMasterBase, (buffersize), (buffer), (path), (name), __VA_ARGS__)
#endif

STRPTR __xadConvertNameA(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG charset, __reg("a0") const struct TagItem * tags)="\tjsr\t-174(a6)";
#define xadConvertNameA(charset, tags) __xadConvertNameA(xadMasterBase, (charset), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
STRPTR __xadConvertName(__reg("a6") struct xadMasterBase *, __reg("d0") ULONG charset, Tag tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-174(a6)\n\tmovea.l\t(a7)+,a0";
#define xadConvertName(charset, ...) __xadConvertName(xadMasterBase, (charset), __VA_ARGS__)
#endif

#endif /*  _VBCCINLINE_XADMASTER_H  */
