/*
 * Copyright (c) 2021-2022 Visa Hankala
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Cadence GEM (Raspberry Pi 5 / RP1) SANA-II driver, private
          definitions. Register map from OpenBSD sys/dev/fdt/if_cad.c.
*/

#ifndef CDNSGEM_H
#define CDNSGEM_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <oop/oop.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/timer.h>

#define GEM_NETCTL                      0x0000
#define GEM_NETCTL_STARTTX              (1 << 9)
#define GEM_NETCTL_STATCLR              (1 << 5)
#define GEM_NETCTL_MDEN                 (1 << 4)
#define GEM_NETCTL_TXEN                 (1 << 3)
#define GEM_NETCTL_RXEN                 (1 << 2)
#define GEM_NETCFG                      0x0004
#define GEM_NETCFG_SGMIIEN              (1 << 27)
#define GEM_NETCFG_RXCSUMEN             (1 << 24)
#define GEM_NETCFG_MDCCLKDIV_MASK       (0x7 << 18)
#define GEM_NETCFG_MDCCLKDIV_SHIFT      18
#define GEM_NETCFG_FCSREM               (1 << 17)
#define GEM_NETCFG_RXOFFS_MASK          (0x3 << 14)
#define GEM_NETCFG_RXOFFS_SHIFT         14
#define GEM_NETCFG_PCSSEL               (1 << 11)
#define GEM_NETCFG_1000                 (1 << 10)
#define GEM_NETCFG_1536RXEN             (1 << 8)
#define GEM_NETCFG_UCASTHASHEN          (1 << 7)
#define GEM_NETCFG_MCASTHASHEN          (1 << 6)
#define GEM_NETCFG_BCASTDI              (1 << 5)
#define GEM_NETCFG_COPYALL              (1 << 4)
#define GEM_NETCFG_FDEN                 (1 << 1)
#define GEM_NETCFG_100                  (1 << 0)
#define GEM_NETSR                       0x0008
#define GEM_NETSR_PHY_MGMT_IDLE         (1 << 2)
#define GEM_DMACR                       0x0010
#define GEM_DMACR_DMA64                 (1 << 30)
#define GEM_DMACR_AHBDISC               (1 << 24)
#define GEM_DMACR_RXBUF_MASK            (0xff << 16)
#define GEM_DMACR_RXBUF_SHIFT           16
#define GEM_DMACR_TXCSUMEN              (1 << 11)
#define GEM_DMACR_TXSIZE                (1 << 10)
#define GEM_DMACR_RXSIZE_MASK           (0x3 << 8)
#define GEM_DMACR_RXSIZE_8K             (0x3 << 8)
#define GEM_DMACR_ES_PDATA              (1 << 7)
#define GEM_DMACR_ES_DESCR              (1 << 6)
#define GEM_DMACR_BLEN_MASK             (0x1f << 0)
#define GEM_DMACR_BLEN_16               (0x10 << 0)
#define GEM_TXSR                        0x0014
#define GEM_TXSR_TXGO                   (1 << 3)
#define GEM_RXQBASE                     0x0018
#define GEM_TXQBASE                     0x001c
#define GEM_RXSR                        0x0020
#define GEM_ISR                         0x0024
#define GEM_IER                         0x0028
#define GEM_IDR                         0x002c
#define GEM_IXR_HRESP                   (1 << 11)
#define GEM_IXR_RXOVR                   (1 << 10)
#define GEM_IXR_TXDONE                  (1 << 7)
#define GEM_IXR_TXURUN                  (1 << 6)
#define GEM_IXR_RETRY                   (1 << 5)
#define GEM_IXR_TXUSED                  (1 << 3)
#define GEM_IXR_RXDONE                  (1 << 1)
#define GEM_PHYMNTNC                    0x0034
#define GEM_PHYMNTNC_CLAUSE_22          (1 << 30)
#define GEM_PHYMNTNC_OP_READ            (0x2 << 28)
#define GEM_PHYMNTNC_OP_WRITE           (0x1 << 28)
#define GEM_PHYMNTNC_ADDR_MASK          (0x1f << 23)
#define GEM_PHYMNTNC_ADDR_SHIFT         23
#define GEM_PHYMNTNC_REG_MASK           (0x1f << 18)
#define GEM_PHYMNTNC_REG_SHIFT          18
#define GEM_PHYMNTNC_MUST_10            (0x2 << 16)
#define GEM_PHYMNTNC_DATA_MASK          0xffff
#define GEM_HASHL                       0x0080
#define GEM_HASHH                       0x0084
#define GEM_LADDRL(i)                   (0x0088 + (i) * 8)
#define GEM_LADDRH(i)                   (0x008c + (i) * 8)
#define GEM_LADDRNUM                    4
#define GEM_MID                         0x00fc
#define GEM_MID_VERSION_MASK            (0xfff << 16)
#define GEM_MID_VERSION_SHIFT           16
#define GEM_CFG6                        0x0294
#define GEM_CFG6_DMA64                  (1 << 23)
#define GEM_CFG6_PRIQ_MASK(x)           ((x) & 0xffff)
#define GEM_CFG8                        0x029c
#define GEM_CFG8_NUM_TYPE1_SCR(x)       (((x) >> 24) & 0xff)
#define GEM_CFG8_NUM_TYPE2_SCR(x)       (((x) >> 16) & 0xff)
#define GEM_TXQ1BASE(i)                 (0x0440 + (i) * 4)
#define GEM_TXQ1BASE_DISABLE            (1 << 0)
#define GEM_RXQ1BASE(i)                 (0x0480 + (i) * 4)
#define GEM_RXQ1BASE_DISABLE            (1 << 0)
#define GEM_TXQBASEHI                   0x04c8
#define GEM_RXQBASEHI                   0x04d4
#define GEM_SCR_TYPE1(i)                (0x0500 + (i) * 4)
#define GEM_SCR_TYPE2(i)                (0x0540 + (i) * 4)
#define GEM_RXQ8BASE(i)                 (0x05c0 + (i) * 4)
#define GEM_RXQ8BASE_DISABLE            (1 << 0)

