#ifndef CLIB_UTILITY_PROTOS_H
#define CLIB_UTILITY_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP2(BOOL, AddNamedObject,
    AROS_LPA(struct NamedObject *, nameSpace, A0),
    AROS_LPA(struct NamedObject *, object, A1),
    struct UtilityBase *, UtilityBase, 37, Utility)
#define AddNamedObject(nameSpace, object) \
    AROS_LC2(BOOL, AddNamedObject, \
    AROS_LCA(struct NamedObject *, nameSpace, A0), \
    AROS_LCA(struct NamedObject *, object, A1), \
    struct UtilityBase *, UtilityBase, 37, Utility)

AROS_LP1(struct TagItem *, AllocateTagItems,
    AROS_LPA(ULONG, numTags, D0),
    struct UtilityBase *, UtilityBase, 11, Utility)
#define AllocateTagItems(numTags) \
    AROS_LC1(struct TagItem *, AllocateTagItems, \
    AROS_LCA(ULONG, numTags, D0), \
    struct UtilityBase *, UtilityBase, 11, Utility)

AROS_LP2(struct NamedObject *, AllocNamedObjectA,
    AROS_LPA(STRPTR          , name, A0),
    AROS_LPA(struct TagItem *, tagList, A1),
    struct UtilityBase *, UtilityBase, 38, Utility)
#define AllocNamedObjectA(name, tagList) \
    AROS_LC2(struct NamedObject *, AllocNamedObjectA, \
    AROS_LCA(STRPTR          , name, A0), \
    AROS_LCA(struct TagItem *, tagList, A1), \
    struct UtilityBase *, UtilityBase, 38, Utility)

AROS_LP2(void, Amiga2Date,
    AROS_LPA(ULONG             , seconds, D0),
    AROS_LPA(struct ClockData *, result, A0),
    struct Library *, UtilityBase, 20, Utility)
