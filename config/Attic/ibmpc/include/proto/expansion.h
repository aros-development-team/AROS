#ifndef PROTO_EXPANSION_H
#define PROTO_EXPANSION_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef EXPANSION_BASE_NAME
#define EXPANSION_BASE_NAME ExpansionBase
#endif

#define AddBootNode(bootPri, flags, deviceNode, configDev) \
	LP4(0x18, BOOL, AddBootNode, long, bootPri, unsigned long, flags, struct DeviceNode *, deviceNode, struct ConfigDev *, configDev, \
	, EXPANSION_BASE_NAME)

#define AddConfigDev(configDev) \
	LP1NR(0x14, AddConfigDev, struct ConfigDev *, configDev, \
	, EXPANSION_BASE_NAME)

#define AddDosNode(bootPri, flags, deviceNode) \
	LP3(0x64, BOOL, AddDosNode, long, bootPri, unsigned long, flags, struct DeviceNode *, deviceNode, \
	, EXPANSION_BASE_NAME)

#define AllocBoardMem(slotSpec) \
	LP1NR(0x1c, AllocBoardMem, unsigned long, slotSpec, \
	, EXPANSION_BASE_NAME)

#define AllocConfigDev() \
	LP0(0x20, struct ConfigDev *, AllocConfigDev, \
	, EXPANSION_BASE_NAME)

#define AllocExpansionMem(numSlots, slotAlign) \
	LP2(0x24, APTR, AllocExpansionMem, unsigned long, numSlots, unsigned long, slotAlign, \
	, EXPANSION_BASE_NAME)

#define ConfigBoard(board, configDev) \
	LP2NR(0x28, ConfigBoard, APTR, board, struct ConfigDev *, configDev, \
	, EXPANSION_BASE_NAME)

#define ConfigChain(baseAddr) \
	LP1NR(0x2c, ConfigChain, APTR, baseAddr, \
	, EXPANSION_BASE_NAME)

#define FindConfigDev(oldConfigDev, manufacturer, product) \
	LP3(0x30, struct ConfigDev *, FindConfigDev, struct ConfigDev *, oldConfigDev, long, manufacturer, long, product, \
	, EXPANSION_BASE_NAME)

#define FreeBoardMem(startSlot, slotSpec) \
	LP2NR(0x34, FreeBoardMem, unsigned long, startSlot, unsigned long, slotSpec, \
	, EXPANSION_BASE_NAME)

#define FreeConfigDev(configDev) \
	LP1NR(0x38, FreeConfigDev, struct ConfigDev *, configDev, \
	, EXPANSION_BASE_NAME)

#define FreeExpansionMem(startSlot, numSlots) \
	LP2NR(0x3c, FreeExpansionMem, unsigned long, startSlot, unsigned long, numSlots, \
	, EXPANSION_BASE_NAME)

#define GetCurrentBinding(currentBinding, bindingSize) \
	LP2(0x5c, ULONG, GetCurrentBinding, struct CurrentBinding *, currentBinding, unsigned long, bindingSize, \
	, EXPANSION_BASE_NAME)

#define MakeDosNode(parmPacket) \
	LP1(0x60, struct DeviceNode *, MakeDosNode, APTR, parmPacket, \
	, EXPANSION_BASE_NAME)

#define ObtainConfigBinding() \
	LP0NR(0x50, ObtainConfigBinding, \
	, EXPANSION_BASE_NAME)

#define ReadExpansionByte(board, offset) \
	LP2(0x40, UBYTE, ReadExpansionByte, APTR, board, unsigned long, offset, \
	, EXPANSION_BASE_NAME)

#define ReadExpansionRom(board, configDev) \
	LP2NR(0x44, ReadExpansionRom, APTR, board, struct ConfigDev *, configDev, \
	, EXPANSION_BASE_NAME)

#define ReleaseConfigBinding() \
	LP0NR(0x54, ReleaseConfigBinding, \
	, EXPANSION_BASE_NAME)

#define RemConfigDev(configDev) \
	LP1NR(0x48, RemConfigDev, struct ConfigDev *, configDev, \
	, EXPANSION_BASE_NAME)

#define SetCurrentBinding(currentBinding, bindingSize) \
	LP2NR(0x58, SetCurrentBinding, struct CurrentBinding *, currentBinding, unsigned long, bindingSize, \
	, EXPANSION_BASE_NAME)

#define WriteExpansionByte(board, offset, byte) \
	LP3NR(0x4c, WriteExpansionByte, APTR, board, unsigned long, offset, unsigned long, byte, \
	, EXPANSION_BASE_NAME)

#endif /* PROTO_EXPANSION_H */
