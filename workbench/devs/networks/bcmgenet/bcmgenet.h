
/* $OpenBSD: bcmgenetreg.h,v 1.1 2020/04/14 21:02:39 kettenis Exp $ */
/* $NetBSD: bcmgenetreg.h,v 1.2 2020/02/22 13:41:41 jmcneill Exp $ */

/*
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' ...
 */

/*
 * Copyright (C) 2026, The AROS Development Team.
 *
 * Portions adapted for AROS from OpenBSD/NetBSD bcmgenet sources.
 *
 * Desc: Broadcom GENETv5 (BCM2711 / Raspberry Pi 4) SANA-II driver,
 *       private definitions.
 */

#ifndef BCMGENET_H
#define BCMGENET_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/timer.h>

/*
 * The register map below is the GENETv5 block as wired up on the
 * BCM2711 (Raspberry Pi 4). GENETv1-v4 (older Broadcom SoCs) lay
 * some of this out differently - this driver only claims v5.
 */
#define GENET_SYS_REV_CTRL              0x000
#define GENET_SYS_REV_MAJOR_SHIFT       24
#define GENET_SYS_REV_MAJOR_MASK        (0xf << GENET_SYS_REV_MAJOR_SHIFT)
#define GENET_SYS_REV_MINOR_SHIFT       16
#define GENET_SYS_REV_MINOR_MASK        (0xf << GENET_SYS_REV_MINOR_SHIFT)

#define GENET_SYS_PORT_CTRL             0x004
#define GENET_SYS_PORT_MODE_EXT_GPHY    3

#define GENET_SYS_RBUF_FLUSH_CTRL       0x008
#define GENET_SYS_RBUF_FLUSH_RESET      (1 << 1)

#define GENET_SYS_TBUF_FLUSH_CTRL       0x00c

/* External RGMII out-of-band status/control */
#define GENET_EXT_RGMII_OOB_CTRL            0x08c
#define GENET_EXT_RGMII_OOB_ID_MODE_DISABLE (1 << 16)
#define GENET_EXT_RGMII_OOB_RGMII_MODE_EN   (1 << 6)
#define GENET_EXT_RGMII_OOB_OOB_DISABLE     (1 << 5)
#define GENET_EXT_RGMII_OOB_RGMII_LINK      (1 << 4)

/* Level-2 (CPU-facing) interrupt controller */
#define GENET_INTRL2_CPU_STAT           0x200
#define GENET_INTRL2_CPU_CLEAR          0x208
#define GENET_INTRL2_CPU_STAT_MASK      0x20c
#define GENET_INTRL2_CPU_SET_MASK       0x210
#define GENET_INTRL2_CPU_CLEAR_MASK     0x214
#define GENET_IRQ_MDIO_ERROR            (1 << 24)
#define GENET_IRQ_MDIO_DONE             (1 << 23)
#define GENET_IRQ_TXDMA_DONE            (1 << 16)
#define GENET_IRQ_RXDMA_DONE            (1 << 13)

#define GENET_RBUF_CTRL                 0x300
#define GENET_RBUF_BAD_DIS              (1 << 2)
#define GENET_RBUF_ALIGN_2B             (1 << 1)
#define GENET_RBUF_64B_EN               (1 << 0)

#define GENET_RBUF_TBUF_SIZE_CTRL       0x3b4

/* UniMAC */
#define GENET_UMAC_CMD                  0x808
#define GENET_UMAC_CMD_LCL_LOOP_EN      (1 << 15)
#define GENET_UMAC_CMD_SW_RESET         (1 << 13)
#define GENET_UMAC_CMD_PROMISC          (1 << 4)
#define GENET_UMAC_CMD_SPEED_SHIFT      2
#define GENET_UMAC_CMD_SPEED_MASK       (0x3 << GENET_UMAC_CMD_SPEED_SHIFT)
#define GENET_UMAC_CMD_SPEED_10         0
#define GENET_UMAC_CMD_SPEED_100        1
#define GENET_UMAC_CMD_SPEED_1000       2
#define GENET_UMAC_CMD_RXEN             (1 << 1)
#define GENET_UMAC_CMD_TXEN             (1 << 0)

