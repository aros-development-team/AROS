#ifndef _INLINE_IFFPARSE_H
#define _INLINE_IFFPARSE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef IFFPARSE_BASE_NAME
#define IFFPARSE_BASE_NAME IFFParseBase
#endif

#define AllocIFF() \
	LP0(0x1e, struct IFFHandle *, AllocIFF, \
	, IFFPARSE_BASE_NAME)

#define AllocLocalItem(type, id, ident, dataSize) \
	LP4(0xba, struct LocalContextItem *, AllocLocalItem, long, type, d0, long, id, d1, long, ident, d2, long, dataSize, d3, \
	, IFFPARSE_BASE_NAME)

#define CloseClipboard(clipHandle) \
	LP1NR(0xfc, CloseClipboard, struct ClipboardHandle *, clipHandle, a0, \
	, IFFPARSE_BASE_NAME)

#define CloseIFF(iff) \
	LP1NR(0x30, CloseIFF, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define CollectionChunk(iff, type, id) \
	LP3(0x8a, LONG, CollectionChunk, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, \
	, IFFPARSE_BASE_NAME)

#define CollectionChunks(iff, propArray, numPairs) \
	LP3(0x90, LONG, CollectionChunks, struct IFFHandle *, iff, a0, LONG *, propArray, a1, long, numPairs, d0, \
	, IFFPARSE_BASE_NAME)

#define CurrentChunk(iff) \
	LP1(0xae, struct ContextNode *, CurrentChunk, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define EntryHandler(iff, type, id, position, handler, object) \
	LP6(0x66, LONG, EntryHandler, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, long, position, d2, struct Hook *, handler, a1, APTR, object, a2, \
	, IFFPARSE_BASE_NAME)

#define ExitHandler(iff, type, id, position, handler, object) \
	LP6(0x6c, LONG, ExitHandler, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, long, position, d2, struct Hook *, handler, a1, APTR, object, a2, \
	, IFFPARSE_BASE_NAME)

#define FindCollection(iff, type, id) \
	LP3(0xa2, struct CollectionItem *, FindCollection, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, \
	, IFFPARSE_BASE_NAME)

#define FindLocalItem(iff, type, id, ident) \
	LP4(0xd2, struct LocalContextItem *, FindLocalItem, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, long, ident, d2, \
	, IFFPARSE_BASE_NAME)

#define FindProp(iff, type, id) \
	LP3(0x9c, struct StoredProperty *, FindProp, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, \
	, IFFPARSE_BASE_NAME)

#define FindPropContext(iff) \
	LP1(0xa8, struct ContextNode *, FindPropContext, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define FreeIFF(iff) \
	LP1NR(0x36, FreeIFF, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define FreeLocalItem(localItem) \
	LP1NR(0xcc, FreeLocalItem, struct LocalContextItem *, localItem, a0, \
	, IFFPARSE_BASE_NAME)

#define GoodID(id) \
	LP1(0x102, LONG, GoodID, long, id, d0, \
	, IFFPARSE_BASE_NAME)

#define GoodType(type) \
	LP1(0x108, LONG, GoodType, long, type, d0, \
	, IFFPARSE_BASE_NAME)

#define IDtoStr(id, buf) \
	LP2(0x10e, STRPTR, IDtoStr, long, id, d0, STRPTR, buf, a0, \
	, IFFPARSE_BASE_NAME)

#define InitIFF(iff, flags, streamHook) \
	LP3NR(0xe4, InitIFF, struct IFFHandle *, iff, a0, long, flags, d0, struct Hook *, streamHook, a1, \
	, IFFPARSE_BASE_NAME)

#define InitIFFasClip(iff) \
	LP1NR(0xf0, InitIFFasClip, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define InitIFFasDOS(iff) \
	LP1NR(0xea, InitIFFasDOS, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define LocalItemData(localItem) \
	LP1(0xc0, APTR, LocalItemData, struct LocalContextItem *, localItem, a0, \
	, IFFPARSE_BASE_NAME)

#define OpenClipboard(unitNumber) \
	LP1(0xf6, struct ClipboardHandle *, OpenClipboard, long, unitNumber, d0, \
	, IFFPARSE_BASE_NAME)

#define OpenIFF(iff, rwMode) \
	LP2(0x24, LONG, OpenIFF, struct IFFHandle *, iff, a0, long, rwMode, d0, \
	, IFFPARSE_BASE_NAME)

#define ParentChunk(contextNode) \
	LP1(0xb4, struct ContextNode *, ParentChunk, struct ContextNode *, contextNode, a0, \
	, IFFPARSE_BASE_NAME)

#define ParseIFF(iff, control) \
	LP2(0x2a, LONG, ParseIFF, struct IFFHandle *, iff, a0, long, control, d0, \
	, IFFPARSE_BASE_NAME)

#define PopChunk(iff) \
	LP1(0x5a, LONG, PopChunk, struct IFFHandle *, iff, a0, \
	, IFFPARSE_BASE_NAME)

#define PropChunk(iff, type, id) \
	LP3(0x72, LONG, PropChunk, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, \
	, IFFPARSE_BASE_NAME)

#define PropChunks(iff, propArray, numPairs) \
	LP3(0x78, LONG, PropChunks, struct IFFHandle *, iff, a0, LONG *, propArray, a1, long, numPairs, d0, \
	, IFFPARSE_BASE_NAME)

#define PushChunk(iff, type, id, size) \
	LP4(0x54, LONG, PushChunk, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, long, size, d2, \
	, IFFPARSE_BASE_NAME)

#define ReadChunkBytes(iff, buf, numBytes) \
	LP3(0x3c, LONG, ReadChunkBytes, struct IFFHandle *, iff, a0, APTR, buf, a1, long, numBytes, d0, \
	, IFFPARSE_BASE_NAME)

#define ReadChunkRecords(iff, buf, bytesPerRecord, numRecords) \
	LP4(0x48, LONG, ReadChunkRecords, struct IFFHandle *, iff, a0, APTR, buf, a1, long, bytesPerRecord, d0, long, numRecords, d1, \
	, IFFPARSE_BASE_NAME)

#define SetLocalItemPurge(localItem, purgeHook) \
	LP2NR(0xc6, SetLocalItemPurge, struct LocalContextItem *, localItem, a0, struct Hook *, purgeHook, a1, \
	, IFFPARSE_BASE_NAME)

#define StopChunk(iff, type, id) \
	LP3(0x7e, LONG, StopChunk, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, \
	, IFFPARSE_BASE_NAME)

#define StopChunks(iff, propArray, numPairs) \
	LP3(0x84, LONG, StopChunks, struct IFFHandle *, iff, a0, LONG *, propArray, a1, long, numPairs, d0, \
	, IFFPARSE_BASE_NAME)

#define StopOnExit(iff, type, id) \
	LP3(0x96, LONG, StopOnExit, struct IFFHandle *, iff, a0, long, type, d0, long, id, d1, \
	, IFFPARSE_BASE_NAME)

#define StoreItemInContext(iff, localItem, contextNode) \
	LP3NR(0xde, StoreItemInContext, struct IFFHandle *, iff, a0, struct LocalContextItem *, localItem, a1, struct ContextNode *, contextNode, a2, \
	, IFFPARSE_BASE_NAME)

#define StoreLocalItem(iff, localItem, position) \
	LP3(0xd8, LONG, StoreLocalItem, struct IFFHandle *, iff, a0, struct LocalContextItem *, localItem, a1, long, position, d0, \
	, IFFPARSE_BASE_NAME)

#define WriteChunkBytes(iff, buf, numBytes) \
	LP3(0x42, LONG, WriteChunkBytes, struct IFFHandle *, iff, a0, APTR, buf, a1, long, numBytes, d0, \
	, IFFPARSE_BASE_NAME)

#define WriteChunkRecords(iff, buf, bytesPerRecord, numRecords) \
	LP4(0x4e, LONG, WriteChunkRecords, struct IFFHandle *, iff, a0, APTR, buf, a1, long, bytesPerRecord, d0, long, numRecords, d1, \
	, IFFPARSE_BASE_NAME)

#endif /* _INLINE_IFFPARSE_H */
