/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Funktion table for IFFParse
        Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,IFFParse) (void);
void AROS_SLIB_ENTRY(close,IFFParse) (void);
void AROS_SLIB_ENTRY(expunge,IFFParse) (void);
void AROS_SLIB_ENTRY(null,IFFParse) (void);
void AROS_SLIB_ENTRY(AllocIFF,IFFParse) (void);
void AROS_SLIB_ENTRY(OpenIFF,IFFParse) (void);
void AROS_SLIB_ENTRY(ParseIFF,IFFParse) (void);
void AROS_SLIB_ENTRY(CloseIFF,IFFParse) (void);
void AROS_SLIB_ENTRY(FreeIFF,IFFParse) (void);
void AROS_SLIB_ENTRY(ReadChunkBytes,IFFParse) (void);
void AROS_SLIB_ENTRY(WriteChunkBytes,IFFParse) (void);
void AROS_SLIB_ENTRY(ReadChunkRecords,IFFParse) (void);
void AROS_SLIB_ENTRY(WriteChunkRecords,IFFParse) (void);
void AROS_SLIB_ENTRY(PushChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(PopChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(EntryHandler,IFFParse) (void);
void AROS_SLIB_ENTRY(ExitHandler,IFFParse) (void);
void AROS_SLIB_ENTRY(PropChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(PropChunks,IFFParse) (void);
void AROS_SLIB_ENTRY(StopChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(StopChunks,IFFParse) (void);
void AROS_SLIB_ENTRY(CollectionChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(CollectionChunks,IFFParse) (void);
void AROS_SLIB_ENTRY(StopOnExit,IFFParse) (void);
void AROS_SLIB_ENTRY(FindProp,IFFParse) (void);
void AROS_SLIB_ENTRY(FindCollection,IFFParse) (void);
void AROS_SLIB_ENTRY(FindPropContext,IFFParse) (void);
void AROS_SLIB_ENTRY(CurrentChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(ParentChunk,IFFParse) (void);
void AROS_SLIB_ENTRY(AllocLocalItem,IFFParse) (void);
void AROS_SLIB_ENTRY(LocalItemData,IFFParse) (void);
void AROS_SLIB_ENTRY(SetLocalItemPurge,IFFParse) (void);
void AROS_SLIB_ENTRY(FreeLocalItem,IFFParse) (void);
void AROS_SLIB_ENTRY(FindLocalItem,IFFParse) (void);
void AROS_SLIB_ENTRY(StoreLocalItem,IFFParse) (void);
void AROS_SLIB_ENTRY(StoreItemInContext,IFFParse) (void);
void AROS_SLIB_ENTRY(InitIFF,IFFParse) (void);
void AROS_SLIB_ENTRY(InitIFFasDOS,IFFParse) (void);
void AROS_SLIB_ENTRY(InitIFFasClip,IFFParse) (void);
void AROS_SLIB_ENTRY(OpenClipboard,IFFParse) (void);
void AROS_SLIB_ENTRY(CloseClipboard,IFFParse) (void);
void AROS_SLIB_ENTRY(GoodID,IFFParse) (void);
void AROS_SLIB_ENTRY(GoodType,IFFParse) (void);
void AROS_SLIB_ENTRY(IDtoStr,IFFParse) (void);

void *const IFFParse_functable[]=
{
    AROS_SLIB_ENTRY(open,IFFParse), /* 1 */
    AROS_SLIB_ENTRY(close,IFFParse), /* 2 */
    AROS_SLIB_ENTRY(expunge,IFFParse), /* 3 */
    AROS_SLIB_ENTRY(null,IFFParse), /* 4 */
    AROS_SLIB_ENTRY(AllocIFF,IFFParse), /* 5 */
    AROS_SLIB_ENTRY(OpenIFF,IFFParse), /* 6 */
    AROS_SLIB_ENTRY(ParseIFF,IFFParse), /* 7 */
    AROS_SLIB_ENTRY(CloseIFF,IFFParse), /* 8 */
    AROS_SLIB_ENTRY(FreeIFF,IFFParse), /* 9 */
    AROS_SLIB_ENTRY(ReadChunkBytes,IFFParse), /* 10 */
    AROS_SLIB_ENTRY(WriteChunkBytes,IFFParse), /* 11 */
    AROS_SLIB_ENTRY(ReadChunkRecords,IFFParse), /* 12 */
    AROS_SLIB_ENTRY(WriteChunkRecords,IFFParse), /* 13 */
    AROS_SLIB_ENTRY(PushChunk,IFFParse), /* 14 */
    AROS_SLIB_ENTRY(PopChunk,IFFParse), /* 15 */
    NULL, /* 16 */
    AROS_SLIB_ENTRY(EntryHandler,IFFParse), /* 17 */
    AROS_SLIB_ENTRY(ExitHandler,IFFParse), /* 18 */
    AROS_SLIB_ENTRY(PropChunk,IFFParse), /* 19 */
    AROS_SLIB_ENTRY(PropChunks,IFFParse), /* 20 */
    AROS_SLIB_ENTRY(StopChunk,IFFParse), /* 21 */
    AROS_SLIB_ENTRY(StopChunks,IFFParse), /* 22 */
    AROS_SLIB_ENTRY(CollectionChunk,IFFParse), /* 23 */
    AROS_SLIB_ENTRY(CollectionChunks,IFFParse), /* 24 */
    AROS_SLIB_ENTRY(StopOnExit,IFFParse), /* 25 */
    AROS_SLIB_ENTRY(FindProp,IFFParse), /* 26 */
    AROS_SLIB_ENTRY(FindCollection,IFFParse), /* 27 */
    AROS_SLIB_ENTRY(FindPropContext,IFFParse), /* 28 */
    AROS_SLIB_ENTRY(CurrentChunk,IFFParse), /* 29 */
    AROS_SLIB_ENTRY(ParentChunk,IFFParse), /* 30 */
    AROS_SLIB_ENTRY(AllocLocalItem,IFFParse), /* 31 */
    AROS_SLIB_ENTRY(LocalItemData,IFFParse), /* 32 */
    AROS_SLIB_ENTRY(SetLocalItemPurge,IFFParse), /* 33 */
    AROS_SLIB_ENTRY(FreeLocalItem,IFFParse), /* 34 */
    AROS_SLIB_ENTRY(FindLocalItem,IFFParse), /* 35 */
    AROS_SLIB_ENTRY(StoreLocalItem,IFFParse), /* 36 */
    AROS_SLIB_ENTRY(StoreItemInContext,IFFParse), /* 37 */
    AROS_SLIB_ENTRY(InitIFF,IFFParse), /* 38 */
    AROS_SLIB_ENTRY(InitIFFasDOS,IFFParse), /* 39 */
    AROS_SLIB_ENTRY(InitIFFasClip,IFFParse), /* 40 */
    AROS_SLIB_ENTRY(OpenClipboard,IFFParse), /* 41 */
    AROS_SLIB_ENTRY(CloseClipboard,IFFParse), /* 42 */
    AROS_SLIB_ENTRY(GoodID,IFFParse), /* 43 */
    AROS_SLIB_ENTRY(GoodType,IFFParse), /* 44 */
    AROS_SLIB_ENTRY(IDtoStr,IFFParse), /* 45 */
    (void *)-1L
};

char IFFParse_end;