#define GENET_UMAC_MAC0                 0x80c
#define GENET_UMAC_MAC1                 0x810
#define GENET_UMAC_MAX_FRAME_LEN        0x814

#define GENET_UMAC_TX_FLUSH             0xb34

#define GENET_UMAC_MIB_CTRL             0xd80
#define GENET_UMAC_MIB_RESET_TX         (1 << 2)
#define GENET_UMAC_MIB_RESET_RUNT       (1 << 1)
#define GENET_UMAC_MIB_RESET_RX         (1 << 0)

/* MDIO, reached through the UniMAC block */
#define GENET_MDIO_CMD                  0xe14
#define GENET_MDIO_START_BUSY           (1 << 29)
#define GENET_MDIO_READ                 (1 << 27)
#define GENET_MDIO_WRITE                (1 << 26)
#define GENET_MDIO_PMD_SHIFT            21
#define GENET_MDIO_PMD_MASK             (0x1f << GENET_MDIO_PMD_SHIFT)
#define GENET_MDIO_REG_SHIFT            16
#define GENET_MDIO_REG_MASK             (0x1f << GENET_MDIO_REG_SHIFT)

#define GENET_UMAC_MDF_CTRL             0xe50
#define GENET_UMAC_MDF_ADDR0(n)         (0xe54 + (n) * 0x8)
#define GENET_UMAC_MDF_ADDR1(n)         (0xe58 + (n) * 0x8)
/* 17 exact-match entries, enabled from the top bit down: entry n is
 * bit (GENET_MAX_MDF_FILTER - 1 - n) of GENET_UMAC_MDF_CTRL. */
#define GENET_MAX_MDF_FILTER            17
#define GENET_MDF_CTRL_ENABLE(n)        ((((1UL << (n)) - 1)) << (GENET_MAX_MDF_FILTER - (n)))

/* DMA rings share one descriptor pool per direction, indexed 0-15 plus
 * a 17th "default" queue (16) that this driver is the only user of. */
#define GENET_DMA_DESC_COUNT            256
#define GENET_DMA_DESC_SIZE             12
#define GENET_DMA_DEFAULT_QUEUE         16

#define GENET_DMA_RING_SIZE             0x40
#define GENET_DMA_RINGS_SIZE            (GENET_DMA_RING_SIZE * (GENET_DMA_DEFAULT_QUEUE + 1))

#define GENET_RX_BASE                   0x2000
#define GENET_TX_BASE                   0x4000

#define GENET_RX_DMA_RINGBASE(qid)      (GENET_RX_BASE + 0xc00 + GENET_DMA_RING_SIZE * (qid))
#define GENET_RX_DMA_WRITE_PTR_LO(qid)  (GENET_RX_DMA_RINGBASE(qid) + 0x00)
#define GENET_RX_DMA_WRITE_PTR_HI(qid)  (GENET_RX_DMA_RINGBASE(qid) + 0x04)
#define GENET_RX_DMA_PROD_INDEX(qid)    (GENET_RX_DMA_RINGBASE(qid) + 0x08)
#define GENET_RX_DMA_CONS_INDEX(qid)    (GENET_RX_DMA_RINGBASE(qid) + 0x0c)
#define GENET_RX_DMA_RING_BUF_SIZE(qid) (GENET_RX_DMA_RINGBASE(qid) + 0x10)
#define GENET_RX_DMA_START_ADDR_LO(qid) (GENET_RX_DMA_RINGBASE(qid) + 0x14)
#define GENET_RX_DMA_START_ADDR_HI(qid) (GENET_RX_DMA_RINGBASE(qid) + 0x18)
#define GENET_RX_DMA_END_ADDR_LO(qid)   (GENET_RX_DMA_RINGBASE(qid) + 0x1c)
#define GENET_RX_DMA_END_ADDR_HI(qid)   (GENET_RX_DMA_RINGBASE(qid) + 0x20)
#define GENET_RX_DMA_XON_XOFF_THRES(qid) (GENET_RX_DMA_RINGBASE(qid) + 0x28)
#define GENET_RX_DMA_READ_PTR_LO(qid)   (GENET_RX_DMA_RINGBASE(qid) + 0x2c)
#define GENET_RX_DMA_READ_PTR_HI(qid)   (GENET_RX_DMA_RINGBASE(qid) + 0x30)

