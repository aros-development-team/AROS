#ifndef CLIB_UTILITY_PROTOS_H
#define CLIB_UTILITY_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
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

__AROS_LP2(struct TagItem *, FindTagItem,
    __AROS_LPA(Tag,              tagValue, D0),
    __AROS_LPA(struct TagItem *, tagList,  A0),
    struct UtilityBase *, UtilityBase, 5, Utility)
#define FindTagItem(tagValue, tagList) \
    __AROS_LC2(struct TagItem *, FindTagItem, \
    __AROS_LCA(Tag,              tagValue, D0), \
    __AROS_LCA(struct TagItem *, tagList,  A0), \
    struct UtilityBase *, UtilityBase, 5, Utility)

__AROS_LP0(ULONG, GetUniqueID,
    struct Library *, UtilityBase, 45, Utility)
#define GetUniqueID() \
    __AROS_LC0(ULONG, GetUniqueID, \
    struct Library *, UtilityBase, 45, Utility)

__AROS_LP1(struct TagItem *, NextTagItem,
    __AROS_LPA(struct TagItem **, tagListPtr, A0),
    struct Library *, UtilityBase, 8, Utility)
#define NextTagItem(tagListPtr) \
    __AROS_LC1(struct TagItem *, NextTagItem, \
    __AROS_LCA(struct TagItem **, tagListPtr, A0), \
    struct Library *, UtilityBase, 8, Utility)

__AROS_LP2I(LONG, Stricmp,
    __AROS_LPA(STRPTR, string1, A0),
    __AROS_LPA(STRPTR, string2, A1),
    struct UtilityBase *, UtilityBase, 27, Utility)
#define Stricmp(string1, string2) \
    __AROS_LC2I(LONG, Stricmp, \
    __AROS_LCA(STRPTR, string1, A0), \
    __AROS_LCA(STRPTR, string2, A1), \
    struct UtilityBase *, UtilityBase, 27, Utility)

__AROS_LP3I(LONG, Strnicmp,
    __AROS_LPA(STRPTR, string1, A0),
    __AROS_LPA(STRPTR, string2, A1),
    __AROS_LPA(LONG,   length,  D0),
    struct UtilityBase *, UtilityBase, 28, Utility)
#define Strnicmp(string1, string2, length) \
    __AROS_LC3I(LONG, Strnicmp, \
    __AROS_LCA(STRPTR, string1, A0), \
    __AROS_LCA(STRPTR, string2, A1), \
    __AROS_LCA(LONG,   length,  D0), \
    struct UtilityBase *, UtilityBase, 28, Utility)

__AROS_LP1I(UBYTE, ToLower,
    __AROS_LPA(ULONG, character, D0),
    struct UtilityBase *, UtilityBase, 30, Utility)
#define ToLower(character) \
    __AROS_LC1I(UBYTE, ToLower, \
    __AROS_LCA(ULONG, character, D0), \
    struct UtilityBase *, UtilityBase, 30, Utility)


#endif /* CLIB_UTILITY_PROTOS_H */
