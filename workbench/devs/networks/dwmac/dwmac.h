/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DesignWare MAC (dwmac 4.x/5.x) SANA-II driver, private definitions.
*/

#ifndef DWMAC_H
#define DWMAC_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <aros/types/spinlock_s.h>
#include <proto/exec.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

/*
 * Two generations of this block share a name and almost nothing else.
 * Everything here is the 4.x/5.x register map, where the MAC, the queue
 * layer and the DMA engine sit in blocks of their own; a 3.x part puts
 * its DMA registers at 0x1000 and lays the rest out differently, so it
 * cannot be driven by this code without a second register map.
 */
#define DWMAC_MAC_BASE                  0x0000
#define DWMAC_MMC_BASE                  0x0700
#define DWMAC_MTL_BASE                  0x0c00
#define DWMAC_DMA_BASE                  0x1000

/* MAC block */
#define DWMAC_MAC_CONFIG                (DWMAC_MAC_BASE + 0x0000)
#define DWMAC_MAC_PKT_FILTER            (DWMAC_MAC_BASE + 0x0008)
#define DWMAC_MAC_RXQ_CTRL0             (DWMAC_MAC_BASE + 0x00a0)
#define DWMAC_MAC_INT_STATUS            (DWMAC_MAC_BASE + 0x00b0)
#define DWMAC_MAC_INT_ENABLE            (DWMAC_MAC_BASE + 0x00b4)
#define DWMAC_MAC_VERSION               (DWMAC_MAC_BASE + 0x0110)
#define DWMAC_MAC_HW_FEATURE1           (DWMAC_MAC_BASE + 0x0120)
#define DWMAC_MAC_MDIO_ADDR             (DWMAC_MAC_BASE + 0x0200)
#define DWMAC_MAC_MDIO_DATA             (DWMAC_MAC_BASE + 0x0204)
#define DWMAC_MAC_ADDR_HI(n)            (DWMAC_MAC_BASE + 0x0300 + ((n) * 8))
#define DWMAC_MAC_ADDR_LO(n)            (DWMAC_MAC_BASE + 0x0304 + ((n) * 8))

/*
 * The version register reports the core in the low byte and whatever
 * the integrator put there in the next one up. 0x51 is a 5.10a part.
 */
#define DWMAC_VERSION_CORE(x)           ((x) & 0xff)
#define DWMAC_VERSION_USER(x)           (((x) >> 8) & 0xff)

/* MAC_CONFIG */
#define DWMAC_CONFIG_RE                 (1 << 0)    /* receiver enable  */
#define DWMAC_CONFIG_TE                 (1 << 1)    /* transmitter on   */
#define DWMAC_CONFIG_DM                 (1 << 13)   /* full duplex      */
#define DWMAC_CONFIG_FES                (1 << 14)   /* 100Mbit          */
#define DWMAC_CONFIG_PS                 (1 << 15)   /* port select: MII */
#define DWMAC_CONFIG_JE                 (1 << 16)   /* jumbo enable     */
#define DWMAC_CONFIG_JD                 (1 << 17)   /* jabber disable   */
#define DWMAC_CONFIG_WD                 (1 << 19)   /* watchdog disable */
#define DWMAC_CONFIG_ACS                (1 << 20)   /* strip pad/fcs    */
#define DWMAC_CONFIG_CST                (1 << 21)   /* strip fcs (type) */

/* MAC_PKT_FILTER */
#define DWMAC_FILTER_PR                 (1 << 0)    /* promiscuous       */
#define DWMAC_FILTER_PM                 (1 << 4)    /* all multicast     */

/* MAC_RXQ_CTRL0: queue 0 routed as a plain (DCB) queue */
#define DWMAC_RXQ0_ENABLE_DCB           (2 << 0)

/* HW_FEATURE1 carries the fifo sizes, encoded as log2(bytes / 128) */
#define DWMAC_HWFEAT1_RXFIFO(x)         ((x) & 0x1f)
#define DWMAC_HWFEAT1_TXFIFO(x)         (((x) >> 6) & 0x1f)