#define GENET_TX_DMA_RINGBASE(qid)      (GENET_TX_BASE + 0xc00 + GENET_DMA_RING_SIZE * (qid))
#define GENET_TX_DMA_READ_PTR_LO(qid)   (GENET_TX_DMA_RINGBASE(qid) + 0x00)
#define GENET_TX_DMA_READ_PTR_HI(qid)   (GENET_TX_DMA_RINGBASE(qid) + 0x04)
#define GENET_TX_DMA_CONS_INDEX(qid)    (GENET_TX_DMA_RINGBASE(qid) + 0x08)
#define GENET_TX_DMA_PROD_INDEX(qid)    (GENET_TX_DMA_RINGBASE(qid) + 0x0c)
#define GENET_TX_DMA_RING_BUF_SIZE(qid) (GENET_TX_DMA_RINGBASE(qid) + 0x10)
#define GENET_TX_DMA_START_ADDR_LO(qid) (GENET_TX_DMA_RINGBASE(qid) + 0x14)
#define GENET_TX_DMA_START_ADDR_HI(qid) (GENET_TX_DMA_RINGBASE(qid) + 0x18)
#define GENET_TX_DMA_END_ADDR_LO(qid)   (GENET_TX_DMA_RINGBASE(qid) + 0x1c)
#define GENET_TX_DMA_END_ADDR_HI(qid)   (GENET_TX_DMA_RINGBASE(qid) + 0x20)
#define GENET_TX_DMA_MBUF_DONE_THRES(qid) (GENET_TX_DMA_RINGBASE(qid) + 0x24)
#define GENET_TX_DMA_FLOW_PERIOD(qid)   (GENET_TX_DMA_RINGBASE(qid) + 0x28)
#define GENET_TX_DMA_WRITE_PTR_LO(qid)  (GENET_TX_DMA_RINGBASE(qid) + 0x2c)
#define GENET_TX_DMA_WRITE_PTR_HI(qid)  (GENET_TX_DMA_RINGBASE(qid) + 0x30)

/*
 * Per-descriptor state is a set of registers indexed by slot, not a
 * structure in RAM: there is no ring to DMA-map, only a buffer address
 * to hand the engine for each of the 256 slots per direction.
 */
#define GENET_RX_DESC_STATUS(idx)       (GENET_RX_BASE + GENET_DMA_DESC_SIZE * (idx) + 0x00)
#define GENET_RX_DESC_STATUS_BUFLEN_SHIFT 16
#define GENET_RX_DESC_STATUS_BUFLEN_MASK (0xfff << GENET_RX_DESC_STATUS_BUFLEN_SHIFT)
#define GENET_RX_DESC_STATUS_OWN        (1 << 15)
#define GENET_RX_DESC_STATUS_EOP        (1 << 14)
#define GENET_RX_DESC_STATUS_SOP        (1 << 13)
#define GENET_RX_DESC_STATUS_RX_ERROR   (1 << 2)
#define GENET_RX_DESC_ADDRESS_LO(idx)   (GENET_RX_BASE + GENET_DMA_DESC_SIZE * (idx) + 0x04)
#define GENET_RX_DESC_ADDRESS_HI(idx)   (GENET_RX_BASE + GENET_DMA_DESC_SIZE * (idx) + 0x08)

#define GENET_TX_DESC_STATUS(idx)       (GENET_TX_BASE + GENET_DMA_DESC_SIZE * (idx) + 0x00)
#define GENET_TX_DESC_STATUS_BUFLEN_SHIFT 16
#define GENET_TX_DESC_STATUS_BUFLEN_MASK (0xfff << GENET_TX_DESC_STATUS_BUFLEN_SHIFT)
#define GENET_TX_DESC_STATUS_OWN        (1 << 15)
#define GENET_TX_DESC_STATUS_EOP        (1 << 14)
#define GENET_TX_DESC_STATUS_SOP        (1 << 13)
#define GENET_TX_DESC_STATUS_QTAG_SHIFT 7
#define GENET_TX_DESC_STATUS_QTAG_MASK  (0x3f << GENET_TX_DESC_STATUS_QTAG_SHIFT)
#define GENET_TX_DESC_STATUS_CRC        (1 << 6)
#define GENET_TX_DESC_ADDRESS_LO(idx)   (GENET_TX_BASE + GENET_DMA_DESC_SIZE * (idx) + 0x04)
#define GENET_TX_DESC_ADDRESS_HI(idx)   (GENET_TX_BASE + GENET_DMA_DESC_SIZE * (idx) + 0x08)

