/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    *** Automatic generated file. Do not edit ***
    Desc: Function table for Utility
    Lang: english
*/
#ifndef LIBCORE_COMPILER_H
#   include <libcore/compiler.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

#include "libdefs.h"
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader) (void);
extern void AROS_SLIB_ENTRY(FindTagItem,BASENAME) (void);
extern void AROS_SLIB_ENTRY(GetTagData,BASENAME) (void);
extern void AROS_SLIB_ENTRY(PackBoolTags,BASENAME) (void);
extern void AROS_SLIB_ENTRY(NextTagItem,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FilterTagChanges,BASENAME) (void);
extern void AROS_SLIB_ENTRY(MapTags,BASENAME) (void);
extern void AROS_SLIB_ENTRY(AllocateTagItems,BASENAME) (void);
extern void AROS_SLIB_ENTRY(CloneTagItems,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FreeTagItems,BASENAME) (void);
extern void AROS_SLIB_ENTRY(RefreshTagItemClones,BASENAME) (void);
extern void AROS_SLIB_ENTRY(TagInArray,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FilterTagItems,BASENAME) (void);
extern void AROS_SLIB_ENTRY(CallHookPkt,BASENAME) (void);
extern void AROS_SLIB_ENTRY(Amiga2Date,BASENAME) (void);
extern void AROS_SLIB_ENTRY(Date2Amiga,BASENAME) (void);
extern void AROS_SLIB_ENTRY(CheckDate,BASENAME) (void);
extern void AROS_SLIB_ENTRY(SMult32,BASENAME) (void);
extern void AROS_SLIB_ENTRY(UMult32,BASENAME) (void);
extern void AROS_SLIB_ENTRY(SDivMod32,BASENAME) (void);
extern void AROS_SLIB_ENTRY(UDivMod32,BASENAME) (void);
extern void AROS_SLIB_ENTRY(Stricmp,BASENAME) (void);
extern void AROS_SLIB_ENTRY(Strnicmp,BASENAME) (void);
extern void AROS_SLIB_ENTRY(ToUpper,BASENAME) (void);
extern void AROS_SLIB_ENTRY(ToLower,BASENAME) (void);
extern void AROS_SLIB_ENTRY(ApplyTagChanges,BASENAME) (void);
extern void AROS_SLIB_ENTRY(SMult64,BASENAME) (void);
extern void AROS_SLIB_ENTRY(UMult64,BASENAME) (void);
extern void AROS_SLIB_ENTRY(PackStructureTags,BASENAME) (void);
extern void AROS_SLIB_ENTRY(UnpackStructureTags,BASENAME) (void);
extern void AROS_SLIB_ENTRY(AddNamedObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(AllocNamedObjectA,BASENAME) (void);
extern void AROS_SLIB_ENTRY(AttemptRemNamedObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FindNamedObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(FreeNamedObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(NamedObjectName,BASENAME) (void);
extern void AROS_SLIB_ENTRY(ReleaseNamedObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(RemNamedObject,BASENAME) (void);
extern void AROS_SLIB_ENTRY(GetUniqueID,BASENAME) (void);

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),
    AROS_SLIB_ENTRY(FindTagItem,BASENAME), /* 5 */
    AROS_SLIB_ENTRY(GetTagData,BASENAME), /* 6 */
    AROS_SLIB_ENTRY(PackBoolTags,BASENAME), /* 7 */
    AROS_SLIB_ENTRY(NextTagItem,BASENAME), /* 8 */
    AROS_SLIB_ENTRY(FilterTagChanges,BASENAME), /* 9 */
    AROS_SLIB_ENTRY(MapTags,BASENAME), /* 10 */
    AROS_SLIB_ENTRY(AllocateTagItems,BASENAME), /* 11 */
    AROS_SLIB_ENTRY(CloneTagItems,BASENAME), /* 12 */
    AROS_SLIB_ENTRY(FreeTagItems,BASENAME), /* 13 */
    AROS_SLIB_ENTRY(RefreshTagItemClones,BASENAME), /* 14 */
    AROS_SLIB_ENTRY(TagInArray,BASENAME), /* 15 */
    AROS_SLIB_ENTRY(FilterTagItems,BASENAME), /* 16 */
    AROS_SLIB_ENTRY(CallHookPkt,BASENAME), /* 17 */
    NULL, /* 18 */
    NULL, /* 19 */
    AROS_SLIB_ENTRY(Amiga2Date,BASENAME), /* 20 */
    AROS_SLIB_ENTRY(Date2Amiga,BASENAME), /* 21 */
    AROS_SLIB_ENTRY(CheckDate,BASENAME), /* 22 */
    AROS_SLIB_ENTRY(SMult32,BASENAME), /* 23 */
    AROS_SLIB_ENTRY(UMult32,BASENAME), /* 24 */
    AROS_SLIB_ENTRY(SDivMod32,BASENAME), /* 25 */
    AROS_SLIB_ENTRY(UDivMod32,BASENAME), /* 26 */
    AROS_SLIB_ENTRY(Stricmp,BASENAME), /* 27 */
    AROS_SLIB_ENTRY(Strnicmp,BASENAME), /* 28 */
    AROS_SLIB_ENTRY(ToUpper,BASENAME), /* 29 */
    AROS_SLIB_ENTRY(ToLower,BASENAME), /* 30 */
    AROS_SLIB_ENTRY(ApplyTagChanges,BASENAME), /* 31 */
    NULL, /* 32 */
    AROS_SLIB_ENTRY(SMult64,BASENAME), /* 33 */
    AROS_SLIB_ENTRY(UMult64,BASENAME), /* 34 */
    AROS_SLIB_ENTRY(PackStructureTags,BASENAME), /* 35 */
    AROS_SLIB_ENTRY(UnpackStructureTags,BASENAME), /* 36 */
    AROS_SLIB_ENTRY(AddNamedObject,BASENAME), /* 37 */
    AROS_SLIB_ENTRY(AllocNamedObjectA,BASENAME), /* 38 */
    AROS_SLIB_ENTRY(AttemptRemNamedObject,BASENAME), /* 39 */
    AROS_SLIB_ENTRY(FindNamedObject,BASENAME), /* 40 */
    AROS_SLIB_ENTRY(FreeNamedObject,BASENAME), /* 41 */
    AROS_SLIB_ENTRY(NamedObjectName,BASENAME), /* 42 */
    AROS_SLIB_ENTRY(ReleaseNamedObject,BASENAME), /* 43 */
    AROS_SLIB_ENTRY(RemNamedObject,BASENAME), /* 44 */
    AROS_SLIB_ENTRY(GetUniqueID,BASENAME), /* 45 */
    (void *)-1L
};
