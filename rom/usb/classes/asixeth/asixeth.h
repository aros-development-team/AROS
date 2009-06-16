#ifndef ASIXETH_H
#define ASIXETH_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <exec/devices.h>

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define PF_AX88178      0x0001
#define PF_AX88772      0x0002
#define PF_DLINKGPIO    0x0010 // 0x9f9d9f
#define PF_HAWKINGGPIO  0x0020 // 0x1f1d1f

#define DDF_CONFIGURED (1<<2)  /* station address is configured */
#define DDF_ONLINE     (1<<3)  /* device is online */
#define DDF_OFFLINE    (1<<4)  /* device was put offline */

#define DROPPED        (1<<0)  /* Did the packet get dropped? */
#define PACKETFILTER   (1<<1)  /* Use the packet filter? */

/* Ethernet address bytesize
*/
#define ETHER_ADDR_SIZE 6

#define ETHER_MIN_LEN  60           /* smallest amount that nic will accept */
#define ETHER_MAX_LEN  2048         /* largest legal amount for Ethernet */

/* Ethernet packet data sizes (maximum)
*/
#define ETHERPKT_SIZE  1500
#define RAWPKT_SIZE    1514

/* ASIX AX8817X based USB 2.0 Ethernet Devices */

#define UAXR_SET_SW_MII               0x06
#define UAXR_READ_MII_REG             0x07
#define UAXR_WRITE_MII_REG            0x08
#define UAXR_SET_HW_MII               0x0a
#define UAXR_READ_EEPROM              0x0b
#define UAXR_WRITE_EEPROM             0x0c
#define UAXR_WRITE_ENABLE             0x0d
#define UAXR_WRITE_DISABLE            0x0e
#define UAXR_READ_RX_CTL              0x0f
#define UAXR_WRITE_RX_CTL             0x10
#define UAXR_READ_IPG012              0x11
#define UAXR_WRITE_IPG0               0x12
#define UAXR_WRITE_IPG1               0x13
#define UAXR_READ_NODE_ID             0x13
#define UAXR_WRITE_IPG2               0x14
#define UAXR_WRITE_MULTI_FILTER       0x16
#define UAXR_READ_NODE_ID_2           0x17
#define UAXR_READ_PHY_ID              0x19
#define UAXR_READ_MEDIUM_STATUS       0x1a
#define UAXR_WRITE_MEDIUM_MODE        0x1b
#define UAXR_READ_MONITOR_MODE        0x1c
#define UAXR_WRITE_MONITOR_MODE       0x1d
#define UAXR_READ_GPIOS               0x1e
#define UAXR_WRITE_GPIOS              0x1f
#define UAXR_SW_RESET                 0x20
#define UAXR_SW_PHY_STATUS            0x21
#define UAXR_SW_PHY_SELECT            0x22

#define AX_MONITOR_MODE                 0x01
#define AX_MONITOR_LINK                 0x02
#define AX_MONITOR_MAGIC                0x04
#define AX_MONITOR_HSFS                 0x10

/* AX88172 Medium Status Register values */
#define AX88172_MEDIUM_FD               0x02
#define AX88172_MEDIUM_TX               0x04
#define AX88172_MEDIUM_FC               0x10
#define AX88172_MEDIUM_DEFAULT  (AX88172_MEDIUM_FD|AX88172_MEDIUM_TX|AX88172_MEDIUM_FC)

#define AX_MCAST_FILTER_SIZE            8
#define AX_MAX_MCAST                    64

#define AX_SWRESET_CLEAR                0x00
#define AX_SWRESET_RR                   0x01
#define AX_SWRESET_RT                   0x02
#define AX_SWRESET_PRTE                 0x04
#define AX_SWRESET_PRL                  0x08
#define AX_SWRESET_BZ                   0x10
#define AX_SWRESET_IPRL                 0x20
#define AX_SWRESET_IPPD                 0x40

#define AX88772_IPG0_DEFAULT            0x15
#define AX88772_IPG1_DEFAULT            0x0c
#define AX88772_IPG2_DEFAULT            0x12

