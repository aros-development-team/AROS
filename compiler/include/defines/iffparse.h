#ifndef DEFINES_IFFPARSE_H
#define DEFINES_IFFPARSE_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AllocIFF() \
    AROS_LC0(struct IFFHandle *, AllocIFF, \
    struct Library *, IFFParseBase, 5, Iffparse)

#define AllocLocalItem(type, id, ident, dataSize) \
    AROS_LC4(struct LocalContextItem *, AllocLocalItem, \
    AROS_LCA(LONG, type, D0), \
    AROS_LCA(LONG, id, D1), \
    AROS_LCA(LONG, ident, D2), \
    AROS_LCA(ULONG, dataSize, D3), \
    struct Library *, IFFParseBase, 31, Iffparse)

#define CloseClipboard(clipHandle) \
    AROS_LC1(void, CloseClipboard, \
    AROS_LCA(struct ClipboardHandle *, clipHandle, A0), \
    struct Library *, IFFParseBase, 42, Iffparse)

#define CloseIFF(iff) \
    AROS_LC1(void, CloseIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 8, Iffparse)

#define CollectionChunk(iff, type, id) \
    AROS_LC3(LONG, CollectionChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 23, Iffparse)

#define CollectionChunks(iff, propArray, numPairs) \
    AROS_LC3(LONG, CollectionChunks, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG             *, propArray, A1), \
    AROS_LCA(LONG              , numPairs, D0), \
    struct Library *, IFFParseBase, 24, Iffparse)

#define CurrentChunk(iff) \
    AROS_LC1(struct ContextNode *, CurrentChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 29, Iffparse)

#define EntryHandler(iff, type, id, position, handler, object) \
    AROS_LC6(LONG, EntryHandler, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , position, D2), \
    AROS_LCA(struct Hook      *, handler, A1), \
    AROS_LCA(APTR              , object, A2), \
    struct Library *, IFFParseBase, 17, Iffparse)

#define ExitHandler(iff, type, id, position, handler, object) \
    AROS_LC6(LONG, ExitHandler, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , position, D2), \
    AROS_LCA(struct Hook      *, handler, A1), \
    AROS_LCA(APTR              , object, A2), \
    struct Library *, IFFParseBase, 18, Iffparse)

#define FindCollection(iff, type, id) \
    AROS_LC3(struct CollectionItem *, FindCollection, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 27, Iffparse)

#define FindLocalItem(iff, type, id, ident) \
    AROS_LC4(struct LocalContextItem *, FindLocalItem, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , ident, D2), \
    struct Library *, IFFParseBase, 35, Iffparse)

#define FindProp(iff, type, id) \
    AROS_LC3(struct StoredProperty *, FindProp, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 26, Iffparse)

#define FindPropContext(iff) \
    AROS_LC1(struct ContextNode *, FindPropContext, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 28, Iffparse)

#define FreeIFF(iff) \
    AROS_LC1(void, FreeIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 9, Iffparse)

#define FreeLocalItem(localItem) \
    AROS_LC1(void, FreeLocalItem, \
    AROS_LCA(struct LocalContextItem *, localItem, A0), \
    struct Library *, IFFParseBase, 34, Iffparse)

#define GoodID(id) \
    AROS_LC1(LONG, GoodID, \
    AROS_LCA(LONG, id, D0), \
    struct Library *, IFFParseBase, 43, Iffparse)

#define GoodType(type) \
    AROS_LC1(LONG, GoodType, \
    AROS_LCA(LONG, type, D0), \
    struct Library *, IFFParseBase, 44, Iffparse)

#define IDtoStr(id, buf) \
    AROS_LC2(STRPTR, IDtoStr, \
    AROS_LCA(LONG  , id, D0), \
    AROS_LCA(STRPTR, buf, A0), \
    struct Library *, IFFParseBase, 45, Iffparse)

#define InitIFF(iff, flags, streamHook) \
    AROS_LC3(void, InitIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , flags, D0), \
    AROS_LCA(struct Hook      *, streamHook, A1), \
    struct Library *, IFFParseBase, 38, Iffparse)

#define InitIFFasClip(iff) \
    AROS_LC1(void, InitIFFasClip, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 40, Iffparse)

#define InitIFFasDOS(iff) \
    AROS_LC1(void, InitIFFasDOS, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 39, Iffparse)

