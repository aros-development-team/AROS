#ifndef _INLINE_UTILITY_H
#define _INLINE_UTILITY_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef UTILITY_BASE_NAME
#define UTILITY_BASE_NAME UtilityBase
#endif

#define AddNamedObject(nameSpace, object) \
	LP2(0xde, BOOL, AddNamedObject, struct NamedObject *, nameSpace, a0, struct NamedObject *, object, a1, \
	, UTILITY_BASE_NAME)

#define AllocNamedObjectA(name, tagList) \
	LP2(0xe4, struct NamedObject *, AllocNamedObjectA, STRPTR, name, a0, struct TagItem *, tagList, a1, \
	, UTILITY_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AllocNamedObject(a0, tags...) \
	({ULONG _tags[] = { tags }; AllocNamedObjectA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AllocateTagItems(numTags) \
	LP1(0x42, struct TagItem *, AllocateTagItems, unsigned long, numTags, d0, \
	, UTILITY_BASE_NAME)

#define Amiga2Date(seconds, result) \
	LP2NR(0x78, Amiga2Date, unsigned long, seconds, d0, struct ClockData *, result, a0, \
	, UTILITY_BASE_NAME)

#define ApplyTagChanges(list, changeList) \
	LP2NR(0xba, ApplyTagChanges, struct TagItem *, list, a0, struct TagItem *, changeList, a1, \
	, UTILITY_BASE_NAME)

#define AttemptRemNamedObject(object) \
	LP1(0xea, LONG, AttemptRemNamedObject, struct NamedObject *, object, a0, \
	, UTILITY_BASE_NAME)

#define CallHookPkt(hook, object, paramPacket) \
	LP3(0x66, ULONG, CallHookPkt, struct Hook *, hook, a0, APTR, object, a2, APTR, paramPacket, a1, \
	, UTILITY_BASE_NAME)

#define CheckDate(date) \
	LP1(0x84, ULONG, CheckDate, struct ClockData *, date, a0, \
	, UTILITY_BASE_NAME)

#define CloneTagItems(tagList) \
	LP1(0x48, struct TagItem *, CloneTagItems, struct TagItem *, tagList, a0, \
	, UTILITY_BASE_NAME)

#define Date2Amiga(date) \
	LP1(0x7e, ULONG, Date2Amiga, struct ClockData *, date, a0, \
	, UTILITY_BASE_NAME)

#define FilterTagChanges(changeList, originalList, apply) \
	LP3NR(0x36, FilterTagChanges, struct TagItem *, changeList, a0, struct TagItem *, originalList, a1, unsigned long, apply, d0, \
	, UTILITY_BASE_NAME)

#define FilterTagItems(tagList, filterArray, logic) \
	LP3(0x60, ULONG, FilterTagItems, struct TagItem *, tagList, a0, Tag *, filterArray, a1, unsigned long, logic, d0, \
	, UTILITY_BASE_NAME)

#define FindNamedObject(nameSpace, name, lastObject) \
	LP3(0xf0, struct NamedObject *, FindNamedObject, struct NamedObject *, nameSpace, a0, STRPTR, name, a1, struct NamedObject *, lastObject, a2, \
	, UTILITY_BASE_NAME)

#define FindTagItem(tagVal, tagList) \
	LP2(0x1e, struct TagItem *, FindTagItem, Tag, tagVal, d0, struct TagItem *, tagList, a0, \
	, UTILITY_BASE_NAME)

#define FreeNamedObject(object) \
	LP1NR(0xf6, FreeNamedObject, struct NamedObject *, object, a0, \
	, UTILITY_BASE_NAME)

#define FreeTagItems(tagList) \
	LP1NR(0x4e, FreeTagItems, struct TagItem *, tagList, a0, \
	, UTILITY_BASE_NAME)

#define GetTagData(tagValue, defaultVal, tagList) \
	LP3(0x24, ULONG, GetTagData, Tag, tagValue, d0, unsigned long, defaultVal, d1, struct TagItem *, tagList, a0, \
	, UTILITY_BASE_NAME)

#define GetUniqueID() \
	LP0(0x10e, ULONG, GetUniqueID, \
	, UTILITY_BASE_NAME)

#define MapTags(tagList, mapList, mapType) \
	LP3NR(0x3c, MapTags, struct TagItem *, tagList, a0, struct TagItem *, mapList, a1, unsigned long, mapType, d0, \
	, UTILITY_BASE_NAME)

#define NamedObjectName(object) \
	LP1(0xfc, STRPTR, NamedObjectName, struct NamedObject *, object, a0, \
	, UTILITY_BASE_NAME)

#define NextTagItem(tagListPtr) \
	LP1(0x30, struct TagItem *, NextTagItem, struct TagItem **, tagListPtr, a0, \
	, UTILITY_BASE_NAME)

#define PackBoolTags(initialFlags, tagList, boolMap) \
	LP3(0x2a, ULONG, PackBoolTags, unsigned long, initialFlags, d0, struct TagItem *, tagList, a0, struct TagItem *, boolMap, a1, \
	, UTILITY_BASE_NAME)

#define PackStructureTags(pack, packTable, tagList) \
	LP3(0xd2, ULONG, PackStructureTags, APTR, pack, a0, ULONG *, packTable, a1, struct TagItem *, tagList, a2, \
	, UTILITY_BASE_NAME)

#define RefreshTagItemClones(clone, original) \
	LP2NR(0x54, RefreshTagItemClones, struct TagItem *, clone, a0, struct TagItem *, original, a1, \
	, UTILITY_BASE_NAME)

#define ReleaseNamedObject(object) \
	LP1NR(0x102, ReleaseNamedObject, struct NamedObject *, object, a0, \
	, UTILITY_BASE_NAME)

#define RemNamedObject(object, message) \
	LP2NR(0x108, RemNamedObject, struct NamedObject *, object, a0, struct Message *, message, a1, \
	, UTILITY_BASE_NAME)

#define SDivMod32(dividend, divisor) \
	LP2(0x96, LONG, SDivMod32, long, dividend, d0, long, divisor, d1, \
	, UTILITY_BASE_NAME)

#define SMult32(arg1, arg2) \
	LP2(0x8a, LONG, SMult32, long, arg1, d0, long, arg2, d1, \
	, UTILITY_BASE_NAME)

#define SMult64(arg1, arg2) \
	LP2(0xc6, LONG, SMult64, long, arg1, d0, long, arg2, d1, \
	, UTILITY_BASE_NAME)

#define Stricmp(string1, string2) \
	LP2(0xa2, LONG, Stricmp, STRPTR, string1, a0, STRPTR, string2, a1, \
	, UTILITY_BASE_NAME)

#define Strnicmp(string1, string2, length) \
	LP3(0xa8, LONG, Strnicmp, STRPTR, string1, a0, STRPTR, string2, a1, long, length, d0, \
	, UTILITY_BASE_NAME)

#define TagInArray(tagValue, tagArray) \
	LP2(0x5a, BOOL, TagInArray, Tag, tagValue, d0, Tag *, tagArray, a0, \
	, UTILITY_BASE_NAME)

#define ToLower(character) \
	LP1(0xb4, UBYTE, ToLower, unsigned long, character, d0, \
	, UTILITY_BASE_NAME)

#define ToUpper(character) \
	LP1(0xae, UBYTE, ToUpper, unsigned long, character, d0, \
	, UTILITY_BASE_NAME)

#define UDivMod32(dividend, divisor) \
	LP2(0x9c, ULONG, UDivMod32, unsigned long, dividend, d0, unsigned long, divisor, d1, \
	, UTILITY_BASE_NAME)

#define UMult32(arg1, arg2) \
	LP2(0x90, ULONG, UMult32, unsigned long, arg1, d0, unsigned long, arg2, d1, \
	, UTILITY_BASE_NAME)

#define UMult64(arg1, arg2) \
	LP2(0xcc, ULONG, UMult64, unsigned long, arg1, d0, unsigned long, arg2, d1, \
	, UTILITY_BASE_NAME)

#define UnpackStructureTags(pack, packTable, tagList) \
	LP3(0xd8, ULONG, UnpackStructureTags, APTR, pack, a0, ULONG *, packTable, a1, struct TagItem *, tagList, a2, \
	, UTILITY_BASE_NAME)

#endif /* _INLINE_UTILITY_H */
