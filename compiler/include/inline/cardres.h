/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_CARDRES_H
#define _INLINE_CARDRES_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef CARDRES_BASE_NAME
#define CARDRES_BASE_NAME CardResource
#endif

#define BeginCardAccess(handle) \
	LP1(0x18, BOOL, BeginCardAccess, struct CardHandle *, handle, a1, \
	, CARDRES_BASE_NAME)

#define CardAccessSpeed(handle, nanoseconds) \
	LP2(0x36, ULONG, CardAccessSpeed, struct CardHandle *, handle, a1, unsigned long, nanoseconds, d0, \
	, CARDRES_BASE_NAME)

#define CardChangeCount() \
	LP0(0x60, ULONG, CardChangeCount, \
	, CARDRES_BASE_NAME)

#define CardForceChange() \
	LP0(0x5a, BOOL, CardForceChange, \
	, CARDRES_BASE_NAME)

#define CardInterface() \
	LP0(0x66, ULONG, CardInterface, \
	, CARDRES_BASE_NAME)

#define CardMiscControl(handle, control_bits) \
	LP2(0x30, UBYTE, CardMiscControl, struct CardHandle *, handle, a1, unsigned long, control_bits, d1, \
	, CARDRES_BASE_NAME)

#define CardProgramVoltage(handle, voltage) \
	LP2(0x3c, LONG, CardProgramVoltage, struct CardHandle *, handle, a1, unsigned long, voltage, d0, \
	, CARDRES_BASE_NAME)

#define CardResetCard(handle) \
	LP1(0x42, BOOL, CardResetCard, struct CardHandle *, handle, a1, \
	, CARDRES_BASE_NAME)

#define CardResetRemove(handle, flag) \
	LP2(0x2a, BOOL, CardResetRemove, struct CardHandle *, handle, a1, unsigned long, flag, d0, \
	, CARDRES_BASE_NAME)

#define CopyTuple(handle, buffer, tuplecode, size) \
	LP4(0x48, BOOL, CopyTuple, struct CardHandle *, handle, a1, UBYTE *, buffer, a0, unsigned long, tuplecode, d1, unsigned long, size, d0, \
	, CARDRES_BASE_NAME)

#define DeviceTuple(tuple_data, storage) \
	LP2(0x4e, ULONG, DeviceTuple, UBYTE *, tuple_data, a0, struct DeviceTData *, storage, a1, \
	, CARDRES_BASE_NAME)

#define EndCardAccess(handle) \
	LP1(0x1e, BOOL, EndCardAccess, struct CardHandle *, handle, a1, \
	, CARDRES_BASE_NAME)

#define GetCardMap() \
	LP0(0x12, struct CardMemoryMap *, GetCardMap, \
	, CARDRES_BASE_NAME)

#define IfAmigaXIP(handle) \
	LP1(0x54, struct Resident *, IfAmigaXIP, struct CardHandle *, handle, a2, \
	, CARDRES_BASE_NAME)

#define OwnCard(handle) \
	LP1(0x6, struct CardHandle *, OwnCard, struct CardHandle *, handle, a1, \
	, CARDRES_BASE_NAME)

#define ReadCardStatus() \
	LP0(0x24, UBYTE, ReadCardStatus, \
	, CARDRES_BASE_NAME)

#define ReleaseCard(handle, flags) \
	LP2NR(0xc, ReleaseCard, struct CardHandle *, handle, a1, unsigned long, flags, d0, \
	, CARDRES_BASE_NAME)

#endif /* _INLINE_CARDRES_H */
