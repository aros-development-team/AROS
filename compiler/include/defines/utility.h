#ifndef DEFINES_UTILITY_H
#define DEFINES_UTILITY_H

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AddNamedObject(nameSpace, object) \
    AROS_LC2(BOOL, AddNamedObject, \
    AROS_LCA(struct NamedObject *, nameSpace, A0), \
    AROS_LCA(struct NamedObject *, object, A1), \
    struct UtilityBase *, UtilityBase, 37, Utility)

#define AllocateTagItems(numTags) \
    AROS_LC1(struct TagItem *, AllocateTagItems, \
    AROS_LCA(ULONG, numTags, D0), \
    struct UtilityBase *, UtilityBase, 11, Utility)

#define AllocNamedObjectA(name, tagList) \
    AROS_LC2(struct NamedObject *, AllocNamedObjectA, \
    AROS_LCA(STRPTR          , name, A0), \
    AROS_LCA(struct TagItem *, tagList, A1), \
    struct UtilityBase *, UtilityBase, 38, Utility)

#define ApplyTagChanges(list, changelist) \
    AROS_LC2(void, ApplyTagChanges, \
    AROS_LCA(struct TagItem *, list,       A0), \
    AROS_LCA(struct TagItem *, changelist, A1), \
    struct UtilityBase *, UtilityBase, 31, Utility)

#define CloneTagItems(tagList) \
    AROS_LC1(struct TagItem *, CloneTagItems, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 12, Utility)

#define Date2Amiga(date) \
    AROS_LC1(ULONG, Date2Amiga, \
    AROS_LCA(struct ClockData *, date, A0), \
    struct UtilityBase *, UtilityBase, 21, Utility)

#define FindNamedObject(nameSpace, name, lastObject) \
    AROS_LC3(struct NamedObject *, FindNamedObject, \
    AROS_LCA(struct NamedObject *, nameSpace, A0), \
    AROS_LCA(STRPTR              , name, A1), \
    AROS_LCA(struct NamedObject *, lastObject, A2), \
    struct UtilityBase *, UtilityBase, 40, Utility)

#define FindTagItem(tagValue, tagList) \
    AROS_LC2(struct TagItem *, FindTagItem, \
    AROS_LCA(Tag,              tagValue, D0), \
    AROS_LCA(struct TagItem *, tagList,  A0), \
    struct UtilityBase *, UtilityBase, 5, Utility)

#define FreeNamedObject(object) \
    AROS_LC1(void, FreeNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct UtilityBase *, UtilityBase, 41, Utility)

#define FreeTagItems(tagList) \
    AROS_LC1(void, FreeTagItems, \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 13, Utility)

#define GetTagData(tagValue, defaultVal, tagList) \
    AROS_LC3(ULONG, GetTagData, \
    AROS_LCA(Tag             , tagValue, D0), \
    AROS_LCA(ULONG           , defaultVal, D1), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    struct UtilityBase *, UtilityBase, 6, Utility)

#define GetUniqueID() \
    AROS_LC0(ULONG, GetUniqueID, \
    struct UtilityBase *, UtilityBase, 45, Utility)

#define NamedObjectName(object) \
    AROS_LC1(STRPTR, NamedObjectName, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct UtilityBase *, UtilityBase, 42, Utility)

#define PackBoolTags(initialFlags, tagList, boolMap) \
    AROS_LC3(ULONG, PackBoolTags, \
    AROS_LCA(ULONG           , initialFlags, D0), \
    AROS_LCA(struct TagItem *, tagList, A0), \
    AROS_LCA(struct TagItem *, boolMap, A1), \
    struct UtilityBase *, UtilityBase, 7, Utility)

#define RefreshTagItemClones(clone, original) \
    AROS_LC2(void, RefreshTagItemClones, \
    AROS_LCA(struct TagItem *, clone, A0), \
    AROS_LCA(struct TagItem *, original, A1), \
    struct UtilityBase *, UtilityBase, 14, Utility)

#define ReleaseNamedObject(object) \
    AROS_LC1(void, ReleaseNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    struct UtilityBase *, UtilityBase, 43, Utility)

#define RemNamedObject(object, message) \
    AROS_LC2(void, RemNamedObject, \
    AROS_LCA(struct NamedObject *, object, A0), \
    AROS_LCA(struct Message     *, message, A1), \
    struct UtilityBase *, UtilityBase, 44, Utility)

#define SMult32(arg1, arg2) \
    AROS_LC2(LONG, SMult32, \
    AROS_LCA(LONG, arg1, D0), \
    AROS_LCA(LONG, arg2, D1), \
    struct UtilityBase *, UtilityBase, 23, Utility)

#define SMult64(arg1, arg2) \
    AROS_LC2(QUAD, SMult64, \
    AROS_LCA(LONG, arg1, D0), \
    AROS_LCA(LONG, arg2, D1), \
    struct UtilityBase *, UtilityBase, 33, Utility)

#define Stricmp(string1, string2) \
    AROS_LC2(LONG, Stricmp, \
    AROS_LCA(STRPTR, string1, A0), \
    AROS_LCA(STRPTR, string2, A1), \
    struct UtilityBase *, UtilityBase, 27, Utility)

#define Strnicmp(string1, string2, length) \
    AROS_LC3(LONG, Strnicmp, \
    AROS_LCA(STRPTR, string1, A0), \
    AROS_LCA(STRPTR, string2, A1), \
    AROS_LCA(LONG,   length,  D0), \
    struct UtilityBase *, UtilityBase, 28, Utility)

#define TagInArray(tagValue, tagArray) \
    AROS_LC2(BOOL, TagInArray, \
    AROS_LCA(Tag  , tagValue, D0), \
    AROS_LCA(Tag *, tagArray, A0), \
    struct UtilityBase *, UtilityBase, 15, Utility)

#define ToLower(character) \
    AROS_LC1I(UBYTE, ToLower, \
    AROS_LCA(ULONG, character, D0), \
    struct UtilityBase *, UtilityBase, 30, Utility)

#define ToUpper(character) \
    AROS_LC1I(UBYTE, ToUpper, \
    AROS_LCA(ULONG, character, D0), \
    struct UtilityBase *, UtilityBase, 29, Utility)

#define UMult32(arg1, arg2) \
    AROS_LC2(ULONG, UMult32, \
    AROS_LCA(ULONG        , arg1, D0), \
    AROS_LCA(ULONG        , arg2, D1), \
    struct UtilityBase *, UtilityBase, 24, Utility)

#define UMult64(arg1, arg2) \
    AROS_LC2(UQUAD, UMult64, \
    AROS_LCA(ULONG        , arg1, D0), \
    AROS_LCA(ULONG        , arg2, D1), \
    struct UtilityBase *, UtilityBase, 34, Utility)


#endif /* DEFINES_UTILITY_H */
