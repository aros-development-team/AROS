#ifndef _INLINE_ICON_H
#define _INLINE_ICON_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef ICON_BASE_NAME
#define ICON_BASE_NAME IconBase
#endif

#define AddFreeList(freelist, mem, size) \
	LP3(0x48, BOOL, AddFreeList, struct FreeList *, freelist, a0, APTR, mem, a1, unsigned long, size, a2, \
	, ICON_BASE_NAME)

#define BumpRevision(newname, oldname) \
	LP2(0x6c, UBYTE *, BumpRevision, UBYTE *, newname, a0, UBYTE *, oldname, a1, \
	, ICON_BASE_NAME)

#define DeleteDiskObject(name) \
	LP1(0x8a, BOOL, DeleteDiskObject, UBYTE *, name, a0, \
	, ICON_BASE_NAME)

#define FindToolType(toolTypeArray, typeName) \
	LP2(0x60, UBYTE *, FindToolType, UBYTE **, toolTypeArray, a0, UBYTE *, typeName, a1, \
	, ICON_BASE_NAME)

#define FreeDiskObject(diskobj) \
	LP1NR(0x5a, FreeDiskObject, struct DiskObject *, diskobj, a0, \
	, ICON_BASE_NAME)

#define FreeFreeList(freelist) \
	LP1NR(0x36, FreeFreeList, struct FreeList *, freelist, a0, \
	, ICON_BASE_NAME)

#define GetDefDiskObject(type) \
	LP1(0x78, struct DiskObject *, GetDefDiskObject, long, type, d0, \
	, ICON_BASE_NAME)

#define GetDiskObject(name) \
	LP1(0x4e, struct DiskObject *, GetDiskObject, UBYTE *, name, a0, \
	, ICON_BASE_NAME)

#define GetDiskObjectNew(name) \
	LP1(0x84, struct DiskObject *, GetDiskObjectNew, UBYTE *, name, a0, \
	, ICON_BASE_NAME)

#define MatchToolValue(typeString, value) \
	LP2(0x66, BOOL, MatchToolValue, UBYTE *, typeString, a0, UBYTE *, value, a1, \
	, ICON_BASE_NAME)

#define PutDefDiskObject(diskObject) \
	LP1(0x7e, BOOL, PutDefDiskObject, struct DiskObject *, diskObject, a0, \
	, ICON_BASE_NAME)

#define PutDiskObject(name, diskobj) \
	LP2(0x54, BOOL, PutDiskObject, UBYTE *, name, a0, struct DiskObject *, diskobj, a1, \
	, ICON_BASE_NAME)

#endif /* _INLINE_ICON_H */