/* MDIO_ADDR */
#define DWMAC_MDIO_BUSY                 (1 << 0)
#define DWMAC_MDIO_C45E                 (1 << 1)
#define DWMAC_MDIO_GOC_SHIFT            2           /* operation command */
#define DWMAC_MDIO_GOC_WRITE            (1 << DWMAC_MDIO_GOC_SHIFT)
#define DWMAC_MDIO_GOC_READ             (3 << DWMAC_MDIO_GOC_SHIFT)
#define DWMAC_MDIO_CR_SHIFT             8           /* csr clock range   */
#define DWMAC_MDIO_REG_SHIFT            16
#define DWMAC_MDIO_ADDR_SHIFT           21

#define DWMAC_MDIO_DATA_MASK            0xffff

/* How long to wait on the MDIO bus before calling it dead, in microseconds */
#define DWMAC_MDIO_TIMEOUT              100000

/* MMC counter blocks - only ever masked off here */
#define DWMAC_MMC_RX_INT_MASK           (DWMAC_MMC_BASE + 0x000c)
#define DWMAC_MMC_TX_INT_MASK           (DWMAC_MMC_BASE + 0x0010)
#define DWMAC_MMC_RX_IPC_INT_MASK       (DWMAC_MMC_BASE + 0x0100)

/* MTL queue 0 */
#define DWMAC_MTL_TXQ0_OPMODE           (DWMAC_MTL_BASE + 0x0100)
#define DWMAC_MTL_RXQ0_OPMODE           (DWMAC_MTL_BASE + 0x0130)

#define DWMAC_MTL_TXQ_FTQ               (1 << 0)    /* flush queue        */
#define DWMAC_MTL_TXQ_TSF               (1 << 1)    /* store and forward  */
#define DWMAC_MTL_TXQ_EN                (2 << 2)    /* queue enabled      */
#define DWMAC_MTL_TXQ_TQS_SHIFT         16          /* size in 256b units */

#define DWMAC_MTL_RXQ_FUP               (1 << 3)
#define DWMAC_MTL_RXQ_FEP               (1 << 4)
#define DWMAC_MTL_RXQ_RSF               (1 << 5)    /* store and forward  */
#define DWMAC_MTL_RXQ_RQS_SHIFT         20

/* DMA block */
#define DWMAC_DMA_MODE                  (DWMAC_DMA_BASE + 0x0000)
#define DWMAC_DMA_SYSBUS_MODE           (DWMAC_DMA_BASE + 0x0004)

#define DWMAC_DMA_MODE_SWR              (1 << 0)    /* software reset */

#define DWMAC_DMA_SYSBUS_BLEN4          (1 << 1)
#define DWMAC_DMA_SYSBUS_BLEN8          (1 << 2)
#define DWMAC_DMA_SYSBUS_BLEN16         (1 << 3)
#define DWMAC_DMA_SYSBUS_EAME           (1 << 11)   /* 64-bit addressing */
#define DWMAC_DMA_SYSBUS_AAL            (1 << 12)

