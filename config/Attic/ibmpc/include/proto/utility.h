#ifndef PROTO_UTILITY_H
#define PROTO_UTILITY_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef UTILITY_BASE_NAME
#define UTILITY_BASE_NAME UtilityBase
#endif

#define AddNamedObject(nameSpace, object) \
	LP2(0x94, BOOL, AddNamedObject, struct NamedObject *, nameSpace, struct NamedObject *, object, \
	, UTILITY_BASE_NAME)

#define AllocNamedObjectA(name, tagList) \
	LP2(0x98, struct NamedObject *, AllocNamedObjectA, STRPTR, name, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AllocNamedObject(a0, tags...) \
	({ULONG _tags[] = { tags }; AllocNamedObjectA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AllocateTagItems(numTags) \
	LP1(0x2c, struct TagItem *, AllocateTagItems, unsigned long, numTags, \
	, UTILITY_BASE_NAME)

#define Amiga2Date(seconds, result) \
	LP2NR(0x50, Amiga2Date, unsigned long, seconds, struct ClockData *, result, \
	, UTILITY_BASE_NAME)

#define ApplyTagChanges(list, changeList) \
	LP2NR(0x7c, ApplyTagChanges, struct TagItem *, list, struct TagItem *, changeList, \
	, UTILITY_BASE_NAME)

#define AttemptRemNamedObject(object) \
	LP1(0x9c, LONG, AttemptRemNamedObject, struct NamedObject *, object, \
	, UTILITY_BASE_NAME)

#define CallHookPkt(hook, object, paramPacket) \
	LP3(0x44, ULONG, CallHookPkt, struct Hook *, hook, APTR, object, APTR, paramPacket, \
	, UTILITY_BASE_NAME)

#define CheckDate(date) \
	LP1(0x58, ULONG, CheckDate, struct ClockData *, date, \
	, UTILITY_BASE_NAME)

#define CloneTagItems(tagList) \
	LP1(0x30, struct TagItem *, CloneTagItems, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#define Date2Amiga(date) \
	LP1(0x54, ULONG, Date2Amiga, struct ClockData *, date, \
	, UTILITY_BASE_NAME)

#define FilterTagChanges(changeList, originalList, apply) \
	LP3NR(0x24, FilterTagChanges, struct TagItem *, changeList, struct TagItem *, originalList, unsigned long, apply, \
	, UTILITY_BASE_NAME)

#define FilterTagItems(tagList, filterArray, logic) \
	LP3(0x40, ULONG, FilterTagItems, struct TagItem *, tagList, Tag *, filterArray, unsigned long, logic, \
	, UTILITY_BASE_NAME)

#define FindNamedObject(nameSpace, name, lastObject) \
	LP3(0xa0, struct NamedObject *, FindNamedObject, struct NamedObject *, nameSpace, STRPTR, name, struct NamedObject *, lastObject, \
	, UTILITY_BASE_NAME)

#define FindTagItem(tagVal, tagList) \
	LP2(0x14, struct TagItem *, FindTagItem, Tag, tagVal, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#define FreeNamedObject(object) \
	LP1NR(0xa4, FreeNamedObject, struct NamedObject *, object, \
	, UTILITY_BASE_NAME)

#define FreeTagItems(tagList) \
	LP1NR(0x34, FreeTagItems, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#define GetTagData(tagValue, defaultVal, tagList) \
	LP3(0x18, ULONG, GetTagData, Tag, tagValue, unsigned long, defaultVal, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#define GetUniqueID() \
	LP0(0xb4, ULONG, GetUniqueID, \
	, UTILITY_BASE_NAME)

#define MapTags(tagList, mapList, mapType) \
	LP3NR(0x28, MapTags, struct TagItem *, tagList, struct TagItem *, mapList, unsigned long, mapType, \
	, UTILITY_BASE_NAME)

#define NamedObjectName(object) \
	LP1(0xa8, STRPTR, NamedObjectName, struct NamedObject *, object, \
	, UTILITY_BASE_NAME)

#define NextTagItem(tagListPtr) \
	LP1(0x20, struct TagItem *, NextTagItem, struct TagItem **, tagListPtr, \
	, UTILITY_BASE_NAME)

#define PackBoolTags(initialFlags, tagList, boolMap) \
	LP3(0x1c, ULONG, PackBoolTags, unsigned long, initialFlags, struct TagItem *, tagList, struct TagItem *, boolMap, \
	, UTILITY_BASE_NAME)

#define PackStructureTags(pack, packTable, tagList) \
	LP3(0x8c, ULONG, PackStructureTags, APTR, pack, ULONG *, packTable, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#define RefreshTagItemClones(clone, original) \
	LP2NR(0x38, RefreshTagItemClones, struct TagItem *, clone, struct TagItem *, original, \
	, UTILITY_BASE_NAME)

#define ReleaseNamedObject(object) \
	LP1NR(0xac, ReleaseNamedObject, struct NamedObject *, object, \
	, UTILITY_BASE_NAME)

#define RemNamedObject(object, message) \
	LP2NR(0xb0, RemNamedObject, struct NamedObject *, object, struct Message *, message, \
	, UTILITY_BASE_NAME)

#define SDivMod32(dividend, divisor) \
	LP2(0x64, LONG, SDivMod32, long, dividend, long, divisor, \
	, UTILITY_BASE_NAME)

#define SMult32(arg1, arg2) \
	LP2(0x5c, LONG, SMult32, long, arg1, long, arg2, \
	, UTILITY_BASE_NAME)

#define SMult64(arg1, arg2) \
	LP2(0x84, LONG, SMult64, long, arg1, long, arg2, \
	, UTILITY_BASE_NAME)

#define Stricmp(string1, string2) \
	LP2(0x6c, LONG, Stricmp, STRPTR, string1, STRPTR, string2, \
	, UTILITY_BASE_NAME)

#define Strnicmp(string1, string2, length) \
	LP3(0x70, LONG, Strnicmp, STRPTR, string1, STRPTR, string2, long, length, \
	, UTILITY_BASE_NAME)

#define TagInArray(tagValue, tagArray) \
	LP2(0x3c, BOOL, TagInArray, Tag, tagValue, Tag *, tagArray, \
	, UTILITY_BASE_NAME)

#define ToLower(character) \
	LP1(0x78, UBYTE, ToLower, unsigned long, character, \
	, UTILITY_BASE_NAME)

#define ToUpper(character) \
	LP1(0x74, UBYTE, ToUpper, unsigned long, character, \
	, UTILITY_BASE_NAME)

#define UDivMod32(dividend, divisor) \
	LP2(0x68, ULONG, UDivMod32, unsigned long, dividend, unsigned long, divisor, \
	, UTILITY_BASE_NAME)

#define UMult32(arg1, arg2) \
	LP2(0x60, ULONG, UMult32, unsigned long, arg1, unsigned long, arg2, \
	, UTILITY_BASE_NAME)

#define UMult64(arg1, arg2) \
	LP2(0x88, ULONG, UMult64, unsigned long, arg1, unsigned long, arg2, \
	, UTILITY_BASE_NAME)

#define UnpackStructureTags(pack, packTable, tagList) \
	LP3(0x90, ULONG, UnpackStructureTags, APTR, pack, ULONG *, packTable, struct TagItem *, tagList, \
	, UTILITY_BASE_NAME)

#endif /* PROTO_UTILITY_H */
