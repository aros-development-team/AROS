#ifndef PEGASUS_H
#define PEGASUS_H

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

#define PF_PNA                  0x0001
#define PF_PEG2                 0x0002
#define PF_CHIP_8513            0x0004

#define DDF_CONFIGURED (1<<2)  /* station address is configured */
#define DDF_ONLINE     (1<<3)  /* device is online */
#define DDF_OFFLINE    (1<<4)  /* device was put offline */

#define DROPPED        (1<<0)  /* Did the packet get dropped? */
#define PACKETFILTER   (1<<1)  /* Use the packet filter? */

/* Ethernet address bytesize
*/
#define ETHER_ADDR_SIZE 6

#define ETHER_MIN_LEN  60           /* smallest amount that nic will accept */
#define ETHER_MAX_LEN  1536         /* largest legal amount for Ethernet */

/* Ethernet packet data sizes (maximum)
*/
#define ETHERPKT_SIZE  1500
#define RAWPKT_SIZE    1514

#define EEPROM_WRITE            0x01
#define EEPROM_READ             0x02
#define EEPROM_DONE             0x04
#define EEPROM_WR_ENABLE        0x10
#define EEPROM_LOAD             0x20

#define PHY_DONE                0x80
#define PHY_READ                0x40
#define PHY_WRITE               0x20
#define DEFAULT_GPIO_RESET      0x24
#define DEFAULT_GPIO_SET        0x26

#define RX_MULTICAST            0x02
#define RX_PROMISCUOUS          0x04

#define TX_UNDERRUN             0x80
#define EXCESSIVE_COL           0x40
#define LATE_COL                0x20
#define NO_CARRIER              0x10
#define LOSS_CARRIER            0x08
#define JABBER_TIMEOUT          0x04

#define LINK_STATUS             0x01

/* USB Control Pipe Commands */
#define UPGR_GET_REGS           0xf0
#define UPGR_SET_REGS           0xf1

/* registers */
#define PEGREG_ETH_CTRL0        0x00
#define PEGREG_ETH_CTRL1        0x01
#define PEGREG_ETH_CTRL2        0x02
#define PEGREG_MCAST            0x08
#define PEGREG_ETH_ID           0x10
#define PEGREG_REG_1D           0x1d
#define PEGREG_EEPROM_OFFSET    0x20
#define PEGREG_EEPROM_DATA_LOW  0x21
#define PEGREG_EEPROM_DATA_HIGH 0x22
#define PEGREG_EEPROM_CTRL      0x23
#define PEGREG_PHY_ADDR         0x25
#define PEGREG_PHY_DATA_LOW     0x26
#define PEGREG_PHY_DATA_HIGH    0x27
#define PEGREG_PHY_CTRL         0x28
#define PEGREG_USB_STATUS       0x2a
#define PEGREG_ETH_TX_STAT0     0x2b
#define PEGREG_ETH_TX_STAT1     0x2c
#define PEGREG_ETH_RX_STAT      0x2d
#define PEGREG_WAKEUP_CTRL      0x78
#define PEGREG_REG_7B           0x7b
#define PEGREG_GPIO0            0x7e
#define PEGREG_GPIO1            0x7f
#define PEGREG_REG_80           0x80
#define PEGREG_REG_81           0x81
#define PEGREG_REG_83           0x83
#define PEGREG_REG_84           0x84

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

/* Vendor IDs */

#define VENDOR_3COM             0x0506
#define VENDOR_ABOCOM           0x07b8
#define VENDOR_ACCTON           0x083a
#define VENDOR_ADMTEK           0x07a6
#define VENDOR_AEILAB           0x3334
#define VENDOR_ALLIEDTEL        0x07c9
#define VENDOR_ATEN             0x0557
#define VENDOR_BELKIN           0x050d
#define VENDOR_BILLIONTON       0x08dd
#define VENDOR_COMPAQ           0x049f
#define VENDOR_COREGA           0x07aa
#define VENDOR_DLINK            0x2001
#define VENDOR_ELCON            0x0db7
#define VENDOR_ELECOM           0x056e
#define VENDOR_ELSA             0x05cc
#define VENDOR_GIGABYTE         0x1044
#define VENDOR_HAWKING          0x0e66
#define VENDOR_HP               0x03f0
#define VENDOR_IODATA           0x04bb
#define VENDOR_KINGSTON         0x0951
#define VENDOR_LANEED           0x056e
#define VENDOR_LINKSYS          0x066b
#define VENDOR_LINKSYS2         0x077b
#define VENDOR_MELCO            0x0411
#define VENDOR_MICROSOFT        0x045e
#define VENDOR_MOBILITY         0x1342
#define VENDOR_NETGEAR          0x0846
#define VENDOR_OCT              0x0b39
#define VENDOR_SMARTBRIDGES     0x08d1
#define VENDOR_SMC              0x0707
#define VENDOR_SOHOWARE         0x15e8
#define VENDOR_SIEMENS          0x067c

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab

#define MT_AUTO                 0x0000
#define MT_10BASE_T_HALF_DUP    0x0001
#define MT_10BASE_T_FULL_DUP    0x0002
#define MT_100BASE_TX_HALF_DUP  0x0003
#define MT_100BASE_TX_FULL_DUP  0x0004

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


#endif /* PEGASUS_H */