#define GENET_RX_DMA_RING_CFG           (GENET_RX_BASE + 0x1040 + 0x00)
#define GENET_RX_DMA_CTRL               (GENET_RX_BASE + 0x1040 + 0x04)
#define GENET_RX_DMA_CTRL_RBUF_EN(qid)  (1 << ((qid) + 1))
#define GENET_RX_DMA_CTRL_EN            (1 << 0)
#define GENET_RX_SCB_BURST_SIZE         (GENET_RX_BASE + 0x1040 + 0x0c)

#define GENET_TX_DMA_RING_CFG           (GENET_TX_BASE + 0x1040 + 0x00)
#define GENET_TX_DMA_CTRL               (GENET_TX_BASE + 0x1040 + 0x04)
#define GENET_TX_DMA_CTRL_RBUF_EN(qid)  (1 << ((qid) + 1))
#define GENET_TX_DMA_CTRL_EN            (1 << 0)
#define GENET_TX_SCB_BURST_SIZE         (GENET_TX_BASE + 0x1040 + 0x0c)

/*
 * Standard MII/MDIO registers - every PHY answers these regardless of
 * make. The BCM54213PE on the Pi 4B is reached this way, through the
 * UniMAC's own MDIO master (GENET_MDIO_CMD), not a separate controller.
 */
#define MII_BMCR                        0x00
#define MII_BMSR                        0x01
#define MII_PHYSID1                     0x02
#define MII_PHYSID2                     0x03
#define MII_ANAR                        0x04
#define MII_ANLPAR                      0x05
#define MII_GBCR                        0x09
#define MII_GBSR                        0x0a

#define BMCR_RESET                      0x8000
#define BMCR_ANENABLE                   0x1000
#define BMCR_ANRESTART                  0x0200

#define BMSR_LSTATUS                    0x0004
#define BMSR_ANEGCOMPLETE               0x0020

#define ANAR_CSMA                       0x0001
#define ANAR_10HD                       0x0020
#define ANAR_10FD                       0x0040
#define ANAR_100HD                      0x0080
#define ANAR_100FD                      0x0100

#define GBCR_1000HD                     0x0100
#define GBCR_1000FD                     0x0200
#define GBSR_LP1000HD                   0x0400
#define GBSR_LP1000FD                   0x0800

/* Ethernet on the wire */
#define ETH_ADDRESSSIZE                 6
#define ETH_HEADERSIZE                  14
#define ETH_MTU                         1500
#define ETH_MAXPACKETSIZE               (ETH_HEADERSIZE + ETH_MTU)
#define ETH_ZLEN                        60

struct eth_frame
{
    UBYTE               eth_packet_dest[ETH_ADDRESSSIZE];
    UBYTE               eth_packet_source[ETH_ADDRESSSIZE];
    UWORD               eth_packet_type;
    UBYTE               eth_packet_data[];
};

/* Ring geometry. A buffer holds one whole frame; no chaining. */
#define GENET_BUFSIZE                   1536 /* Ethernet header, MTU, and spare room */

/*
 * How the PHY is strapped to the MAC. The Pi 4B ties RXID (the RX
 * clock gets the internal delay, TX does not) - see the "phy-mode"
 * property on the &genet node in bcm2711-rpi-4-b.dts.
 */
enum genet_phy_mode
{
    GENET_PHY_MODE_RGMII,
    GENET_PHY_MODE_RGMII_ID,
    GENET_PHY_MODE_RGMII_TXID,
    GENET_PHY_MODE_RGMII_RXID,
};

/*
 * What the device tree told us about the controller. Filled in by
 * bcmgenet_dt.c, read by everyone else.
 */