/* DMA channel 0 - the only one used */
#define DWMAC_DMA_CH0_BASE              (DWMAC_DMA_BASE + 0x0100)
#define DWMAC_DMA_CH0_CONTROL           (DWMAC_DMA_CH0_BASE + 0x00)
#define DWMAC_DMA_CH0_DSL_SHIFT         18          /* descriptor skip length, in bus words */
#define DWMAC_DMA_CH0_TX_CONTROL        (DWMAC_DMA_CH0_BASE + 0x04)
#define DWMAC_DMA_CH0_RX_CONTROL        (DWMAC_DMA_CH0_BASE + 0x08)
#define DWMAC_DMA_CH0_TXDESC_HI         (DWMAC_DMA_CH0_BASE + 0x10)
#define DWMAC_DMA_CH0_TXDESC_LO         (DWMAC_DMA_CH0_BASE + 0x14)
#define DWMAC_DMA_CH0_RXDESC_HI         (DWMAC_DMA_CH0_BASE + 0x18)
#define DWMAC_DMA_CH0_RXDESC_LO         (DWMAC_DMA_CH0_BASE + 0x1c)
#define DWMAC_DMA_CH0_TXDESC_TAIL       (DWMAC_DMA_CH0_BASE + 0x20)
#define DWMAC_DMA_CH0_RXDESC_TAIL       (DWMAC_DMA_CH0_BASE + 0x28)
#define DWMAC_DMA_CH0_TXDESC_RINGLEN    (DWMAC_DMA_CH0_BASE + 0x2c)
#define DWMAC_DMA_CH0_RXDESC_RINGLEN    (DWMAC_DMA_CH0_BASE + 0x30)
#define DWMAC_DMA_CH0_INT_ENABLE        (DWMAC_DMA_CH0_BASE + 0x34)
#define DWMAC_DMA_CH0_RX_WATCHDOG       (DWMAC_DMA_CH0_BASE + 0x38)
#define DWMAC_DMA_CH0_STATUS            (DWMAC_DMA_CH0_BASE + 0x60)

#define DWMAC_DMA_TX_ST                 (1 << 0)    /* start transmit */
#define DWMAC_DMA_TX_OSF                (1 << 4)
#define DWMAC_DMA_TX_PBL_SHIFT          16

#define DWMAC_DMA_RX_SR                 (1 << 0)    /* start receive  */
#define DWMAC_DMA_RX_RBSZ_SHIFT         1
#define DWMAC_DMA_RX_PBL_SHIFT          16

#define DWMAC_DMA_INT_TIE               (1 << 0)
#define DWMAC_DMA_INT_RIE               (1 << 6)
#define DWMAC_DMA_INT_RBUE              (1 << 7)
#define DWMAC_DMA_INT_FBEE              (1 << 12)
#define DWMAC_DMA_INT_AIE               (1 << 14)
#define DWMAC_DMA_INT_NIE               (1 << 15)

#define DWMAC_DMA_STAT_TI               (1 << 0)
#define DWMAC_DMA_STAT_TPS              (1 << 1)
#define DWMAC_DMA_STAT_TBU              (1 << 2)
#define DWMAC_DMA_STAT_RI               (1 << 6)
#define DWMAC_DMA_STAT_RBU              (1 << 7)
#define DWMAC_DMA_STAT_RPS              (1 << 8)
#define DWMAC_DMA_STAT_FBE              (1 << 12)
#define DWMAC_DMA_STAT_AIS              (1 << 14)
#define DWMAC_DMA_STAT_NIS              (1 << 15)

/*
 * One descriptor, both rings, both views: software prepares the "read"
 * layout, the engine writes status back over it when it is done.
 */
struct dwmac_desc
{
    ULONG               des0;
    ULONG               des1;
    ULONG               des2;
    ULONG               des3;
};

/*
 * The engine's DMA is not coherent with the CPU caches on the boards this
 * runs on (the Titan's tree gives dma-coherent to its PCIe roots and not
 * to the MAC), so every descriptor gets a cache line of its own: the CPU
 * cleans a line after writing it and invalidates one before reading what
 * the engine wrote back, and with four 16-byte descriptors per line those
 * operations would clobber the neighbours (a clean writes a stale OWN
 * back over an engine's completion - seen as a ring "full" of descriptors
 * the engine had long finished, TX dead). The engine steps 64 bytes too,
 * through the channel's descriptor skip length (DSL, counted in bus
 * words, so the bus width is probed at start-up: dwu_DSL).
 */
