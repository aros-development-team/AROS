#ifndef DEFINES_EXPANSION_H
#define DEFINES_EXPANSION_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AddBootNode(bootPri, flags, deviceNode, configDev) \
    AROS_LC4(BOOL, AddBootNode, \
    AROS_LCA(LONG               , bootPri, D0), \
    AROS_LCA(ULONG              , flags, D1), \
    AROS_LCA(struct DeviceNode *, deviceNode, A0), \
    AROS_LCA(struct ConfigDev  *, configDev, A1), \
    struct ExpansionBase *, ExpansionBase, 6, Expansion)

#define AddConfigDev(configDev) \
    AROS_LC1(void, AddConfigDev, \
    AROS_LCA(struct ConfigDev *, configDev, A0), \
    struct ExpansionBase *, ExpansionBase, 5, Expansion)

#define AddDosNode(bootPri, flags, deviceNode) \
    AROS_LC3(BOOL, AddDosNode, \
    AROS_LCA(LONG               , bootPri, D0), \
    AROS_LCA(ULONG              , flags, D1), \
    AROS_LCA(struct DeviceNode *, deviceNode, A0), \
    struct ExpansionBase *, ExpansionBase, 25, Expansion)

#define AllocBoardMem(slotSpec) \
    AROS_LC1(void, AllocBoardMem, \
    AROS_LCA(ULONG, slotSpec, D0), \
    struct ExpansionBase *, ExpansionBase, 7, Expansion)

#define AllocConfigDev() \
    AROS_LC0(struct ConfigDev *, AllocConfigDev, \
    struct ExpansionBase *, ExpansionBase, 8, Expansion)

#define AllocExpansionMem(numSlots, slotAlign) \
    AROS_LC2(APTR, AllocExpansionMem, \
    AROS_LCA(ULONG, numSlots, D0), \
    AROS_LCA(ULONG, slotAlign, D1), \
    struct ExpansionBase *, ExpansionBase, 9, Expansion)

#define ConfigBoard(board, configDev) \
    AROS_LC2(void, ConfigBoard, \
    AROS_LCA(APTR              , board, A0), \
    AROS_LCA(struct ConfigDev *, configDev, A1), \
    struct ExpansionBase *, ExpansionBase, 10, Expansion)

#define ConfigChain(baseAddr) \
    AROS_LC1(void, ConfigChain, \
    AROS_LCA(APTR, baseAddr, A0), \
    struct ExpansionBase *, ExpansionBase, 11, Expansion)

#define FindConfigDev(oldConfigDev, manufacturer, product) \
    AROS_LC3(struct ConfigDev *, FindConfigDev, \
    AROS_LCA(struct ConfigDev *, oldConfigDev, A0), \
    AROS_LCA(LONG              , manufacturer, D0), \
    AROS_LCA(LONG              , product, D1), \
    struct ExpansionBase *, ExpansionBase, 12, Expansion)

#define FreeBoardMem(startSlot, slotSpec) \
    AROS_LC2(void, FreeBoardMem, \
    AROS_LCA(ULONG, startSlot, D0), \
    AROS_LCA(ULONG, slotSpec, D1), \
    struct ExpansionBase *, ExpansionBase, 13, Expansion)

#define FreeConfigDev(configDev) \
    AROS_LC1(void, FreeConfigDev, \
    AROS_LCA(struct ConfigDev *, configDev, A0), \
    struct ExpansionBase *, ExpansionBase, 14, Expansion)

#define FreeExpansionMem(startSlot, numSlots) \
    AROS_LC2(void, FreeExpansionMem, \
    AROS_LCA(ULONG, startSlot, D0), \
    AROS_LCA(ULONG, numSlots, D1), \
    struct ExpansionBase *, ExpansionBase, 15, Expansion)

#define GetCurrentBinding(currentBinding, bindingSize) \
    AROS_LC2(ULONG, GetCurrentBinding, \
    AROS_LCA(struct CurrentBinding *, currentBinding, A0), \
    AROS_LCA(ULONG                  , bindingSize, D0), \
    struct ExpansionBase *, ExpansionBase, 23, Expansion)

#define MakeDosNode(parmPacket) \
    AROS_LC1(struct DeviceNode *, MakeDosNode, \
    AROS_LCA(APTR, parmPacket, A0), \
    struct ExpansionBase *, ExpansionBase, 24, Expansion)

#define ObtainConfigBinding() \
    AROS_LC0(void, ObtainConfigBinding, \
    struct ExpansionBase *, ExpansionBase, 20, Expansion)

#define ReadExpansionByte(board, offset) \
    AROS_LC2(UBYTE, ReadExpansionByte, \
    AROS_LCA(APTR , board, A0), \
    AROS_LCA(ULONG, offset, D0), \
    struct ExpansionBase *, ExpansionBase, 16, Expansion)

#define ReadExpansionRom(board, configDev) \
    AROS_LC2(void, ReadExpansionRom, \
    AROS_LCA(APTR              , board, A0), \
    AROS_LCA(struct ConfigDev *, configDev, A1), \
    struct ExpansionBase *, ExpansionBase, 17, Expansion)

#define ReleaseConfigBinding() \
    AROS_LC0(void, ReleaseConfigBinding, \
    struct ExpansionBase *, ExpansionBase, 21, Expansion)

#define RemConfigDev(configDev) \
    AROS_LC1(void, RemConfigDev, \
    AROS_LCA(struct ConfigDev *, configDev, A0), \
    struct ExpansionBase *, ExpansionBase, 18, Expansion)

#define SetCurrentBinding(currentBinding, bindingSize) \
    AROS_LC2(void, SetCurrentBinding, \
    AROS_LCA(struct CurrentBinding *, currentBinding, A0), \
    AROS_LCA(ULONG                  , bindingSize, D0), \
    struct ExpansionBase *, ExpansionBase, 22, Expansion)

#define WriteExpansionByte(board, offset, byte) \
    AROS_LC3(void, WriteExpansionByte, \
    AROS_LCA(APTR , board, A0), \
    AROS_LCA(ULONG, offset, D0), \
    AROS_LCA(ULONG, byte, D1), \
    struct ExpansionBase *, ExpansionBase, 19, Expansion)


#endif /* DEFINES_EXPANSION_H */
