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
    struct Library *, IFFParseBase, 5, IFFParse)

AROS_LP4(struct LocalContextItem *, AllocLocalItem,
    AROS_LPA(LONG, type, D0),
    AROS_LPA(LONG, id, D1),
    AROS_LPA(LONG, ident, D2),
    AROS_LPA(ULONG, dataSize, D3),
    struct Library *, IFFParseBase, 31, IFFParse)

AROS_LP1(void, CloseClipboard,
    AROS_LPA(struct ClipboardHandle *, clipHandle, A0),
    struct Library *, IFFParseBase, 42, IFFParse)

AROS_LP1(void, CloseIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 8, IFFParse)

AROS_LP3(LONG, CollectionChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 23, IFFParse)

AROS_LP3(LONG, CollectionChunks,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG             *, propArray, A1),
    AROS_LPA(LONG              , numPairs, D0),
    struct Library *, IFFParseBase, 24, IFFParse)

AROS_LP1(struct ContextNode *, CurrentChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 29, IFFParse)

AROS_LP6(LONG, EntryHandler,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , position, D2),
    AROS_LPA(struct Hook      *, handler, A1),
    AROS_LPA(APTR              , object, A2),
    struct Library *, IFFParseBase, 17, IFFParse)

AROS_LP6(LONG, ExitHandler,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , position, D2),
    AROS_LPA(struct Hook      *, handler, A1),
    AROS_LPA(APTR              , object, A2),
    struct Library *, IFFParseBase, 18, IFFParse)

AROS_LP3(struct CollectionItem *, FindCollection,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 27, IFFParse)

AROS_LP4(struct LocalContextItem *, FindLocalItem,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , ident, D2),
    struct Library *, IFFParseBase, 35, IFFParse)

AROS_LP3(struct StoredProperty *, FindProp,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 26, IFFParse)

AROS_LP1(struct ContextNode *, FindPropContext,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 28, IFFParse)

AROS_LP1(void, FreeIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 9, IFFParse)

AROS_LP1(void, FreeLocalItem,
    AROS_LPA(struct LocalContextItem *, localItem, A0),
    struct Library *, IFFParseBase, 34, IFFParse)

AROS_LP1(LONG, GoodID,
    AROS_LPA(LONG, id, D0),
    struct Library *, IFFParseBase, 43, IFFParse)

AROS_LP1(LONG, GoodType,
    AROS_LPA(LONG, type, D0),
    struct Library *, IFFParseBase, 44, IFFParse)

AROS_LP2(STRPTR, IDtoStr,
    AROS_LPA(LONG  , id, D0),
    AROS_LPA(STRPTR, buf, A0),
    struct Library *, IFFParseBase, 45, IFFParse)

AROS_LP3(void, InitIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , flags, D0),
    AROS_LPA(struct Hook      *, streamHook, A1),
    struct Library *, IFFParseBase, 38, IFFParse)

AROS_LP1(void, InitIFFasClip,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 40, IFFParse)

AROS_LP1(void, InitIFFasDOS,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 39, IFFParse)

AROS_LP1(APTR, LocalItemData,
    AROS_LPA(struct LocalContextItem *, localItem, A0),
    struct Library *, IFFParseBase, 32, IFFParse)

AROS_LP1(struct ClipboardHandle *, OpenClipboard,
    AROS_LPA(LONG, unitNumber, D0),
    struct Library *, IFFParseBase, 41, IFFParse)

AROS_LP2(LONG, OpenIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , rwMode, D0),
    struct Library *, IFFParseBase, 6, IFFParse)

AROS_LP1(struct ContextNode *, ParentChunk,
    AROS_LPA(struct ContextNode *, contextNode, A0),
    struct Library *, IFFParseBase, 30, IFFParse)

AROS_LP2(LONG, ParseIFF,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , mode, D0),
    struct Library *, IFFParseBase, 7, IFFParse)

AROS_LP1(LONG, PopChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    struct Library *, IFFParseBase, 15, IFFParse)

AROS_LP3(LONG, PropChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 19, IFFParse)

AROS_LP3(LONG, PropChunks,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG             *, propArray, A1),
    AROS_LPA(LONG              , numPairs, D0),
    struct Library *, IFFParseBase, 20, IFFParse)

AROS_LP4(LONG, PushChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    AROS_LPA(LONG              , size, D2),
    struct Library *, IFFParseBase, 14, IFFParse)

AROS_LP3(LONG, ReadChunkBytes,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , numBytes, D0),
    struct Library *, IFFParseBase, 10, IFFParse)

AROS_LP4(LONG, ReadChunkRecords,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , bytesPerRecord, D0),
    AROS_LPA(LONG              , numRecords, D1),
    struct Library *, IFFParseBase, 12, IFFParse)

AROS_LP2(void, SetLocalItemPurge,
    AROS_LPA(struct LocalContextItem *, localItem, A0),
    AROS_LPA(struct Hook             *, purgeHook, A1),
    struct Library *, IFFParseBase, 33, IFFParse)

AROS_LP3(LONG, StopChunk,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 21, IFFParse)

AROS_LP3(LONG, StopChunks,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG             *, propArray, A1),
    AROS_LPA(LONG              , numPairs, D0),
    struct Library *, IFFParseBase, 22, IFFParse)

AROS_LP3(LONG, StopOnExit,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(LONG              , type, D0),
    AROS_LPA(LONG              , id, D1),
    struct Library *, IFFParseBase, 25, IFFParse)

AROS_LP3(void, StoreItemInContext,
    AROS_LPA(struct IFFHandle        *, iff, A0),
    AROS_LPA(struct LocalContextItem *, localItem, A1),
    AROS_LPA(struct ContextNode      *, contextNode, A2),
    struct Library *, IFFParseBase, 37, IFFParse)

AROS_LP3(LONG, StoreLocalItem,
    AROS_LPA(struct IFFHandle        *, iff, A0),
    AROS_LPA(struct LocalContextItem *, localItem, A1),
    AROS_LPA(LONG                     , position, D0),
    struct Library *, IFFParseBase, 36, IFFParse)

AROS_LP3(LONG, WriteChunkBytes,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , numBytes, D0),
    struct Library *, IFFParseBase, 11, IFFParse)

AROS_LP4(LONG, WriteChunkRecords,
    AROS_LPA(struct IFFHandle *, iff, A0),
    AROS_LPA(APTR              , buf, A1),
    AROS_LPA(LONG              , bytesPerRecord, D0),
    AROS_LPA(LONG              , numRecords, D1),
    struct Library *, IFFParseBase, 13, IFFParse)


#endif /* CLIB_IFFPARSE_PROTOS_H */