#define DWMAC_DESC_STRIDE               64
#define DWMAC_TXD(unit, i)  ((volatile struct dwmac_desc *)((UBYTE *)(unit)->dwu_TXDesc + (i) * DWMAC_DESC_STRIDE))
#define DWMAC_RXD(unit, i)  ((volatile struct dwmac_desc *)((UBYTE *)(unit)->dwu_RXDesc + (i) * DWMAC_DESC_STRIDE))

/* Transmit, as prepared */
#define DWMAC_TDES2_IOC                 (1UL << 31)
#define DWMAC_TDES2_B1L_MASK            0x3fff
#define DWMAC_TDES3_OWN                 (1UL << 31)
#define DWMAC_TDES3_FD                  (1UL << 29)
#define DWMAC_TDES3_LD                  (1UL << 28)
#define DWMAC_TDES3_LEN_MASK            0x7fff
/* Transmit, as written back */
#define DWMAC_TDES3_ES                  (1UL << 15)

/* Receive, as prepared */
#define DWMAC_RDES3_OWN                 (1UL << 31)
#define DWMAC_RDES3_IOC                 (1UL << 30)
#define DWMAC_RDES3_BUF1V               (1UL << 24)
/* Receive, as written back */
#define DWMAC_RDES3_FD                  (1UL << 29)
#define DWMAC_RDES3_LD                  (1UL << 28)
#define DWMAC_RDES3_ES                  (1UL << 15)
#define DWMAC_RDES3_LEN_MASK            0x7fff

/* Standard mdio registers, the ones every PHY has to answer */
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
#define BMSR_ESTATEN                    0x0100

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
#define ETH_CRCSIZE                     4

/*
 * Jumbo ceiling.  The MAC's Jumbo-Enable extends the giant-packet limit to
 * 9018 bytes on the wire, so 9000 is the largest MTU we advertise.  The
 * effective maximum on a given SoC is also bounded by the MTL FIFO depth in
 * store-and-forward mode; that is not probed here.
 */
#define DWMAC_MAX_MTU                   9000
#define DWMAC_ENV_MTU_PATH              "SYS/Net/dwmac/unit0/MTU"

struct eth_frame
{
    UBYTE               eth_packet_dest[ETH_ADDRESSSIZE];
    UBYTE               eth_packet_source[ETH_ADDRESSSIZE];
    UWORD               eth_packet_type;
    UBYTE               eth_packet_data[];
};

/* Ring geometry. A buffer holds one whole frame; no chaining. */
#define DWMAC_TXDESC                    32
#define DWMAC_RXDESC                    32
#define DWMAC_BUFSIZE                   1536

/* What the device tree told us about one controller */
struct dwmac_hw
{
    IPTR                base;           /* register block, as mapped   */
    IPTR                phys;           /* and where it really lives   */
    IPTR                size;
    ULONG               irq;            /* source at the interrupt controller */
    ULONG               phyAddr;        /* address of the PHY on the mdio bus */
    ULONG               csrClock;       /* feeds the mdio divider, in Hz */
    UBYTE               macAddr[6];
    BOOL                haveMacAddr;
};

/* Deferred request queues, indices into dwu_RequestPorts */
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

/* dwu_Flags */
#define IFF_UP                          0x0001
#define IFF_CONFIGURED                  0x0002
#define IFF_PROMISC                     0x0004

struct DWMACUnit
{
    struct DWMACBase   *dwu_Base;
    struct dwmac_hw    *dwu_HW;

    struct SignalSemaphore dwu_Lock;
    struct MinList      dwu_Openers;
    struct MinList      dwu_MulticastRanges;
    struct MinList      dwu_TypeTrackers;
    LONG                dwu_RangeCount;
    ULONG               dwu_OpenCount;
    ULONG               dwu_Flags;

    struct MsgPort     *dwu_InputPort;
    struct MsgPort     *dwu_RequestPorts[REQUEST_QUEUE_COUNT];
    struct Task        *dwu_Task;
    struct Task        *dwu_DeathWatch;

