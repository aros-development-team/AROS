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
    struct Library *, IFFParseBase, 5, IFFParse)

#define AllocLocalItem(type, id, ident, dataSize) \
    AROS_LC4(struct LocalContextItem *, AllocLocalItem, \
    AROS_LCA(LONG, type, D0), \
    AROS_LCA(LONG, id, D1), \
    AROS_LCA(LONG, ident, D2), \
    AROS_LCA(ULONG, dataSize, D3), \
    struct Library *, IFFParseBase, 31, IFFParse)

#define CloseClipboard(clipHandle) \
    AROS_LC1(void, CloseClipboard, \
    AROS_LCA(struct ClipboardHandle *, clipHandle, A0), \
    struct Library *, IFFParseBase, 42, IFFParse)

#define CloseIFF(iff) \
    AROS_LC1(void, CloseIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 8, IFFParse)

#define CollectionChunk(iff, type, id) \
    AROS_LC3(LONG, CollectionChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 23, IFFParse)

#define CollectionChunks(iff, propArray, numPairs) \
    AROS_LC3(LONG, CollectionChunks, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG             *, propArray, A1), \
    AROS_LCA(LONG              , numPairs, D0), \
    struct Library *, IFFParseBase, 24, IFFParse)

#define CurrentChunk(iff) \
    AROS_LC1(struct ContextNode *, CurrentChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 29, IFFParse)

#define EntryHandler(iff, type, id, position, handler, object) \
    AROS_LC6(LONG, EntryHandler, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , position, D2), \
    AROS_LCA(struct Hook      *, handler, A1), \
    AROS_LCA(APTR              , object, A2), \
    struct Library *, IFFParseBase, 17, IFFParse)

#define ExitHandler(iff, type, id, position, handler, object) \
    AROS_LC6(LONG, ExitHandler, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , position, D2), \
    AROS_LCA(struct Hook      *, handler, A1), \
    AROS_LCA(APTR              , object, A2), \
    struct Library *, IFFParseBase, 18, IFFParse)

#define FindCollection(iff, type, id) \
    AROS_LC3(struct CollectionItem *, FindCollection, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 27, IFFParse)

#define FindLocalItem(iff, type, id, ident) \
    AROS_LC4(struct LocalContextItem *, FindLocalItem, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , ident, D2), \
    struct Library *, IFFParseBase, 35, IFFParse)

#define FindProp(iff, type, id) \
    AROS_LC3(struct StoredProperty *, FindProp, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 26, IFFParse)

#define FindPropContext(iff) \
    AROS_LC1(struct ContextNode *, FindPropContext, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 28, IFFParse)

#define FreeIFF(iff) \
    AROS_LC1(void, FreeIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 9, IFFParse)

#define FreeLocalItem(localItem) \
    AROS_LC1(void, FreeLocalItem, \
    AROS_LCA(struct LocalContextItem *, localItem, A0), \
    struct Library *, IFFParseBase, 34, IFFParse)

#define GoodID(id) \
    AROS_LC1(LONG, GoodID, \
    AROS_LCA(LONG, id, D0), \
    struct Library *, IFFParseBase, 43, IFFParse)

#define GoodType(type) \
    AROS_LC1(LONG, GoodType, \
    AROS_LCA(LONG, type, D0), \
    struct Library *, IFFParseBase, 44, IFFParse)

#define IDtoStr(id, buf) \
    AROS_LC2(STRPTR, IDtoStr, \
    AROS_LCA(LONG  , id, D0), \
    AROS_LCA(STRPTR, buf, A0), \
    struct Library *, IFFParseBase, 45, IFFParse)

#define InitIFF(iff, flags, streamHook) \
    AROS_LC3(void, InitIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , flags, D0), \
    AROS_LCA(struct Hook      *, streamHook, A1), \
    struct Library *, IFFParseBase, 38, IFFParse)

#define InitIFFasClip(iff) \
    AROS_LC1(void, InitIFFasClip, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 40, IFFParse)

#define InitIFFasDOS(iff) \
    AROS_LC1(void, InitIFFasDOS, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 39, IFFParse)

#define LocalItemData(localItem) \
    AROS_LC1(APTR, LocalItemData, \
    AROS_LCA(struct LocalContextItem *, localItem, A0), \
    struct Library *, IFFParseBase, 32, IFFParse)