struct bcmgenet_hw
{
    IPTR                base;           /* register block, ARM-side address */
    IPTR                phys;           /* and where it really lives        */
    IPTR                size;
    ULONG               irq[2];         /* GIC SPI 157/158 on the BCM2711   */
    ULONG               phyAddr;        /* PHY address on the mdio bus      */
    enum genet_phy_mode phyMode;
    UBYTE               macAddr[ETH_ADDRESSSIZE];
    BOOL                haveMacAddr;
};

/*
 * The known physical address of the block on a Raspberry Pi 4 (BCM2711),
 * as a fallback if the device tree cannot be read for some reason. The
 * node lives on the "scb" bus, which is windowed differently than the
 * legacy VideoCore peripheral window RPiHDMI/RPiPWM read from - do not
 * reuse BCM2711_BUS_PERIIOBASE-style translation here.
 */
#define GENET_PHYS_BASE                 0xFD580000UL
#define GENET_PHYS_SIZE                 0x10000UL

/* Deferred request queues, indices into bgu_RequestPorts */
enum
{
    WRITE_QUEUE,
    ADOPT_QUEUE,
    EVENT_QUEUE,
    GENERAL_QUEUE,
    REQUEST_QUEUE_COUNT
};

struct Opener
{
    struct MinNode      node;
    struct MsgPort      read_port;
    BOOL                (*rx_function)(APTR, APTR, ULONG);
    BOOL                (*tx_function)(APTR, APTR, ULONG);
    struct Hook        *filter_hook;
    struct MinList      initial_stats;
};

struct TypeStats
{
    struct MinNode      node;
    ULONG               packet_type;
    struct Sana2PacketTypeStats stats;
};

struct TypeTracker
{
    struct MinNode      node;
    ULONG               packet_type;
    struct Sana2PacketTypeStats stats;
    ULONG               user_count;
};

struct AddressRange
{
    struct MinNode      node;
    ULONG               add_count;
    ULONG               lower_bound_left;
    ULONG               upper_bound_left;
    UWORD               lower_bound_right;
    UWORD               upper_bound_right;
};

#define STAT_COUNT                      3

/* bgu_Flags */
#define IFF_UP                          0x0001
#define IFF_CONFIGURED                  0x0002
#define IFF_PROMISC                     0x0004

/*
 * One ring direction's software-side bookkeeping. The hardware side
 * of a descriptor is a set of registers per slot (see GENET_RX_DESC_*
 * / GENET_TX_DESC_* above), so what needs to live in RAM is just the
 * buffer this slot's DMA address points at, plus where the ring is up
 * to - not a descriptor struct to keep in sync with the engine.
 */
#define GENET_DMA_RING_BUF_SIZE_DESC_COUNT_SHIFT  16
#define GENET_DMA_RING_BUF_SIZE_DESC_COUNT_MASK   0xffff0000
#define GENET_DMA_RING_BUF_SIZE_BUF_LENGTH_SHIFT  0
#define GENET_DMA_RING_BUF_SIZE_BUF_LENGTH_MASK   0x0000ffff
#define GENET_RX_DMA_XON_XOFF_THRES_LO_SHIFT  16
#define GENET_RX_DMA_XON_XOFF_THRES_LO_MASK   0xffff0000
#define GENET_RX_DMA_XON_XOFF_THRES_HI_SHIFT  0
#define GENET_RX_DMA_XON_XOFF_THRES_HI_MASK   0x0000ffff

struct bcmgenet_ring
{
    APTR                bufmem;         /* one allocation, N * GENET_BUFSIZE */
    ULONG               bufmemsize;
    UBYTE              *buf[GENET_DMA_DESC_COUNT]; /* slot -> buffer pointer */
    ULONG               cidx;           /* consumer index                   */
    ULONG               pidx;           /* producer index                   */
    ULONG               next;
};

struct BCMGENETUnit
{
    struct BCMGENETBase *bgu_Base;
    struct bcmgenet_hw *bgu_HW;

    struct SignalSemaphore bgu_Lock;
    struct MinList      bgu_Openers;
    struct MinList      bgu_MulticastRanges;
    struct MinList      bgu_TypeTrackers;
    LONG                bgu_RangeCount;
    ULONG               bgu_OpenCount;
    ULONG               bgu_Flags;