#define GEM_MAX_PRIQ                    16

/* Interrupts the driver acts on; the handler loops until none is left. */
#define GEM_IXR_USED_MASK \
    (GEM_IXR_HRESP | GEM_IXR_RXOVR | GEM_IXR_RXDONE | GEM_IXR_TXDONE | \
     GEM_IXR_TXUSED | GEM_IXR_RETRY | GEM_IXR_TXURUN)

/*
 * 64-bit descriptor. The upper halves of the ring base addresses are one
 * register per direction, shared by every queue - so all rings and the
 * dummy descriptors have to sit in the same 4 GB.
 */
struct gem_desc
{
    ULONG               addrlo;
    ULONG               status;
    ULONG               addrhi;
    ULONG               unused;
};

#define GEM_RXD_ADDR_WRAP               (1 << 1)
#define GEM_RXD_ADDR_USED               (1 << 0)
#define GEM_RXD_EOF                     (1 << 15)
#define GEM_RXD_SOF                     (1 << 14)
#define GEM_RXD_LEN_MASK                0x1fff

#define GEM_TXD_USED                    (1UL << 31)
#define GEM_TXD_WRAP                    (1 << 30)
#define GEM_TXD_RLIMIT                  (1 << 29)
#define GEM_TXD_CORRUPT                 (1 << 27)
#define GEM_TXD_LCOLL                   (1 << 26)
#define GEM_TXD_LAST                    (1 << 15)
#define GEM_TXD_LEN_MASK                0x3fff

#define CDNSGEM_NDESC                   256

/* Cadence's PCI ID for the GEM; pcirp1.hidd presents the RP1 one so */
#define CDNSGEM_PCI_VENDOR              0x17CD
#define CDNSGEM_PCI_PRODUCT             0xE007

/* Standard MII registers */
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

#define GBCR_1000FD                     0x0200
#define GBSR_LP1000FD                   0x0800

/*
 * Broadcom BCM54xx vendor registers, for the BCM54213PE on the Pi 5. The
 * RGMII delays fall back to strap defaults on every BMCR_RESET, so they are
 * set again after each one. 0x18 selects a shadow in bits 2:0 on a write and
 * bits 14:12 on a read; 0x1c selects in bits 14:10, data in 9:0.
 */