#define OpenClipboard(unitNumber) \
    AROS_LC1(struct ClipboardHandle *, OpenClipboard, \
    AROS_LCA(LONG, unitNumber, D0), \
    struct Library *, IFFParseBase, 41, IFFParse)

#define OpenIFF(iff, rwMode) \
    AROS_LC2(LONG, OpenIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , rwMode, D0), \
    struct Library *, IFFParseBase, 6, IFFParse)

#define ParentChunk(contextNode) \
    AROS_LC1(struct ContextNode *, ParentChunk, \
    AROS_LCA(struct ContextNode *, contextNode, A0), \
    struct Library *, IFFParseBase, 30, IFFParse)

#define ParseIFF(iff, mode) \
    AROS_LC2(LONG, ParseIFF, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , mode, D0), \
    struct Library *, IFFParseBase, 7, IFFParse)

#define PopChunk(iff) \
    AROS_LC1(LONG, PopChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    struct Library *, IFFParseBase, 15, IFFParse)

#define PropChunk(iff, type, id) \
    AROS_LC3(LONG, PropChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 19, IFFParse)

#define PropChunks(iff, propArray, numPairs) \
    AROS_LC3(LONG, PropChunks, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG             *, propArray, A1), \
    AROS_LCA(LONG              , numPairs, D0), \
    struct Library *, IFFParseBase, 20, IFFParse)

#define PushChunk(iff, type, id, size) \
    AROS_LC4(LONG, PushChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    AROS_LCA(LONG              , size, D2), \
    struct Library *, IFFParseBase, 14, IFFParse)

#define ReadChunkBytes(iff, buf, numBytes) \
    AROS_LC3(LONG, ReadChunkBytes, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , numBytes, D0), \
    struct Library *, IFFParseBase, 10, IFFParse)

#define ReadChunkRecords(iff, buf, bytesPerRecord, numRecords) \
    AROS_LC4(LONG, ReadChunkRecords, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , bytesPerRecord, D0), \
    AROS_LCA(LONG              , numRecords, D1), \
    struct Library *, IFFParseBase, 12, IFFParse)

#define SetLocalItemPurge(localItem, purgeHook) \
    AROS_LC2(void, SetLocalItemPurge, \
    AROS_LCA(struct LocalContextItem *, localItem, A0), \
    AROS_LCA(struct Hook             *, purgeHook, A1), \
    struct Library *, IFFParseBase, 33, IFFParse)

#define StopChunk(iff, type, id) \
    AROS_LC3(LONG, StopChunk, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 21, IFFParse)

#define StopChunks(iff, propArray, numPairs) \
    AROS_LC3(LONG, StopChunks, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG             *, propArray, A1), \
    AROS_LCA(LONG              , numPairs, D0), \
    struct Library *, IFFParseBase, 22, IFFParse)

#define StopOnExit(iff, type, id) \
    AROS_LC3(LONG, StopOnExit, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(LONG              , type, D0), \
    AROS_LCA(LONG              , id, D1), \
    struct Library *, IFFParseBase, 25, IFFParse)

#define StoreItemInContext(iff, localItem, contextNode) \
    AROS_LC3(void, StoreItemInContext, \
    AROS_LCA(struct IFFHandle        *, iff, A0), \
    AROS_LCA(struct LocalContextItem *, localItem, A1), \
    AROS_LCA(struct ContextNode      *, contextNode, A2), \
    struct Library *, IFFParseBase, 37, IFFParse)

#define StoreLocalItem(iff, localItem, position) \
    AROS_LC3(LONG, StoreLocalItem, \
    AROS_LCA(struct IFFHandle        *, iff, A0), \
    AROS_LCA(struct LocalContextItem *, localItem, A1), \
    AROS_LCA(LONG                     , position, D0), \
    struct Library *, IFFParseBase, 36, IFFParse)

#define WriteChunkBytes(iff, buf, numBytes) \
    AROS_LC3(LONG, WriteChunkBytes, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , numBytes, D0), \
    struct Library *, IFFParseBase, 11, IFFParse)

#define WriteChunkRecords(iff, buf, bytesPerRecord, numRecords) \
    AROS_LC4(LONG, WriteChunkRecords, \
    AROS_LCA(struct IFFHandle *, iff, A0), \
    AROS_LCA(APTR              , buf, A1), \
    AROS_LCA(LONG              , bytesPerRecord, D0), \
    AROS_LCA(LONG              , numRecords, D1), \
    struct Library *, IFFParseBase, 13, IFFParse)


#endif /* DEFINES_IFFPARSE_H */
