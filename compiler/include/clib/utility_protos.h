#ifndef CLIB_UTILITY_PROTOS_H
#define CLIB_UTILITY_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP1(struct TagItem *, AllocateTagItems,
    AROS_LPA(unsigned long, numTags, D0),
    struct UtilityBase *, UtilityBase, 11, Utility)
#define AllocateTagItems(numTags) \
    AROS_LC1(struct TagItem *, AllocateTagItems, \
    AROS_LCA(unsigned long, numTags, D0), \
    struct UtilityBase *, UtilityBase, 11, Utility)

AROS_LP2(void, Amiga2Date,
    AROS_LPA(unsigned long     , seconds, D0),
    AROS_LPA(struct ClockData *, result, A0),
    struct Library *, UtilityBase, 20, Utility)
#define Amiga2Date(seconds, result) \
    AROS_LC2(void, Amiga2Date, \
    AROS_LCA(unsigned long     , seconds, D0), \
    AROS_LCA(struct ClockData *, result, A0), \
    struct Library *, UtilityBase, 20, Utility)

AROS_LP2(void, ApplyTagChanges,
    AROS_LPA(struct TagItem *, list,       A0),
    AROS_LPA(struct TagItem *, changelist, A1),
    struct UtilityBase *, UtilityBase, 31, Utility)
#define ApplyTagChanges(list, changelist) \
    AROS_LC2(void, ApplyTagChanges, \
    AROS_LCA(struct TagItem *, list,       A0), \
    AROS_LCA(struct TagItem *, changelist, A1), \
    struct UtilityBase *, UtilityBase, 31, Utility)

AROS_LP3(ULONG, CallHookPkt,
    AROS_LPA(struct Hook *, hook, A0),
    AROS_LPA(APTR         , object, A2),
    AROS_LPA(APTR         , paramPacket, A1),
    struct Library *, UtilityBase, 17, Utility)
#define CallHookPkt(hook, object, paramPacket) \
    AROS_LC3(ULONG, CallHookPkt, \
    AROS_LCA(struct Hook *, hook, A0), \
    AROS_LCA(APTR         , object, A2), \
    AROS_LCA(APTR         , paramPacket, A1), \
    struct Library *, UtilityBase, 17, Utility)

AROS_LP1(struct TagItem *, CloneTagItems,
    AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 12, Utility)
#define CloneTagItems(tagList) \
    AROS_LC1(struct TagItem *, CloneTagItems, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 12, Utility)

AROS_LP1(ULONG, Date2Amiga,
    AROS_LPA(struct ClockData *, date, A0),
    struct UtilityBase *, UtilityBase, 21, Utility)
#define Date2Amiga(date) \
    AROS_LC1(ULONG, Date2Amiga, \
    AROS_LCA(struct ClockData *, date, A0), \
    struct UtilityBase *, UtilityBase, 21, Utility)

AROS_LP2(struct TagItem *, FindTagItem,
    AROS_LPA(Tag,              tagValue, D0),
    AROS_LPA(struct TagItem *, tagList,  A0),
    struct UtilityBase *, UtilityBase, 5, Utility)
#define FindTagItem(tagValue, tagList) \
    AROS_LC2(struct TagItem *, FindTagItem, \
    AROS_LCA(Tag,              tagValue, D0), \
    AROS_LCA(struct TagItem *, tagList,  A0), \
    struct UtilityBase *, UtilityBase, 5, Utility)

AROS_LP1(void, FreeTagItems,
    AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 13, Utility)
#define FreeTagItems(tagList) \
    AROS_LC1(void, FreeTagItems, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 13, Utility)

AROS_LP3(ULONG, GetTagData,
    AROS_LPA(Tag             , tagValue, D0),
    AROS_LPA(unsigned long   , defaultVal, D1),
    AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 6, Utility)
#define GetTagData(tagValue, defaultVal, tagList) \
    AROS_LC3(ULONG, GetTagData, \
    AROS_LCA(Tag             , tagValue, D0), \
    AROS_LCA(unsigned long   , defaultVal, D1), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 6, Utility)

AROS_LP0(ULONG, GetUniqueID,
    struct UtilityBase *, UtilityBase, 45, Utility)
#define GetUniqueID() \
    AROS_LC0(ULONG, GetUniqueID, \
    struct UtilityBase *, UtilityBase, 45, Utility)

AROS_LP1(struct TagItem *, NextTagItem,
    AROS_LPA(struct TagItem **, tagListPtr, A0),
    struct Library *, UtilityBase, 8, Utility)
#define NextTagItem(tagListPtr) \
    AROS_LC1(struct TagItem *, NextTagItem, \
    AROS_LCA(struct TagItem **, tagListPtr, A0), \
    struct Library *, UtilityBase, 8, Utility)

AROS_LP3(ULONG, PackBoolTags,
    AROS_LPA(unsigned long   , initialFlags, D0),
    AROS_LPA(struct TagItem *, tagList, A0),
    AROS_LPA(struct TagItem *, boolMap, A1),
    struct UtilityBase *, UtilityBase, 7, Utility)
