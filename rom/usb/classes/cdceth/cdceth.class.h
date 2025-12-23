#ifndef CDCETH_CLASS_H
#define CDCETH_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for cdceth class
 *----------------------------------------------------------------------------
 *                   Copyright (C) 2025, The AROS Development Team.
 */

#include "common.h"

#include <devices/newstyle.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <exec/devices.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "cdceth.h"
#include "dev.h"

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define DDF_CONFIGURED (1<<2)  /* station address is configured */
#define DDF_ONLINE     (1<<3)  /* device is online */
#define DDF_OFFLINE    (1<<4)  /* device was put offline */

#define DROPPED        (1<<0)  /* Did the packet get dropped? */
#define PACKETFILTER   (1<<1)  /* Use the packet filter? */

#define ETHER_MIN_LEN  60           /* smallest amount that nic will accept */
#define ETHER_MAX_LEN  2048         /* largest legal amount for Ethernet */

/* Ethernet packet data sizes (maximum) */
#define ETHERPKT_SIZE  1500
#define RAWPKT_SIZE    1514

/* CDC Ethernet control requests */
#define UCDC_SET_ETHERNET_PACKET_FILTER 0x43

/* CDC Ethernet packet filter bits (USB CDC 1.2 spec 6.2.30) */
#define USB_CDC_PACKET_TYPE_PROMISCUOUS 0x0001
#define USB_CDC_PACKET_TYPE_ALLMULTI    0x0002
#define USB_CDC_PACKET_TYPE_DIRECTED    0x0004
#define USB_CDC_PACKET_TYPE_BROADCAST   0x0008
#define USB_CDC_PACKET_TYPE_MULTICAST   0x0010

/* Packet status flags parsed from RX status byte */
#define PKSF_UNDERRUN         0x01
#define PKSF_LENGTH_ERROR     0x02
#define PKSF_ALIGNMENT_ERROR  0x04
#define PKSF_CRC_ERROR        0x08
#define PKSF_OVERRUN          0x10
#define PKSF_NO_ERROR         0x20

/* PHY stuff */

/* Generic MII registers. */

#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_BMSR            0x01        /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_CTRL1000        0x09        /* 1000BASE-T control          */
#define MII_STAT1000        0x0a        /* 1000BASE-T status           */
#define MII_ESTATUS         0x0f        /* Extended Status */
#define MDIO_AN_10GBT_CTRL  0x20        /* 10GBASE-T AN control        */
#define MDIO_AN_10GBT_STAT  0x21        /* 10GBASE-T AN status         */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000          0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x00c0  /* Unused...                   */
#define BMSR_ESTATEN            0x0100  /* Extended Status in R15 */
#define BMSR_100FULL2           0x0200  /* Can do 100BASE-T2 HDX */
#define BMSR_100HALF2           0x0400  /* Can do 100BASE-T2 FDX */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

#define BMSR_MEDIA (BMSR_10HALF|BMSR_10FULL|BMSR_100HALF|BMSR_100FULL|BMSR_ANEGCAPABLE)

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

/* 1000/2500BASE-T advertisement/control */
#define ADVERTISE_1000HALF      0x0100
#define ADVERTISE_1000FULL      0x0200
#define ADVERTISE_2500FULL      0x0400
/* Link partner ability for 1000/2500BASE-T */
#define LPA_1000HALF            0x0400
#define LPA_1000FULL            0x0800
#define LPA_2500FULL            0x1000

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
                        ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab

#define MT_AUTO                 0x0000
#define MT_10BASE_T_HALF_DUP    0x0001
#define MT_10BASE_T_FULL_DUP    0x0002
#define MT_100BASE_TX_HALF_DUP  0x0003
#define MT_100BASE_TX_FULL_DUP  0x0004
#define MT_1000BASE_T_HALF_DUP  0x0005
#define MT_1000BASE_T_FULL_DUP  0x0006
#define MT_2500BASE_T_FULL_DUP  0x0007

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_DefaultUnit;
    ULONG cdc_MediaType;
};

#if defined(__GNUC__)
# pragma pack()
#endif

/* Structure of an ethernet packet - internal */

struct EtherPacketHeader
{
    UBYTE       eph_Dest[ETHER_ADDR_SIZE]; /* 0 destination address */
    UBYTE       eph_Src[ETHER_ADDR_SIZE]; /* 6 originator  address */
    UWORD       eph_Type;                 /* 12 packet type */
};

/* Buffer management node - private */
struct BufMan
{
    struct Node bm_Node;
    APTR        bm_DMACopyFromBuf32;
    APTR        bm_CopyFromBuf;
    APTR        bm_DMACopyToBuf32;
    APTR        bm_CopyToBuf;
    APTR        bm_PacketFilter;
    struct List bm_RXQueue;               /* read requests */
};

/* Multicast address range record - private */
struct MulticastAddressRange
{
    struct Node mar_Node;                 /* 0 list node */
    ULONG       mar_UseCount;             /* 8 number of times used */
    UBYTE       mar_LowerAddr[ETHER_ADDR_SIZE]; /* 12 multicast address lower bound */
    UBYTE       mar_UpperAddr[ETHER_ADDR_SIZE]; /* 18 multicast address upper bound */
};