#define Amiga2Date(seconds, result) \
    AROS_LC2(void, Amiga2Date, \
    AROS_LCA(ULONG             , seconds, D0), \
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

AROS_LP1(LONG, AttemptRemNamedObject,
    AROS_LPA(struct NamedObject *, object, A0),
    struct Library *, UtilityBase, 39, Utility)
#define AttemptRemNamedObject(object) \
    AROS_LC1(LONG, AttemptRemNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct Library *, UtilityBase, 39, Utility)

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

AROS_LP1(ULONG, CheckDate,
    AROS_LPA(struct ClockData *, date, A0),
    struct Library *, UtilityBase, 22, Utility)
#define CheckDate(date) \
    AROS_LC1(ULONG, CheckDate, \
    AROS_LCA(struct ClockData *, date, A0), \
    struct Library *, UtilityBase, 22, Utility)

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

AROS_LP3(void, FilterTagChanges,
    AROS_LPA(struct TagItem *, changeList, A0),
    AROS_LPA(struct TagItem *, originalList, A1),
    AROS_LPA(BOOL            , apply, D0),
    struct Library *, UtilityBase, 9, Utility)
#define FilterTagChanges(changeList, originalList, apply) \
    AROS_LC3(void, FilterTagChanges, \
    AROS_LCA(struct TagItem *, changeList, A0), \
    AROS_LCA(struct TagItem *, originalList, A1), \
    AROS_LCA(BOOL            , apply, D0), \
    struct Library *, UtilityBase, 9, Utility)

AROS_LP3(ULONG, FilterTagItems,
    AROS_LPA(struct TagItem *, tagList, A0),
    AROS_LPA(Tag            *, filterArray, A1),
    AROS_LPA(ULONG           , logic, D0),
    struct Library *, UtilityBase, 16, Utility)
#define FilterTagItems(tagList, filterArray, logic) \
    AROS_LC3(ULONG, FilterTagItems, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    AROS_LCA(Tag            *, filterArray, A1), \
    AROS_LCA(ULONG           , logic, D0), \
    struct Library *, UtilityBase, 16, Utility)

AROS_LP3(struct NamedObject *, FindNamedObject,
    AROS_LPA(struct NamedObject *, nameSpace, A0),
    AROS_LPA(STRPTR              , name, A1),
    AROS_LPA(struct NamedObject *, lastObject, A2),
    struct UtilityBase *, UtilityBase, 40, Utility)
#define FindNamedObject(nameSpace, name, lastObject) \
    AROS_LC3(struct NamedObject *, FindNamedObject, \
    AROS_LCA(struct NamedObject *, nameSpace, A0), \
    AROS_LCA(STRPTR              , name, A1), \
    AROS_LCA(struct NamedObject *, lastObject, A2), \
    struct UtilityBase *, UtilityBase, 40, Utility)

AROS_LP2(struct TagItem *, FindTagItem,
    AROS_LPA(Tag,              tagValue, D0),
    AROS_LPA(struct TagItem *, tagList,  A0),
    struct UtilityBase *, UtilityBase, 5, Utility)
#define FindTagItem(tagValue, tagList) \
    AROS_LC2(struct TagItem *, FindTagItem, \
    AROS_LCA(Tag,              tagValue, D0), \
    AROS_LCA(struct TagItem *, tagList,  A0), \
    struct UtilityBase *, UtilityBase, 5, Utility)

AROS_LP1(void, FreeNamedObject,
    AROS_LPA(struct NamedObject *, object, A0),
    struct UtilityBase *, UtilityBase, 41, Utility)
#define FreeNamedObject(object) \
    AROS_LC1(void, FreeNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct UtilityBase *, UtilityBase, 41, Utility)

AROS_LP1(void, FreeTagItems,
    AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 13, Utility)
#define FreeTagItems(tagList) \
    AROS_LC1(void, FreeTagItems, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 13, Utility)

AROS_LP3(ULONG, GetTagData,
    AROS_LPA(Tag             , tagValue, D0),
    AROS_LPA(ULONG           , defaultVal, D1),
    AROS_LPA(struct TagItem *, tagList, A0),
    struct UtilityBase *, UtilityBase, 6, Utility)
#define GetTagData(tagValue, defaultVal, tagList) \
    AROS_LC3(ULONG, GetTagData, \
    AROS_LCA(Tag             , tagValue, D0), \
    AROS_LCA(ULONG           , defaultVal, D1), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 6, Utility)

AROS_LP0(ULONG, GetUniqueID,
    struct UtilityBase *, UtilityBase, 45, Utility)
#define GetUniqueID() \
    AROS_LC0(ULONG, GetUniqueID, \
    struct UtilityBase *, UtilityBase, 45, Utility)

AROS_LP3(void, MapTags,
    AROS_LPA(struct TagItem *, tagList, A0),
    AROS_LPA(struct TagItem *, mapList, A1),
    AROS_LPA(ULONG           , mapType, D0),
    struct Library *, UtilityBase, 10, Utility)
#define MapTags(tagList, mapList, mapType) \
    AROS_LC3(void, MapTags, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    AROS_LCA(struct TagItem *, mapList, A1), \
    AROS_LCA(ULONG           , mapType, D0), \
    struct Library *, UtilityBase, 10, Utility)

AROS_LP1(STRPTR, NamedObjectName,
    AROS_LPA(struct NamedObject *, object, A0),
    struct UtilityBase *, UtilityBase, 42, Utility)
#define NamedObjectName(object) \
    AROS_LC1(STRPTR, NamedObjectName, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct UtilityBase *, UtilityBase, 42, Utility)

AROS_LP1(struct TagItem *, NextTagItem,
    AROS_LPA(struct TagItem **, tagListPtr, A0),
    struct Library *, UtilityBase, 8, Utility)
#define NextTagItem(tagListPtr) \
    AROS_LC1(struct TagItem *, NextTagItem, \
    AROS_LCA(struct TagItem **, tagListPtr, A0), \
    struct Library *, UtilityBase, 8, Utility)

AROS_LP3(ULONG, PackBoolTags,
    AROS_LPA(ULONG           , initialFlags, D0),
    AROS_LPA(struct TagItem *, tagList, A0),
    AROS_LPA(struct TagItem *, boolMap, A1),
    struct UtilityBase *, UtilityBase, 7, Utility)
#define PackBoolTags(initialFlags, tagList, boolMap) \
    AROS_LC3(ULONG, PackBoolTags, \
    AROS_LCA(ULONG           , initialFlags, D0), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    AROS_LCA(struct TagItem *, boolMap, A1), \
    struct UtilityBase *, UtilityBase, 7, Utility)

AROS_LP3(ULONG, PackStructureTags,
    AROS_LPA(APTR            , pack, A0),
    AROS_LPA(ULONG          *, packTable, A1),
    AROS_LPA(struct TagItem *, tagList, A2),
    struct Library *, UtilityBase, 35, Utility)
#define PackStructureTags(pack, packTable, tagList) \
    AROS_LC3(ULONG, PackStructureTags, \
    AROS_LCA(APTR            , pack, A0), \
    AROS_LCA(ULONG          *, packTable, A1), \
    AROS_LCA(struct TagItem *, tagList, A2), \
    struct Library *, UtilityBase, 35, Utility)

AROS_LP2(void, RefreshTagItemClones,
    AROS_LPA(struct TagItem *, clone, A0),
    AROS_LPA(struct TagItem *, original, A1),
    struct UtilityBase *, UtilityBase, 14, Utility)
#define RefreshTagItemClones(clone, original) \
    AROS_LC2(void, RefreshTagItemClones, \
    AROS_LCA(struct TagItem *, clone, A0), \
    AROS_LCA(struct TagItem *, original, A1), \
    struct UtilityBase *, UtilityBase, 14, Utility)

AROS_LP1(void, ReleaseNamedObject,
    AROS_LPA(struct NamedObject *, object, A0),
    struct UtilityBase *, UtilityBase, 43, Utility)
#define ReleaseNamedObject(object) \
    AROS_LC1(void, ReleaseNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct UtilityBase *, UtilityBase, 43, Utility)

AROS_LP2(void, RemNamedObject,
    AROS_LPA(struct NamedObject *, object, A0),
    AROS_LPA(struct Message     *, message, A1),
    struct UtilityBase *, UtilityBase, 44, Utility)
#define RemNamedObject(object, message) \
    AROS_LC2(void, RemNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    AROS_LCA(struct Message     *, message, A1), \
    struct UtilityBase *, UtilityBase, 44, Utility)

AROS_LP2(LONG, SMult32,
    AROS_LPA(LONG, arg1, D0),
    AROS_LPA(LONG, arg2, D1),
    struct UtilityBase *, UtilityBase, 23, Utility)
#define SMult32(arg1, arg2) \
    AROS_LC2(LONG, SMult32, \
    AROS_LCA(LONG, arg1, D0), \
    AROS_LCA(LONG, arg2, D1), \
    struct UtilityBase *, UtilityBase, 23, Utility)

AROS_LP2(QUAD, SMult64,
    AROS_LPA(LONG, arg1, D0),
    AROS_LPA(LONG, arg2, D1),
    struct UtilityBase *, UtilityBase, 33, Utility)
#define SMult64(arg1, arg2) \
    AROS_LC2(QUAD, SMult64, \
    AROS_LCA(LONG, arg1, D0), \
    AROS_LCA(LONG, arg2, D1), \
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
    AROS_LPA(ULONG, character, D0),
    struct UtilityBase *, UtilityBase, 29, Utility)
#define ToUpper(character) \
    AROS_LC1I(UBYTE, ToUpper, \
    AROS_LCA(ULONG, character, D0), \
    struct UtilityBase *, UtilityBase, 29, Utility)

AROS_LP2(ULONG, UMult32,
    AROS_LPA(ULONG        , arg1, D0),
    AROS_LPA(ULONG        , arg2, D1),
    struct UtilityBase *, UtilityBase, 24, Utility)
#define UMult32(arg1, arg2) \
    AROS_LC2(ULONG, UMult32, \
    AROS_LCA(ULONG        , arg1, D0), \
    AROS_LCA(ULONG        , arg2, D1), \
    struct UtilityBase *, UtilityBase, 24, Utility)

AROS_LP2(UQUAD, UMult64,
    AROS_LPA(ULONG        , arg1, D0),
    AROS_LPA(ULONG        , arg2, D1),
    struct UtilityBase *, UtilityBase, 34, Utility)
#define UMult64(arg1, arg2) \
    AROS_LC2(UQUAD, UMult64, \
    AROS_LCA(ULONG        , arg1, D0), \
    AROS_LCA(ULONG        , arg2, D1), \
    struct UtilityBase *, UtilityBase, 34, Utility)

AROS_LP3(ULONG, UnpackStructureTags,
    AROS_LPA(APTR            , pack, A0),
    AROS_LPA(ULONG          *, packTable, A1),
    AROS_LPA(struct TagItem *, tagList, A2),
    struct Library *, UtilityBase, 36, Utility)
#define UnpackStructureTags(pack, packTable, tagList) \
    AROS_LC3(ULONG, UnpackStructureTags, \
    AROS_LCA(APTR            , pack, A0), \
    AROS_LCA(ULONG          *, packTable, A1), \
    AROS_LCA(struct TagItem *, tagList, A2), \
    struct Library *, UtilityBase, 36, Utility)


#endif /* CLIB_UTILITY_PROTOS_H */
