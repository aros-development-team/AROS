#ifndef CLIB_UTILITY_PROTOS_H
#define CLIB_UTILITY_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
__AROS_LP1(struct TagItem *, AllocateTagItems,
    __AROS_LPA(unsigned long, numTags, D0),
    struct UtilityBase *, UtilityBase, 11, Utility)
#define AllocateTagItems(numTags) \
    __AROS_LC1(struct TagItem *, AllocateTagItems, \
    __AROS_LCA(unsigned long, numTags, D0), \
    struct UtilityBase *, UtilityBase, 11, Utility)

__AROS_LP2(void, Amiga2Date,
    __AROS_LPA(unsigned long     , seconds, D0),
    __AROS_LPA(struct ClockData *, result, A0),
    struct Library *, UtilityBase, 20, Utility)
#define Amiga2Date(seconds, result) \
    __AROS_LC2(void, Amiga2Date, \
    __AROS_LCA(unsigned long     , seconds, D0), \
    __AROS_LCA(struct ClockData *, result, A0), \
    struct Library *, UtilityBase, 20, Utility)

__AROS_LP2(void, ApplyTagChanges,
    __AROS_LPA(struct TagItem *, list,       A0),
    __AROS_LPA(struct TagItem *, changelist, A1),
    struct UtilityBase *, UtilityBase, 31, Utility)
#define ApplyTagChanges(list, changelist) \
    __AROS_LC2(void, ApplyTagChanges, \
    __AROS_LCA(struct TagItem *, list,       A0), \
    __AROS_LCA(struct TagItem *, changelist, A1), \
    struct UtilityBase *, UtilityBase, 31, Utility)

__AROS_LP3(ULONG, CallHookPkt,
    __AROS_LPA(struct Hook *, hook, A0),
    __AROS_LPA(APTR         , object, A2),
    __AROS_LPA(APTR         , paramPacket, A1),
    struct Library *, UtilityBase, 17, Utility)
#define CallHookPkt(hook, object, paramPacket) \
    __AROS_LC3(ULONG, CallHookPkt, \
    __AROS_LCA(struct Hook *, hook, A0), \
    __AROS_LCA(APTR         , object, A2), \
    __AROS_LCA(APTR         , paramPacket, A1), \
    struct Library *, UtilityBase, 17, Utility)

__AROS_LP1(struct TagItem *, CloneTagItems,
    __AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 12, Utility)
#define CloneTagItems(tagList) \
    __AROS_LC1(struct TagItem *, CloneTagItems, \
    __AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 12, Utility)

__AROS_LP1(ULONG, Date2Amiga,
    __AROS_LPA(struct ClockData *, date, A0),
    struct UtilityBase *, UtilityBase, 21, Utility)
#define Date2Amiga(date) \
    __AROS_LC1(ULONG, Date2Amiga, \
    __AROS_LCA(struct ClockData *, date, A0), \
    struct UtilityBase *, UtilityBase, 21, Utility)

__AROS_LP2(struct TagItem *, FindTagItem,
    __AROS_LPA(Tag,              tagValue, D0),
    __AROS_LPA(struct TagItem *, tagList,  A0),
    struct UtilityBase *, UtilityBase, 5, Utility)
#define FindTagItem(tagValue, tagList) \
    __AROS_LC2(struct TagItem *, FindTagItem, \
    __AROS_LCA(Tag,              tagValue, D0), \
    __AROS_LCA(struct TagItem *, tagList,  A0), \
    struct UtilityBase *, UtilityBase, 5, Utility)

__AROS_LP1(void, FreeTagItems,
    __AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 13, Utility)
#define FreeTagItems(tagList) \
    __AROS_LC1(void, FreeTagItems, \
    __AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 13, Utility)

__AROS_LP3(ULONG, GetTagData,
    __AROS_LPA(Tag             , tagValue, D0),
    __AROS_LPA(unsigned long   , defaultVal, D1),
    __AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 6, Utility)
#define GetTagData(tagValue, defaultVal, tagList) \
    __AROS_LC3(ULONG, GetTagData, \
    __AROS_LCA(Tag             , tagValue, D0), \
    __AROS_LCA(unsigned long   , defaultVal, D1), \
    __AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 6, Utility)

__AROS_LP0(ULONG, GetUniqueID,
    struct UtilityBase *, UtilityBase, 45, Utility)
#define GetUniqueID() \
    __AROS_LC0(ULONG, GetUniqueID, \
    struct UtilityBase *, UtilityBase, 45, Utility)

__AROS_LP1(struct TagItem *, NextTagItem,
    __AROS_LPA(struct TagItem **, tagListPtr, A0),
    struct Library *, UtilityBase, 8, Utility)
#define NextTagItem(tagListPtr) \
    __AROS_LC1(struct TagItem *, NextTagItem, \
    __AROS_LCA(struct TagItem **, tagListPtr, A0), \
    struct Library *, UtilityBase, 8, Utility)

__AROS_LP3(ULONG, PackBoolTags,
    __AROS_LPA(unsigned long   , initialFlags, D0),
    __AROS_LPA(struct TagItem *, tagList, A0),
    __AROS_LPA(struct TagItem *, boolMap, A1),
    struct UtilityBase *, UtilityBase, 7, Utility)