struct PacketTypeStats
{
    struct Node pts_Node;
    ULONG       pts_PacketType;
    struct Sana2PacketTypeStats pts_Stats;
};


struct NepEthDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepEthBase  *np_ClsBase;       /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassEth
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    ULONG               ncp_OpenFlags;    /* Flags used to open the device */
    struct NepEthBase  *ncp_ClsBase;      /* Up linkage */
    struct NepEthDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct PsdInterface *ncp_ControlInterface; /* CDC control interface */
    BOOL                ncp_FilterTried;  /* Packet filter request attempted */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* Endpoint 1 */
    struct PsdPipe     *ncp_EPOutPipe;    /* Endpoint 1 pipe */
    IPTR                ncp_EPOutMaxPktSize; /* Endpoint 1 max pkt size */
    struct PsdEndpoint *ncp_EPIn;         /* Endpoint 2 */
    struct PsdPipe     *ncp_EPInPipe;     /* Endpoint 2 pipe */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    //BOOL                ncp_DenyRequests; /* Do not accept further IO requests */

    struct List         ncp_BufManList;   /* Buffer Managers */
    struct List         ncp_EventList;    /* List for DoEvent */
    struct List         ncp_TrackList;    /* List of trackables */
    struct List         ncp_Multicasts;   /* List of multicast addresses */
    UBYTE               ncp_MacAddress[ETHER_ADDR_SIZE]; /* Current Mac Address */
    UBYTE               ncp_ROMAddress[ETHER_ADDR_SIZE]; /* ROM Mac Address */
    ULONG               ncp_PhyID;        /* ID of the PHY */
    UWORD               ncp_PhyMode;

    UBYTE               ncp_MulticastArray[8]; /* array for the multicast hashes */
    ULONG               ncp_StateFlags;   /* State of the unit */

    ULONG               ncp_Retries;      /* tx collision count */
    ULONG               ncp_BadMulticasts; /* bad multicast count */

    UBYTE              *ncp_ReadBuffer[2]; /* Packet Double Buffered Read Buffer */
    UBYTE              *ncp_WriteBuffer[2]; /* Packet Write Buffer */

    UWORD               ncp_ReadBufNum;   /* Next Read Buffer to use */
    UWORD               ncp_WriteBufNum;  /* Next Write Buffer to use */

    struct Sana2DeviceStats ncp_DeviceStats; /* SANA Stats */
    struct Sana2PacketTypeStats *ncp_TypeStats2048; /* IP protocol stats ptr, or NULL */
    struct Sana2PacketTypeStats *ncp_TypeStats2054; /* ARP protocol stats ptr, or NULL */

    UBYTE              *ncp_ReadPending;  /* read IORequest pending */
    struct IOSana2Req  *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_OrphanQueue;  /* List of orphan read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */

    UBYTE               ncp_DevIDString[128];  /* Device ID String */

    BOOL                ncp_UsingDefaultCfg;
    struct ClsDevCfg   *ncp_CDC;

    struct Library     *ncp_MUIBase;      /* MUI master base */
    struct Library     *ncp_PsdBase;      /* Poseidon base */
    struct Library     *ncp_IntBase;      /* Intuition base */
    struct Task        *ncp_GUITask;      /* GUI Task */
    struct NepClassHid *ncp_GUIBinding;   /* Window of binding that's open */

    Object             *ncp_App;
    Object             *ncp_MainWindow;

    Object             *ncp_UnitObj;
    Object             *ncp_MediaTypeObj;

    Object             *ncp_UseObj;
    Object             *ncp_SetDefaultObj;
    Object             *ncp_CloseObj;

    Object             *ncp_AboutMI;
    Object             *ncp_UseMI;
    Object             *ncp_SetDefaultMI;
    Object             *ncp_MUIPrefsMI;

};

struct NepEthBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepEthDevBase *nh_DevBase;     /* base of device created */
    struct List         nh_Units;         /* List of units available */

    struct NepClassEth  nh_DummyNCP;      /* Dummy ncp for default config */
};

/* Protos */

struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
struct NepClassEth * usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp);

struct NepClassEth * nAllocEth(void);
void nFreeEth(struct NepClassEth *ncp);

BOOL nInitCDC(struct NepClassEth *ncp);
void nSetOnline(struct NepClassEth *ncp);
void nUpdateRXMode(struct NepClassEth *ncp);

void nDoEvent(struct NepClassEth *ncp, ULONG events);
BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq);
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG len);

BOOL nLoadClassConfig(struct NepEthBase *nh);
BOOL nLoadBindingConfig(struct NepClassEth *ncp);
LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp);

void nGUITaskCleanup(struct NepClassEth *nh);

AROS_UFP0(void, nEthTask);
AROS_UFP0(void, nGUITask);

#endif /* CDCETH_CLASS_H */
