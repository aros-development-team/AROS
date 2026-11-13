#ifndef ETHWRAP_H
#define ETHWRAP_H

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

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_DefaultUnit;
    UBYTE cdc_MACAddress[ETHER_ADDR_SIZE];
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
    Object             *ncp_MACAddressObj;

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


#endif /* ETHWRAP_H */