    struct Interrupt    dwu_IRQHandler;
    struct Interrupt    dwu_TXInt;
    BOOL                dwu_IRQAdded;

    UBYTE               dwu_DevAddr[ETH_ADDRESSSIZE];
    UBYTE               dwu_OrgAddr[ETH_ADDRESSSIZE];

    ULONG               dwu_SpeedMbps;
    BOOL                dwu_FullDuplex;
    BOOL                dwu_LinkUp;

    struct Sana2DeviceQuery dwu_Sana2Info;
    struct Sana2DeviceStats dwu_Stats;
    ULONG               dwu_SpecialStats[STAT_COUNT];

    /* Frame geometry, from ENV:SYS/Net/dwmac/unit0/MTU at unit creation */
    ULONG               dwu_MTU;        /* configured MTU              */
    ULONG               dwu_FrameMax;   /* MTU + Ethernet header + FCS */
    ULONG               dwu_BufSize;    /* per-slot ring buffer size   */

    /* Rings and their buffers, one contiguous allocation each */
    APTR                dwu_DescMem;
    ULONG               dwu_DescMemSize;
    struct dwmac_desc  *dwu_TXDesc;     /* DWMAC_DESC_STRIDE apart, see DWMAC_TXD() */
    struct dwmac_desc  *dwu_RXDesc;
    ULONG               dwu_DSL;        /* descriptor skip length in use (0: probing failed) */
    APTR                dwu_BufMem;
    ULONG               dwu_BufMemSize;
    UBYTE              *dwu_TXBuf;
    UBYTE              *dwu_RXBuf;

    /*
     * The transmit ring is filled at the head (only ever from the tx
     * soft interrupt) and reclaimed at the tail (only ever from the
     * hardware interrupt), with one slot always left empty so the two
     * indices meeting means empty rather than full.
     */
    ULONG               dwu_TXHead;
    ULONG               dwu_TXTail;
    spinlock_t          dwu_ListLock;       /* dwu_Openers and dwu_TypeTrackers */
    ULONG               dwu_RXCurrent;
};

struct DWMACBase
{
    struct Device       dwm_Device;
    APTR                dwm_KernelBase;
    APTR                dwm_UtilityBase;
    APTR                dwm_ExecLockBase;   /* execlock.resource, for the port and list locks */
    struct dwmac_hw     dwm_HW;
    BOOL                dwm_Found;
    struct DWMACUnit   *dwm_Unit;
    struct SignalSemaphore dwm_UnitSem; /* serialises unit creation */
};

/* dwmac_dt.c - everything that knows how the platform describes hardware */
BOOL DWMAC_Discover(struct DWMACBase *base, struct dwmac_hw *hw);

/* dwmac_hw.c */
ULONG DWMAC_Read(struct dwmac_hw *hw, ULONG reg);
void DWMAC_Write(struct dwmac_hw *hw, ULONG reg, ULONG val);
LONG DWMAC_MDIORead(struct dwmac_hw *hw, ULONG phy, ULONG reg);
BOOL DWMAC_MDIOWrite(struct dwmac_hw *hw, ULONG phy, ULONG reg, UWORD val);
BOOL DWMAC_HWInit(struct dwmac_hw *hw);
void DWMAC_SetMACAddress(struct dwmac_hw *hw, const UBYTE *addr);
BOOL DWMAC_GetMACAddress(struct dwmac_hw *hw, UBYTE *addr);
BOOL DWMAC_PHYInit(struct dwmac_hw *hw);
BOOL DWMAC_PHYGetLink(struct dwmac_hw *hw, ULONG *mbps, BOOL *fullduplex);

/* dwmac_unit.c */
struct DWMACUnit *DWMAC_CreateUnit(struct DWMACBase *base);
void DWMAC_DeleteUnit(struct DWMACBase *base, struct DWMACUnit *unit);
void DWMAC_FlushUnit(struct DWMACBase *base, struct DWMACUnit *unit,
                     UBYTE last_queue, BYTE error);
