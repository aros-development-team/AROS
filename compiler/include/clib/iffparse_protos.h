#ifndef CLIB_IFFPARSE_PROTOS_H
#define CLIB_IFFPARSE_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for icon.library
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP0(struct IFFHandle *, AllocIFF,
    struct Library *, IFFParseBase, 5, Iffparse)

AROS_LP4(struct LocalContextItem *, AllocLocalItem,
    AROS_LPA(LONG, type, D0),
    AROS_LPA(LONG, id, D1),
    AROS_LPA(LONG, ident, D2),
    AROS_LPA(ULONG, dataSize, D3),
    struct Library *, IFFParseBase, 31, Iffparse)

AROS_LP1(void, CloseClipboard,
    AROS_LPA(struct ClipboardHandle *, clipHandle, A0),
    struct Library *, IFFParseBase, 42, Iffparse)

AROS_LP1(void, CloseIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 8, Iffparse)

AROS_LP3(LONG, CollectionChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 23, Iffparse)

AROS_LP3(LONG, CollectionChunks,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG             *, propArray, A1),
    AROS_LPA(LONG              , numPairs, D0),
    struct Library *, IFFParseBase, 24, Iffparse)

AROS_LP1(struct ContextNode *, CurrentChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 29, Iffparse)

AROS_LP6(LONG, EntryHandler,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , position, D2),
    AROS_LPA(struct Hook      *, handler, A1),
    AROS_LPA(APTR              , object, A2),
    struct Library *, IFFParseBase, 17, Iffparse)

AROS_LP6(LONG, ExitHandler,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , position, D2),
    AROS_LPA(struct Hook      *, handler, A1),
    AROS_LPA(APTR              , object, A2),
    struct Library *, IFFParseBase, 18, Iffparse)

AROS_LP3(struct CollectionItem *, FindCollection,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 27, Iffparse)

AROS_LP4(struct LocalContextItem *, FindLocalItem,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , ident, D2),
    struct Library *, IFFParseBase, 35, Iffparse)

AROS_LP3(struct StoredProperty *, FindProp,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 26, Iffparse)

AROS_LP1(struct ContextNode *, FindPropContext,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 28, Iffparse)

AROS_LP1(void, FreeIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 9, Iffparse)

AROS_LP1(void, FreeLocalItem,
    AROS_LPA(struct LocalContextItem *, localItem, A0),
    struct Library *, IFFParseBase, 34, Iffparse)

AROS_LP1(LONG, GoodID,
    AROS_LPA(LONG, id, D0),
    struct Library *, IFFParseBase, 43, Iffparse)

AROS_LP1(LONG, GoodType,
    AROS_LPA(LONG, type, D0),
    struct Library *, IFFParseBase, 44, Iffparse)

AROS_LP2(STRPTR, IDtoStr,
    AROS_LPA(LONG  , id, D0),
    AROS_LPA(STRPTR, buf, A0),
    struct Library *, IFFParseBase, 45, Iffparse)

AROS_LP3(void, InitIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , flags, D0),
    AROS_LPA(struct Hook      *, streamHook, A1),
    struct Library *, IFFParseBase, 38, Iffparse)

AROS_LP1(void, InitIFFasClip,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 40, Iffparse)

AROS_LP1(void, InitIFFasDOS,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 39, Iffparse)

AROS_LP1(APTR, LocalItemData,
    AROS_LPA(struct LocalContextItem *, localItem, A0),
    struct Library *, IFFParseBase, 32, Iffparse)

AROS_LP1(struct ClipboardHandle *, OpenClipboard,
    AROS_LPA(LONG, unitNumber, D0),
    struct Library *, IFFParseBase, 41, Iffparse)

AROS_LP2(LONG, OpenIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , rwMode, D0),
    struct Library *, IFFParseBase, 6, Iffparse)

AROS_LP1(struct ContextNode *, ParentChunk,
    AROS_LPA(struct ContextNode *, contextNode, A0),
    struct Library *, IFFParseBase, 30, Iffparse)

AROS_LP2(LONG, ParseIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , mode, D0),
    struct Library *, IFFParseBase, 7, Iffparse)

AROS_LP1(LONG, PopChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 15, Iffparse)

AROS_LP3(LONG, PropChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 19, Iffparse)

AROS_LP3(LONG, PropChunks,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG             *, propArray, A1),
    AROS_LPA(LONG              , numPairs, D0),
    struct Library *, IFFParseBase, 20, Iffparse)

AROS_LP4(LONG, PushChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , size, D2),
    struct Library *, IFFParseBase, 14, Iffparse)

AROS_LP3(LONG, ReadChunkBytes,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , numBytes, D0),
    struct Library *, IFFParseBase, 10, Iffparse)

AROS_LP4(LONG, ReadChunkRecords,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , bytesPerRecord, D0),
    AROS_LPA(LONG              , numRecords, D1),
    struct Library *, IFFParseBase, 12, Iffparse)

AROS_LP2(void, SetLocalItemPurge,
    AROS_LPA(struct LocalContextItem *, localItem, A0),
    AROS_LPA(struct Hook             *, purgeHook, A1),
    struct Library *, IFFParseBase, 33, Iffparse)

AROS_LP3(LONG, StopChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 21, Iffparse)

AROS_LP3(LONG, StopChunks,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG             *, propArray, A1),
    AROS_LPA(LONG              , numPairs, D0),
    struct Library *, IFFParseBase, 22, Iffparse)

AROS_LP3(LONG, StopOnExit,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 25, Iffparse)

AROS_LP3(void, StoreItemInContext,
    AROS_LPA(struct IFFHandle        *, iff, A0),
    AROS_LPA(struct LocalContextItem *, localItem, A1),
    AROS_LPA(struct ContextNode      *, contextNode, A2),
    struct Library *, IFFParseBase, 37, Iffparse)

AROS_LP3(LONG, StoreLocalItem,
    AROS_LPA(struct IFFHandle        *, iff, A0),
    AROS_LPA(struct LocalContextItem *, localItem, A1),
    AROS_LPA(LONG                     , position, D0),
    struct Library *, IFFParseBase, 36, Iffparse)

AROS_LP3(LONG, WriteChunkBytes,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , numBytes, D0),
    struct Library *, IFFParseBase, 11, Iffparse)

AROS_LP4(LONG, WriteChunkRecords,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , bytesPerRecord, D0),
    AROS_LPA(LONG              , numRecords, D1),
    struct Library *, IFFParseBase, 13, Iffparse)

#endif /* CLIB_IFFPARSE_PROTOS_H */
