#ifndef _INLINE_EXPANSION_H
#define _INLINE_EXPANSION_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef EXPANSION_BASE_NAME
#define EXPANSION_BASE_NAME ExpansionBase
#endif

#define AddBootNode(bootPri, flags, deviceNode, configDev) \
	LP4(0x24, BOOL, AddBootNode, long, bootPri, d0, unsigned long, flags, d1, struct DeviceNode *, deviceNode, a0, struct ConfigDev *, configDev, a1, \
	, EXPANSION_BASE_NAME)

#define AddConfigDev(configDev) \
	LP1NR(0x1e, AddConfigDev, struct ConfigDev *, configDev, a0, \
	, EXPANSION_BASE_NAME)

#define AddDosNode(bootPri, flags, deviceNode) \
	LP3(0x96, BOOL, AddDosNode, long, bootPri, d0, unsigned long, flags, d1, struct DeviceNode *, deviceNode, a0, \
	, EXPANSION_BASE_NAME)

#define AllocBoardMem(slotSpec) \
	LP1NR(0x2a, AllocBoardMem, unsigned long, slotSpec, d0, \
	, EXPANSION_BASE_NAME)

#define AllocConfigDev() \
	LP0(0x30, struct ConfigDev *, AllocConfigDev, \
	, EXPANSION_BASE_NAME)

#define AllocExpansionMem(numSlots, slotAlign) \
	LP2(0x36, APTR, AllocExpansionMem, unsigned long, numSlots, d0, unsigned long, slotAlign, d1, \
	, EXPANSION_BASE_NAME)

#define ConfigBoard(board, configDev) \
	LP2NR(0x3c, ConfigBoard, APTR, board, a0, struct ConfigDev *, configDev, a1, \
	, EXPANSION_BASE_NAME)

#define ConfigChain(baseAddr) \
	LP1NR(0x42, ConfigChain, APTR, baseAddr, a0, \
	, EXPANSION_BASE_NAME)

#define FindConfigDev(oldConfigDev, manufacturer, product) \
	LP3(0x48, struct ConfigDev *, FindConfigDev, struct ConfigDev *, oldConfigDev, a0, long, manufacturer, d0, long, product, d1, \
	, EXPANSION_BASE_NAME)

#define FreeBoardMem(startSlot, slotSpec) \
	LP2NR(0x4e, FreeBoardMem, unsigned long, startSlot, d0, unsigned long, slotSpec, d1, \
	, EXPANSION_BASE_NAME)

#define FreeConfigDev(configDev) \
	LP1NR(0x54, FreeConfigDev, struct ConfigDev *, configDev, a0, \
	, EXPANSION_BASE_NAME)

#define FreeExpansionMem(startSlot, numSlots) \
	LP2NR(0x5a, FreeExpansionMem, unsigned long, startSlot, d0, unsigned long, numSlots, d1, \
	, EXPANSION_BASE_NAME)

#define GetCurrentBinding(currentBinding, bindingSize) \
	LP2(0x8a, ULONG, GetCurrentBinding, struct CurrentBinding *, currentBinding, a0, unsigned long, bindingSize, d0, \
	, EXPANSION_BASE_NAME)

#define MakeDosNode(parmPacket) \
	LP1(0x90, struct DeviceNode *, MakeDosNode, APTR, parmPacket, a0, \
	, EXPANSION_BASE_NAME)

#define ObtainConfigBinding() \
	LP0NR(0x78, ObtainConfigBinding, \
	, EXPANSION_BASE_NAME)

#define ReadExpansionByte(board, offset) \
	LP2(0x60, UBYTE, ReadExpansionByte, APTR, board, a0, unsigned long, offset, d0, \
	, EXPANSION_BASE_NAME)

#define ReadExpansionRom(board, configDev) \
	LP2NR(0x66, ReadExpansionRom, APTR, board, a0, struct ConfigDev *, configDev, a1, \
	, EXPANSION_BASE_NAME)

#define ReleaseConfigBinding() \
	LP0NR(0x7e, ReleaseConfigBinding, \
	, EXPANSION_BASE_NAME)

#define RemConfigDev(configDev) \
	LP1NR(0x6c, RemConfigDev, struct ConfigDev *, configDev, a0, \
	, EXPANSION_BASE_NAME)

#define SetCurrentBinding(currentBinding, bindingSize) \
	LP2NR(0x84, SetCurrentBinding, struct CurrentBinding *, currentBinding, a0, unsigned long, bindingSize, d0, \
	, EXPANSION_BASE_NAME)

#define WriteExpansionByte(board, offset, byte) \
	LP3NR(0x72, WriteExpansionByte, APTR, board, a0, unsigned long, offset, d0, unsigned long, byte, d1, \
	, EXPANSION_BASE_NAME)

#endif /* _INLINE_EXPANSION_H */