#define PackBoolTags(initialFlags, tagList, boolMap) \
    AROS_LC3(ULONG, PackBoolTags, \
    AROS_LCA(unsigned long   , initialFlags, D0), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    AROS_LCA(struct TagItem *, boolMap, A1), \
    struct UtilityBase *, UtilityBase, 7, Utility)

AROS_LP2(void, RefreshTagItemClones,
    AROS_LPA(struct TagItem *, clone, A0),
    AROS_LPA(struct TagItem *, original, A1),
    struct UtilityBase *, UtilityBase, 14, Utility)
#define RefreshTagItemClones(clone, original) \
    AROS_LC2(void, RefreshTagItemClones, \
    AROS_LCA(struct TagItem *, clone, A0), \
    AROS_LCA(struct TagItem *, original, A1), \
    struct UtilityBase *, UtilityBase, 14, Utility)

AROS_LP2(LONG, SMult32,
    AROS_LPA(long, arg1, D0),
    AROS_LPA(long, arg2, D1),
    struct UtilityBase *, UtilityBase, 23, Utility)
#define SMult32(arg1, arg2) \
    AROS_LC2(LONG, SMult32, \
    AROS_LCA(long, arg1, D0), \
    AROS_LCA(long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 23, Utility)

AROS_LP2(LONG, SMult64,
    AROS_LPA(long, arg1, D0),
    AROS_LPA(long, arg2, D1),
    struct UtilityBase *, UtilityBase, 33, Utility)
#define SMult64(arg1, arg2) \
    AROS_LC2(LONG, SMult64, \
    AROS_LCA(long, arg1, D0), \
    AROS_LCA(long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 33, Utility)

AROS_LP2(LONG, Stricmp,
    AROS_LPA(STRPTR, string1, A0),
    AROS_LPA(STRPTR, string2, A1),
    struct UtilityBase *, UtilityBase, 27, Utility)
#define Stricmp(string1, string2) \
    AROS_LC2(LONG, Stricmp, \
    AROS_LCA(STRPTR, string1, A0), \
    AROS_LCA(STRPTR, string2, A1), \
    struct UtilityBase *, UtilityBase, 27, Utility)

AROS_LP3(LONG, Strnicmp,
    AROS_LPA(STRPTR, string1, A0),
    AROS_LPA(STRPTR, string2, A1),
    AROS_LPA(LONG,   length,  D0),
    struct UtilityBase *, UtilityBase, 28, Utility)
#define Strnicmp(string1, string2, length) \
    AROS_LC3(LONG, Strnicmp, \
    AROS_LCA(STRPTR, string1, A0), \
    AROS_LCA(STRPTR, string2, A1), \
    AROS_LCA(LONG,   length,  D0), \
    struct UtilityBase *, UtilityBase, 28, Utility)

AROS_LP2(BOOL, TagInArray,
    AROS_LPA(Tag  , tagValue, D0),
    AROS_LPA(Tag *, tagArray, A0),
    struct UtilityBase *, UtilityBase, 15, Utility)
#define TagInArray(tagValue, tagArray) \
    AROS_LC2(BOOL, TagInArray, \
    AROS_LCA(Tag  , tagValue, D0), \
    AROS_LCA(Tag *, tagArray, A0), \
    struct UtilityBase *, UtilityBase, 15, Utility)

AROS_LP1I(UBYTE, ToLower,
    AROS_LPA(ULONG, character, D0),
    struct UtilityBase *, UtilityBase, 30, Utility)
#define ToLower(character) \
    AROS_LC1I(UBYTE, ToLower, \
    AROS_LCA(ULONG, character, D0), \
    struct UtilityBase *, UtilityBase, 30, Utility)

AROS_LP1I(UBYTE, ToUpper,
    AROS_LPA(unsigned long, character, D0),
    struct UtilityBase *, UtilityBase, 29, Utility)
#define ToUpper(character) \
    AROS_LC1I(UBYTE, ToUpper, \
    AROS_LCA(unsigned long, character, D0), \
    struct UtilityBase *, UtilityBase, 29, Utility)

AROS_LP2(ULONG, UMult32,
    AROS_LPA(unsigned long, arg1, D0),
    AROS_LPA(unsigned long, arg2, D1),
    struct UtilityBase *, UtilityBase, 24, Utility)
#define UMult32(arg1, arg2) \
    AROS_LC2(ULONG, UMult32, \
    AROS_LCA(unsigned long, arg1, D0), \
    AROS_LCA(unsigned long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 24, Utility)

AROS_LP2(ULONG, UMult64,
    AROS_LPA(unsigned long, arg1, D0),
    AROS_LPA(unsigned long, arg2, D1),
    struct UtilityBase *, UtilityBase, 34, Utility)
#define UMult64(arg1, arg2) \
    AROS_LC2(ULONG, UMult64, \
    AROS_LCA(unsigned long, arg1, D0), \
    AROS_LCA(unsigned long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 34, Utility)


#endif /* CLIB_UTILITY_PROTOS_H */
