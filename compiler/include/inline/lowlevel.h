#ifndef _INLINE_LOWLEVEL_H
#define _INLINE_LOWLEVEL_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef LOWLEVEL_BASE_NAME
#define LOWLEVEL_BASE_NAME LowLevelBase
#endif

#define AddKBInt(intRoutine, intData) \
	LP2(0x3c, APTR, AddKBInt, APTR, intRoutine, a0, APTR, intData, a1, \
	, LOWLEVEL_BASE_NAME)

#define AddTimerInt(intRoutine, intData) \
	LP2(0x4e, APTR, AddTimerInt, APTR, intRoutine, a0, APTR, intData, a1, \
	, LOWLEVEL_BASE_NAME)

#define AddVBlankInt(intRoutine, intData) \
	LP2(0x6c, APTR, AddVBlankInt, APTR, intRoutine, a0, APTR, intData, a1, \
	, LOWLEVEL_BASE_NAME)

#define ElapsedTime(context) \
	LP1(0x66, ULONG, ElapsedTime, struct EClockVal *, context, a0, \
	, LOWLEVEL_BASE_NAME)

#define GetKey() \
	LP0(0x30, ULONG, GetKey, \
	, LOWLEVEL_BASE_NAME)

#define GetLanguageSelection() \
	LP0(0x24, UBYTE, GetLanguageSelection, \
	, LOWLEVEL_BASE_NAME)

#define QueryKeys(queryArray, arraySize) \
	LP2NR(0x36, QueryKeys, struct KeyQuery *, queryArray, a0, unsigned long, arraySize, d1, \
	, LOWLEVEL_BASE_NAME)

#define ReadJoyPort(port) \
	LP1(0x1e, ULONG, ReadJoyPort, unsigned long, port, d0, \
	, LOWLEVEL_BASE_NAME)

#define RemKBInt(intHandle) \
	LP1NR(0x42, RemKBInt, APTR, intHandle, a1, \
	, LOWLEVEL_BASE_NAME)

#define RemTimerInt(intHandle) \
	LP1NR(0x54, RemTimerInt, APTR, intHandle, a1, \
	, LOWLEVEL_BASE_NAME)

#define RemVBlankInt(intHandle) \
	LP1NR(0x72, RemVBlankInt, APTR, intHandle, a1, \
	, LOWLEVEL_BASE_NAME)

#define SetJoyPortAttrsA(portNumber, tagList) \
	LP2(0x84, BOOL, SetJoyPortAttrsA, unsigned long, portNumber, d0, struct TagItem *, tagList, a1, \
	, LOWLEVEL_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetJoyPortAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; SetJoyPortAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define StartTimerInt(intHandle, timeInterval, continuous) \
	LP3NR(0x60, StartTimerInt, APTR, intHandle, a1, unsigned long, timeInterval, d0, long, continuous, d1, \
	, LOWLEVEL_BASE_NAME)

#define StopTimerInt(intHandle) \
	LP1NR(0x5a, StopTimerInt, APTR, intHandle, a1, \
	, LOWLEVEL_BASE_NAME)

#define SystemControlA(tagList) \
	LP1(0x48, ULONG, SystemControlA, struct TagItem *, tagList, a1, \
	, LOWLEVEL_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SystemControl(tags...) \
	({ULONG _tags[] = { tags }; SystemControlA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* _INLINE_LOWLEVEL_H */