    struct MsgPort     *bgu_InputPort;
    struct MsgPort     *bgu_RequestPorts[REQUEST_QUEUE_COUNT];

    struct Interrupt    bgu_IRQHandler[2];
    BOOL                bgu_IRQAdded[2];

    UBYTE               bgu_DevAddr[ETH_ADDRESSSIZE];
    UBYTE               bgu_OrgAddr[ETH_ADDRESSSIZE];

    ULONG               bgu_SpeedMbps;
    BOOL                bgu_FullDuplex;
    BOOL                bgu_LinkUp;

    struct Sana2DeviceQuery bgu_Sana2Info;
    struct Sana2DeviceStats bgu_Stats;
    ULONG               bgu_SpecialStats[STAT_COUNT];

    struct bcmgenet_ring bgu_TX;
    struct bcmgenet_ring bgu_RX;

    struct MsgPort     *bgu_TimerPort;
    struct timerequest *bgu_TimerReq;
};

struct BCMGENETBase
{
    struct Device       bgm_Device;
    APTR                bgm_KernelBase;
    APTR                bgm_UtilityBase;
    APTR                bgm_OpenFirmwareBase;
    struct bcmgenet_hw  bgm_HW;
    BOOL                bgm_Found;
    struct BCMGENETUnit *bgm_Unit;
    struct SignalSemaphore bgm_UnitSem; /* serialises unit creation */
};

/* Pulls in genmodule's GM_UNIQUENAME / LIBBASE / LIBBASETYPEPTR macros.
 * Must follow the struct BCMGENETBase definition (libbasetype). */
#include LC_LIBDEFS_FILE

/* bcmgenet_dt.c - finding the controller on a BCM2711 device tree */
BOOL BCMGENET_Discover(struct BCMGENETBase *base, struct bcmgenet_hw *hw);

/* bcmgenet_hw.c - register, mdio and PHY access */
ULONG BCMGENET_Read(struct bcmgenet_hw *hw, ULONG reg);
void BCMGENET_Write(struct bcmgenet_hw *hw, ULONG reg, ULONG val);
LONG BCMGENET_MDIORead(struct bcmgenet_hw *hw, ULONG phy, ULONG reg);
BOOL BCMGENET_MDIOWrite(struct bcmgenet_hw *hw, ULONG phy, ULONG reg, UWORD val);
BOOL BCMGENET_HWReset(struct BCMGENETUnit *unit);
BOOL BCMGENET_HWInit(struct BCMGENETUnit *unit);
void BCMGENET_SetMACAddress(struct bcmgenet_hw *hw, const UBYTE *addr);
BOOL BCMGENET_GetMACAddress(struct bcmgenet_hw *hw, UBYTE *addr);
BOOL BCMGENET_PHYInit(struct bcmgenet_hw *hw);
BOOL BCMGENET_PHYGetLink(struct bcmgenet_hw *hw, ULONG *mbps, BOOL *fullduplex);

/* bcmgenet_unit.c */
struct BCMGENETUnit *BCMGENET_CreateUnit(struct BCMGENETBase *base);
void BCMGENET_DeleteUnit(struct BCMGENETBase *base, struct BCMGENETUnit *unit);
void BCMGENET_GoOnline(struct BCMGENETBase *base, struct BCMGENETUnit *unit);
void BCMGENET_GoOffline(struct BCMGENETBase *base, struct BCMGENETUnit *unit);
BOOL BCMGENET_AddressFilter(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                            UBYTE *address);
BOOL BCMGENET_SendPacket(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                         struct IOSana2Req *request);
void BCMGENET_ReportEvents(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                           ULONG events);
struct TypeStats *BCMGENET_FindTypeStats(struct MinList *list, ULONG packet_type);
BOOL BCMGENET_AddMulticastRange(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                                const UBYTE *lower, const UBYTE *upper);
BOOL BCMGENET_RemMulticastRange(struct BCMGENETBase *base, struct BCMGENETUnit *unit,
                                const UBYTE *lower, const UBYTE *upper);

/* bcmgenet_handler.c */
void BCMGENET_HandleRequest(struct BCMGENETBase *base, struct IOSana2Req *request);

#endif /* BCMGENET_H */