#define PackBoolTags(initialFlags, tagList, boolMap) \
    __AROS_LC3(ULONG, PackBoolTags, \
    __AROS_LCA(unsigned long   , initialFlags, D0), \
    __AROS_LCA(struct TagItem *, tagList, A0), \
    __AROS_LCA(struct TagItem *, boolMap, A1), \
    struct UtilityBase *, UtilityBase, 7, Utility)

__AROS_LP2(void, RefreshTagItemClones,
    __AROS_LPA(struct TagItem *, clone, A0),
    __AROS_LPA(struct TagItem *, original, A1),
    struct UtilityBase *, UtilityBase, 14, Utility)
#define RefreshTagItemClones(clone, original) \
    __AROS_LC2(void, RefreshTagItemClones, \
    __AROS_LCA(struct TagItem *, clone, A0), \
    __AROS_LCA(struct TagItem *, original, A1), \
    struct UtilityBase *, UtilityBase, 14, Utility)

__AROS_LP2(LONG, SMult32,
    __AROS_LPA(long, arg1, D0),
    __AROS_LPA(long, arg2, D1),
    struct UtilityBase *, UtilityBase, 23, Utility)
#define SMult32(arg1, arg2) \
    __AROS_LC2(LONG, SMult32, \
    __AROS_LCA(long, arg1, D0), \
    __AROS_LCA(long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 23, Utility)

__AROS_LP2(LONG, SMult64,
    __AROS_LPA(long, arg1, D0),
    __AROS_LPA(long, arg2, D1),
    struct UtilityBase *, UtilityBase, 33, Utility)
#define SMult64(arg1, arg2) \
    __AROS_LC2(LONG, SMult64, \
    __AROS_LCA(long, arg1, D0), \
    __AROS_LCA(long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 33, Utility)

__AROS_LP2(LONG, Stricmp,
    __AROS_LPA(STRPTR, string1, A0),
    __AROS_LPA(STRPTR, string2, A1),
    struct UtilityBase *, UtilityBase, 27, Utility)
#define Stricmp(string1, string2) \
    __AROS_LC2(LONG, Stricmp, \
    __AROS_LCA(STRPTR, string1, A0), \
    __AROS_LCA(STRPTR, string2, A1), \
    struct UtilityBase *, UtilityBase, 27, Utility)

__AROS_LP3(LONG, Strnicmp,
    __AROS_LPA(STRPTR, string1, A0),
    __AROS_LPA(STRPTR, string2, A1),
    __AROS_LPA(LONG,   length,  D0),
    struct UtilityBase *, UtilityBase, 28, Utility)
#define Strnicmp(string1, string2, length) \
    __AROS_LC3(LONG, Strnicmp, \
    __AROS_LCA(STRPTR, string1, A0), \
    __AROS_LCA(STRPTR, string2, A1), \
    __AROS_LCA(LONG,   length,  D0), \
    struct UtilityBase *, UtilityBase, 28, Utility)

__AROS_LP2(BOOL, TagInArray,
    __AROS_LPA(Tag  , tagValue, D0),
    __AROS_LPA(Tag *, tagArray, A0),
    struct UtilityBase *, UtilityBase, 15, Utility)
#define TagInArray(tagValue, tagArray) \
    __AROS_LC2(BOOL, TagInArray, \
    __AROS_LCA(Tag  , tagValue, D0), \
    __AROS_LCA(Tag *, tagArray, A0), \
    struct UtilityBase *, UtilityBase, 15, Utility)

__AROS_LP1I(UBYTE, ToLower,
    __AROS_LPA(ULONG, character, D0),
    struct UtilityBase *, UtilityBase, 30, Utility)
#define ToLower(character) \
    __AROS_LC1I(UBYTE, ToLower, \
    __AROS_LCA(ULONG, character, D0), \
    struct UtilityBase *, UtilityBase, 30, Utility)

__AROS_LP1I(UBYTE, ToUpper,
    __AROS_LPA(unsigned long, character, D0),
    struct UtilityBase *, UtilityBase, 29, Utility)
#define ToUpper(character) \
    __AROS_LC1I(UBYTE, ToUpper, \
    __AROS_LCA(unsigned long, character, D0), \
    struct UtilityBase *, UtilityBase, 29, Utility)

__AROS_LP2(ULONG, UMult32,
    __AROS_LPA(unsigned long, arg1, D0),
    __AROS_LPA(unsigned long, arg2, D1),
    struct UtilityBase *, UtilityBase, 24, Utility)
#define UMult32(arg1, arg2) \
    __AROS_LC2(ULONG, UMult32, \
    __AROS_LCA(unsigned long, arg1, D0), \
    __AROS_LCA(unsigned long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 24, Utility)

__AROS_LP2(ULONG, UMult64,
    __AROS_LPA(unsigned long, arg1, D0),
    __AROS_LPA(unsigned long, arg2, D1),
    struct UtilityBase *, UtilityBase, 34, Utility)
#define UMult64(arg1, arg2) \
    __AROS_LC2(ULONG, UMult64, \
    __AROS_LCA(unsigned long, arg1, D0), \
    __AROS_LCA(unsigned long, arg2, D1), \
    struct UtilityBase *, UtilityBase, 34, Utility)


#endif /* CLIB_UTILITY_PROTOS_H */