#define MII_BCM54XX_AUX_CTL                 0x18
#define BCM54XX_AUXCTL_SHDWSEL_MASK         0x0007
#define BCM54XX_AUXCTL_SHDWSEL_MISC         0x0007
#define BCM54XX_AUXCTL_SHDWSEL_READ_SHIFT   12
#define BCM54XX_AUXCTL_MISC_WREN            0x8000
#define BCM54XX_AUXCTL_MISC_RGMII_SKEW_EN   0x0100

#define MII_BCM54XX_SHD                     0x1c
#define BCM54XX_SHD_WRITE                   0x8000
#define BCM54XX_SHD_SEL_SHIFT               10
#define BCM54XX_SHD_DATA_MASK               0x03ff
#define BCM54XX_SHD_CLK_CTL                 0x03
#define BCM54XX_SHD_CLK_CTL_GTXCLK_EN       0x0200

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

/* One frame per buffer. RX lands 2 bytes in, so the IP header is aligned. */
#define GEM_BUFSIZE                     2048
#define GEM_RX_OFFSET                   2

enum gem_phy_mode
{
    GEM_PHY_MODE_RGMII,
    GEM_PHY_MODE_RGMII_ID,
    GEM_PHY_MODE_RGMII_TXID,
    GEM_PHY_MODE_RGMII_RXID,
};

struct cdnsgem_hw
{
    IPTR                base;           /* register block, CPU address      */
    ULONG               phyAddr;        /* PHY address on the mdio bus      */
    enum gem_phy_mode   phyMode;
    ULONG               qmask;          /* queues present, bit 0 = queue 0  */
    ULONG               ntype1scr;
    ULONG               ntype2scr;
    ULONG               netctl;         /* what NETCTL was last given       */
    UBYTE               macAddr[ETH_ADDRESSSIZE];
    BOOL                haveMacAddr;
};

/* Deferred request queues, indices into cgu_RequestPorts */
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

/* cgu_Flags */
#define IFF_UP                          0x0001
#define IFF_CONFIGURED                  0x0002
#define IFF_PROMISC                     0x0004

/*
 * One direction. pidx/cidx run free and are reduced modulo CDNSGEM_NDESC;
 * desc points into the shared, uncached descriptor block.
 */
struct cdnsgem_ring
{
    volatile struct gem_desc *desc;
    APTR                bufmem;         /* one allocation, N * GEM_BUFSIZE  */
    ULONG               bufmemsize;
    UBYTE              *buf[CDNSGEM_NDESC];
    UQUAD               dma[CDNSGEM_NDESC]; /* buf[] as the GEM sees it     */
    ULONG               pidx;
    ULONG               cidx;

    struct IOSana2Req  *request[CDNSGEM_NDESC];
};

struct CDNSGEMUnit
{
    struct CDNSGEMBase *cgu_Base;
    struct cdnsgem_hw  *cgu_HW;

    struct SignalSemaphore cgu_Lock;
    struct MinList      cgu_Openers;
    struct MinList      cgu_MulticastRanges;
    struct MinList      cgu_TypeTrackers;
    LONG                cgu_RangeCount;
    ULONG               cgu_OpenCount;
    ULONG               cgu_Flags;

    struct MsgPort     *cgu_InputPort;
    struct MsgPort     *cgu_RequestPorts[REQUEST_QUEUE_COUNT];

    struct Interrupt    cgu_IRQHandler;
    BOOL                cgu_IRQAdded;

    UBYTE               cgu_DevAddr[ETH_ADDRESSSIZE];
    UBYTE               cgu_OrgAddr[ETH_ADDRESSSIZE];

    ULONG               cgu_SpeedMbps;
    BOOL                cgu_FullDuplex;
    BOOL                cgu_LinkUp;

    struct Sana2DeviceQuery cgu_Sana2Info;
    struct Sana2DeviceStats cgu_Stats;
    ULONG               cgu_SpecialStats[STAT_COUNT];

    /* TX ring, its dummy, RX ring, its dummy - one PCI allocation */
    APTR                cgu_DescMem;
    UQUAD               cgu_DescDMA;

    struct cdnsgem_ring cgu_TX;
    struct cdnsgem_ring cgu_RX;

    struct MsgPort     *cgu_TimerPort;
    struct timerequest *cgu_TimerReq;