#define LocalItemData(localItem) \
    AROS_LC1(APTR, LocalItemData, \
    AROS_LCA(struct LocalContextItem *, localItem, A0), \
    struct Library *, IFFParseBase, 32, Iffparse)

#define OpenClipboard(unitNumber) \
    AROS_LC1(struct ClipboardHandle *, OpenClipboard, \
    AROS_LCA(LONG, unitNumber, D0), \
    struct Library *, IFFParseBase, 41, Iffparse)

#define OpenIFF(iff, rwMode) \
    AROS_LC2(LONG, OpenIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , rwMode, D0), \
    struct Library *, IFFParseBase, 6, Iffparse)

#define ParentChunk(contextNode) \
    AROS_LC1(struct ContextNode *, ParentChunk, \
    AROS_LCA(struct ContextNode *, contextNode, A0), \
    struct Library *, IFFParseBase, 30, Iffparse)

#define ParseIFF(iff, mode) \
    AROS_LC2(LONG, ParseIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , mode, D0), \
    struct Library *, IFFParseBase, 7, Iffparse)

#define PopChunk(iff) \
    AROS_LC1(LONG, PopChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 15, Iffparse)

#define PropChunk(iff, type, id) \
    AROS_LC3(LONG, PropChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 19, Iffparse)

#define PropChunks(iff, propArray, numPairs) \
    AROS_LC3(LONG, PropChunks, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG             *, propArray, A1), \
    AROS_LCA(LONG              , numPairs, D0), \
    struct Library *, IFFParseBase, 20, Iffparse)

#define PushChunk(iff, type, id, size) \
    AROS_LC4(LONG, PushChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , size, D2), \
    struct Library *, IFFParseBase, 14, Iffparse)

#define ReadChunkBytes(iff, buf, numBytes) \
    AROS_LC3(LONG, ReadChunkBytes, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , numBytes, D0), \
    struct Library *, IFFParseBase, 10, Iffparse)

#define ReadChunkRecords(iff, buf, bytesPerRecord, numRecords) \
    AROS_LC4(LONG, ReadChunkRecords, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , bytesPerRecord, D0), \
    AROS_LCA(LONG              , numRecords, D1), \
    struct Library *, IFFParseBase, 12, Iffparse)

#define SetLocalItemPurge(localItem, purgeHook) \
    AROS_LC2(void, SetLocalItemPurge, \
    AROS_LCA(struct LocalContextItem *, localItem, A0), \
    AROS_LCA(struct Hook             *, purgeHook, A1), \
    struct Library *, IFFParseBase, 33, Iffparse)

#define StopChunk(iff, type, id) \
    AROS_LC3(LONG, StopChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 21, Iffparse)

#define StopChunks(iff, propArray, numPairs) \
    AROS_LC3(LONG, StopChunks, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG             *, propArray, A1), \
    AROS_LCA(LONG              , numPairs, D0), \
    struct Library *, IFFParseBase, 22, Iffparse)

#define StopOnExit(iff, type, id) \
    AROS_LC3(LONG, StopOnExit, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 25, Iffparse)

#define StoreItemInContext(iff, localItem, contextNode) \
    AROS_LC3(void, StoreItemInContext, \
    AROS_LCA(struct IFFHandle        *, iff, A0), \
    AROS_LCA(struct LocalContextItem *, localItem, A1), \
    AROS_LCA(struct ContextNode      *, contextNode, A2), \
    struct Library *, IFFParseBase, 37, Iffparse)

#define StoreLocalItem(iff, localItem, position) \
    AROS_LC3(LONG, StoreLocalItem, \
    AROS_LCA(struct IFFHandle        *, iff, A0), \
    AROS_LCA(struct LocalContextItem *, localItem, A1), \
    AROS_LCA(LONG                     , position, D0), \
    struct Library *, IFFParseBase, 36, Iffparse)

#define WriteChunkBytes(iff, buf, numBytes) \
    AROS_LC3(LONG, WriteChunkBytes, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , numBytes, D0), \
    struct Library *, IFFParseBase, 11, Iffparse)

#define WriteChunkRecords(iff, buf, bytesPerRecord, numRecords) \
    AROS_LC4(LONG, WriteChunkRecords, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , bytesPerRecord, D0), \
    AROS_LCA(LONG              , numRecords, D1), \
    struct Library *, IFFParseBase, 13, Iffparse)


#endif /* DEFINES_IFFPARSE_H */
