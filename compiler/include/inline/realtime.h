/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_REALTIME_H
#define _INLINE_REALTIME_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef REALTIME_BASE_NAME
#define REALTIME_BASE_NAME RealTimeBase
#endif

#define CreatePlayerA(tagList) \
	LP1(0x2a, struct Player *, CreatePlayerA, struct TagItem *, tagList, a0, \
	, REALTIME_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CreatePlayer(tags...) \
	({ULONG _tags[] = { tags }; CreatePlayerA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define DeletePlayer(player) \
	LP1NR(0x30, DeletePlayer, struct Player *, player, a0, \
	, REALTIME_BASE_NAME)

#define ExternalSync(player, minTime, maxTime) \
	LP3(0x42, BOOL, ExternalSync, struct Player *, player, a0, long, minTime, d0, long, maxTime, d1, \
	, REALTIME_BASE_NAME)

#define FindConductor(name) \
	LP1(0x4e, struct Conductor *, FindConductor, STRPTR, name, a0, \
	, REALTIME_BASE_NAME)

#define GetPlayerAttrsA(player, tagList) \
	LP2(0x54, ULONG, GetPlayerAttrsA, struct Player *, player, a0, struct TagItem *, tagList, a1, \
	, REALTIME_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetPlayerAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; GetPlayerAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define LockRealTime(lockType) \
	LP1(0x1e, APTR, LockRealTime, unsigned long, lockType, d0, \
	, REALTIME_BASE_NAME)

#define NextConductor(previousConductor) \
	LP1(0x48, struct Conductor *, NextConductor, struct Conductor *, previousConductor, a0, \
	, REALTIME_BASE_NAME)

#define SetConductorState(player, state, time) \
	LP3(0x3c, LONG, SetConductorState, struct Player *, player, a0, unsigned long, state, d0, long, time, d1, \
	, REALTIME_BASE_NAME)

#define SetPlayerAttrsA(player, tagList) \
	LP2(0x36, BOOL, SetPlayerAttrsA, struct Player *, player, a0, struct TagItem *, tagList, a1, \
	, REALTIME_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetPlayerAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; SetPlayerAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define UnlockRealTime(lock) \
	LP1NR(0x24, UnlockRealTime, APTR, lock, a0, \
	, REALTIME_BASE_NAME)

#endif /* _INLINE_REALTIME_H */