    struct Task        *cgu_Task;
    struct Task        *cgu_DeathWatch;

    volatile ULONG      cgu_IRQPending;
    ULONG               cgu_IRQSignal;
};

#define CDNSGEM_DESCMEM_SIZE    (2 * (CDNSGEM_NDESC + 1) * sizeof(struct gem_desc))

struct CDNSGEMBase
{
    struct Device       cgm_Device;
    APTR                cgm_UtilityBase;
    APTR                cgm_OpenFirmwareBase;
    OOP_Object         *cgm_PCI;        /* held for as long as the device is */
    OOP_Object         *cgm_PCIDevice;
    OOP_Object         *cgm_PCIDriver;
    BOOL                cgm_Owned;
    struct cdnsgem_hw   cgm_HW;
    BOOL                cgm_Found;
    struct CDNSGEMUnit *cgm_Unit;
    struct SignalSemaphore cgm_UnitSem; /* serialises unit creation */
};

/* Pulls in genmodule's GM_UNIQUENAME / LIBBASE / LIBBASETYPEPTR macros.
 * Must follow the struct CDNSGEMBase definition (libbasetype). */
#include LC_LIBDEFS_FILE

/* cdnsgem_hw.c - register, mdio and PHY access */
ULONG CDNSGEM_Read(struct cdnsgem_hw *hw, ULONG reg);
void CDNSGEM_Write(struct cdnsgem_hw *hw, ULONG reg, ULONG val);
LONG CDNSGEM_MDIORead(struct cdnsgem_hw *hw, ULONG phy, ULONG reg);
BOOL CDNSGEM_MDIOWrite(struct cdnsgem_hw *hw, ULONG phy, ULONG reg, UWORD val);
void CDNSGEM_HWReset(struct cdnsgem_hw *hw);
void CDNSGEM_HWInit(struct cdnsgem_hw *hw);
void CDNSGEM_InitRings(struct CDNSGEMUnit *unit);
void CDNSGEM_SetMACAddress(struct cdnsgem_hw *hw, const UBYTE *addr);
BOOL CDNSGEM_GetMACAddress(struct cdnsgem_hw *hw, UBYTE *addr);
UBYTE CDNSGEM_HashMAC(const UBYTE *addr);
BOOL CDNSGEM_PHYInit(struct CDNSGEMUnit *unit);
BOOL CDNSGEM_PHYGetLink(struct cdnsgem_hw *hw, ULONG *mbps, BOOL *fullduplex);

/* cdnsgem_init.c - what pci.hidd hands us */
APTR CDNSGEM_AllocDMA(struct CDNSGEMBase *base, ULONG size);
void CDNSGEM_FreeDMA(struct CDNSGEMBase *base, APTR mem);
UQUAD CDNSGEM_BusAddress(struct CDNSGEMBase *base, APTR cpu);
BOOL CDNSGEM_AddInterrupt(struct CDNSGEMBase *base, struct Interrupt *irq);
void CDNSGEM_RemInterrupt(struct CDNSGEMBase *base, struct Interrupt *irq);

/* cdnsgem_unit.c */
struct CDNSGEMUnit *CDNSGEM_CreateUnit(struct CDNSGEMBase *base);
BOOL CDNSGEM_CheckLink(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit);
void CDNSGEM_DeleteUnit(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit);
void CDNSGEM_GoOnline(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit);
void CDNSGEM_GoOffline(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit);
void CDNSGEM_SetRXFilter(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit);
BOOL CDNSGEM_AddressFilter(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                           UBYTE *address);
BOOL CDNSGEM_SendPacket(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                        struct IOSana2Req *request);
void CDNSGEM_ReportEvents(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                          ULONG events);
struct TypeStats *CDNSGEM_FindTypeStats(struct MinList *list, ULONG packet_type);
BOOL CDNSGEM_AddMulticastRange(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                               const UBYTE *lower, const UBYTE *upper);
BOOL CDNSGEM_RemMulticastRange(struct CDNSGEMBase *base, struct CDNSGEMUnit *unit,
                               const UBYTE *lower, const UBYTE *upper);

/* cdnsgem_handler.c */
void CDNSGEM_HandleRequest(struct CDNSGEMBase *base, struct IOSana2Req *request);

#endif /* CDNSGEM_H */