/* AX88772 & AX88178 Medium Mode Register */
#define AX_MEDIUM_PF            0x0080
#define AX_MEDIUM_JFE           0x0040
#define AX_MEDIUM_TFC           0x0020
#define AX_MEDIUM_RFC           0x0010
#define AX_MEDIUM_ENCK          0x0008
#define AX_MEDIUM_AC            0x0004
#define AX_MEDIUM_FD            0x0002
#define AX_MEDIUM_GM            0x0001
#define AX_MEDIUM_SM            0x1000
#define AX_MEDIUM_SBP           0x0800
#define AX_MEDIUM_PS            0x0200
#define AX_MEDIUM_RE            0x0100

#define AX88178_MEDIUM_DEFAULT  (AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE|AX_MEDIUM_JFE)

#define AX88772_MEDIUM_DEFAULT  (AX_MEDIUM_PS|AX_MEDIUM_FD|AX_MEDIUM_AC|AX_MEDIUM_RFC|AX_MEDIUM_TFC|AX_MEDIUM_RE)

/* AX88772 & AX88178 RX_CTL values */
#define AX_RX_CTL_SO                    0x0080
#define AX_RX_CTL_AP                    0x0020
#define AX_RX_CTL_AM                    0x0010
#define AX_RX_CTL_AB                    0x0008
#define AX_RX_CTL_SEP                   0x0004
#define AX_RX_CTL_AMALL                 0x0002
#define AX_RX_CTL_PRO                   0x0001
#define AX_RX_CTL_MFB_2048              0x0000
#define AX_RX_CTL_MFB_4096              0x0100
#define AX_RX_CTL_MFB_8192              0x0200
#define AX_RX_CTL_MFB_16384             0x0300

#define AX_DEFAULT_RX_CTL       (AX_RX_CTL_SO|AX_RX_CTL_AB)

/* GPIO 0 .. 2 toggles */
#define AX_GPIO_GPO0EN          0x01    /* GPIO0 Output enable */
#define AX_GPIO_GPO_0           0x02    /* GPIO0 Output value */
#define AX_GPIO_GPO1EN          0x04    /* GPIO1 Output enable */
#define AX_GPIO_GPO_1           0x08    /* GPIO1 Output value */
#define AX_GPIO_GPO2EN          0x10    /* GPIO2 Output enable */
#define AX_GPIO_GPO_2           0x20    /* GPIO2 Output value */
#define AX_GPIO_RESERVED        0x40    /* Reserved */
#define AX_GPIO_RSE             0x80    /* Reload serial EEPROM */

#define AX_EEPROM_MAGIC         0xdeadbeef
#define AX88172_EEPROM_LEN      0x40
#define AX88772_EEPROM_LEN      0xff

#define PHY_MODE_MARVELL        0x0000
#define MII_MARVELL_LED_CTRL    0x0018
#define MII_MARVELL_STATUS      0x001b
#define MII_MARVELL_CTRL        0x0014

#define MARVELL_LED_MANUAL      0x0019

#define MARVELL_STATUS_HWCFG    0x0004

#define MARVELL_CTRL_TXDELAY    0x0002
#define MARVELL_CTRL_RXDELAY    0x0080

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
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */

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
#define MT_1000BASE_TX_HALF_DUP 0x0005
#define MT_1000BASE_TX_FULL_DUP 0x0006

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

/* Structure of an ethernet packet - internal
*/

struct EtherPacketHeader
{
    UBYTE       eph_Dest[ETHER_ADDR_SIZE]; /* 0 destination address */
    UBYTE       eph_Src[ETHER_ADDR_SIZE]; /* 6 originator  address */
    UWORD       eph_Type;                 /* 12 packet type */
};

/* Buffer management node - private
*/
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

/* Multicast address range record - private
*/
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

    struct Library     *np_SysBase;       /* cached execbase */
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
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* Endpoint 1 */
    struct PsdPipe     *ncp_EPOutPipe[2]; /* Endpoint 1 pipes */
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
    ULONG               ncp_PatchFlags;   /* Patchflags */
    UBYTE               ncp_MacAddress[ETHER_ADDR_SIZE]; /* Current Mac Address */
    UBYTE               ncp_ROMAddress[ETHER_ADDR_SIZE]; /* ROM Mac Address */
    UBYTE               ncp_EthCtrl[4];   /* shadow of the control registers (one byte padding) */
    ULONG               ncp_PhyID;        /* ID of the PHY */
    UWORD               ncp_PhyMode;
    UWORD               ncp_LedMode;


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
    struct IOSana2Req  *ncp_WritePending[2]; /* write IORequest pending */
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


#endif /* ASIXETH_H */