void DWMAC_GoOnline(struct DWMACBase *base, struct DWMACUnit *unit);
void DWMAC_GoOffline(struct DWMACBase *base, struct DWMACUnit *unit);
BOOL DWMAC_AddressFilter(struct DWMACBase *base, struct DWMACUnit *unit,
                         UBYTE *address);
void DWMAC_CopyPacket(struct DWMACBase *base, struct DWMACUnit *unit,
                      struct IOSana2Req *request, ULONG packet_size,
                      UWORD packet_type, struct eth_frame *frame);
void DWMAC_ReportEvents(struct DWMACBase *base, struct DWMACUnit *unit,
                        ULONG events);
struct TypeStats *DWMAC_FindTypeStats(struct MinList *list, ULONG packet_type);
BOOL DWMAC_AddMulticastRange(struct DWMACBase *base, struct DWMACUnit *unit,
                             const UBYTE *lower, const UBYTE *upper);
BOOL DWMAC_RemMulticastRange(struct DWMACBase *base, struct DWMACUnit *unit,
                             const UBYTE *lower, const UBYTE *upper);

/* dwmac_handler.c */
void DWMAC_HandleRequest(struct DWMACBase *base, struct IOSana2Req *request);


/*
 * The interrupt and the soft interrupt run on whichever CPU takes them;
 * requests are queued and aborted from any other. Disable() only stops
 * this CPU, so every list they share is guarded by a spinlock as well:
 * the MsgPort's own (the one exec's PutMsg/GetMsg take) for the request
 * queues, dwu_ListLock for the openers and type trackers. Disable()
 * stays, so this CPU's interrupt cannot spin on a lock its task holds.
 */
static inline void DWMAC_SpinLock(struct DWMACBase *base, spinlock_t *lock)
{
    Disable();
#if defined(__AROSEXEC_SMP__)
    if (base->dwm_ExecLockBase)
    {
        APTR ExecLockBase = base->dwm_ExecLockBase;
        ObtainLock(lock, SPINLOCK_MODE_WRITE, 0);
    }
#else
    (void)base; (void)lock;
#endif
}

static inline void DWMAC_SpinUnlock(struct DWMACBase *base, spinlock_t *lock)
{
#if defined(__AROSEXEC_SMP__)
    if (base->dwm_ExecLockBase)
    {
        APTR ExecLockBase = base->dwm_ExecLockBase;
        ReleaseLock(lock, 0);
    }
#else
    (void)base; (void)lock;
#endif
    Enable();
}

#define DWMAC_PORT_LOCK(base, port)     DWMAC_SpinLock((base), &(port)->mp_SpinLock)
#define DWMAC_PORT_UNLOCK(base, port)   DWMAC_SpinUnlock((base), &(port)->mp_SpinLock)
#define DWMAC_LIST_LOCK(base, unit)     DWMAC_SpinLock((base), &(unit)->dwu_ListLock)
#define DWMAC_LIST_UNLOCK(base, unit)   DWMAC_SpinUnlock((base), &(unit)->dwu_ListLock)

/* The queue a request sits on while it waits */
static inline struct MsgPort *DWMAC_QueuePort(struct DWMACUnit *unit, struct IOSana2Req *req)
{
    struct Opener *opener = req->ios2_BufferManagement;

    switch (req->ios2_Req.io_Command)
    {
    case CMD_READ:          return opener ? &opener->read_port : NULL;
    case CMD_WRITE:
    case S2_BROADCAST:
    case S2_MULTICAST:      return unit->dwu_RequestPorts[WRITE_QUEUE];
    case S2_ONEVENT:        return unit->dwu_RequestPorts[EVENT_QUEUE];
    case S2_READORPHAN:     return unit->dwu_RequestPorts[ADOPT_QUEUE];
    default:                return unit->dwu_InputPort;
    }
}

#endif /* DWMAC_H */
