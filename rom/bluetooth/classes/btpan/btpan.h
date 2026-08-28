#ifndef BTPAN_H
#define BTPAN_H

/*
 * btpan.class - Bluetooth PAN (personal area networking). Binds to
 * NAP/GN/PANU services of registered devices, speaks BNEP over L2CAP
 * PSM 0x000F and exposes each network as a SANA-II unit of btpan.device,
 * the way Poseidon's USB ethernet classes expose their adapters.
 */

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <utility/utility.h>
#include <utility/hooks.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>

#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#define NewList NEWLIST
#define min(x,y) (((x) < (y)) ? (x) : (y))

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


struct BTPanDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct BTPanBase  *np_ClsBase;       /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct BTPanUnit
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    ULONG               ncp_OpenFlags;    /* Flags used to open the device */
    struct BTPanBase  *ncp_ClsBase;      /* Up linkage */
    struct BTPanDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* bluetooth.library base (unit task) */
    struct BtDevice    *ncp_Device;       /* Up linkage */
    struct BtService   *ncp_Service;      /* the PAN service bound */
    struct BtEndpoint  *ncp_Endpoint;     /* its BNEP L2CAP endpoint (PSM 0x000f) */
    APTR                ncp_Interface;    /* NULL for the default (config) unit */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    APTR                ncp_ReadCh;       /* BNEP channel: one read always pending */
    APTR                ncp_WriteCh;      /* BNEP channel: one write in flight */
    UWORD               ncp_WriteKind;    /* what the write in flight is (BPWK_xxx) */
    UWORD               ncp_BNEPState;    /* BPS_xxx */
    UBYTE               ncp_PeerAddr[6];  /* the peer's BD address */
    UBYTE               ncp_CtlBuf[32];   /* control message being sent / queued */
    UWORD               ncp_CtlLen;       /* pending control reply (0 = none) */
    UWORD               ncp_RetryMS;      /* re-setup backoff, doubles to 60 s (0 = healthy) */
    BOOL                ncp_EstLogged;    /* "connection established" said once already */
    BOOL                ncp_LossLogged;   /* outage reported - say so when it recovers */
    UBYTE              *ncp_AsmBuf;       /* reassembled ethernet frame */

    /* the persistent identity of the unit (units survive rebinds) */
    UBYTE               ncp_UnitAddr[6];  /* device BD address */
    UWORD               ncp_UnitUUID;     /* service class (NAP/GN/PANU) */
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
    struct Library     *ncp_BtBase;      /* Poseidon base */
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

struct BTPanBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct BTPanDevBase *nh_DevBase;     /* base of device created */
    struct List         nh_Units;         /* List of units available */

    struct BTPanUnit  nh_DefaultUnit;      /* Dummy ncp for default config */
};


/* BNEP */
#define BNEP_PSM              0x000f
#define BNEP_GENERAL          0x00
#define BNEP_CONTROL          0x01
#define BNEP_COMPRESSED       0x02
#define BNEP_COMPRESSED_SRC   0x03
#define BNEP_COMPRESSED_DST   0x04
#define BNEP_EXT_FLAG         0x80

#define BNEP_CTL_NOT_UNDERSTOOD  0x00
#define BNEP_CTL_SETUP_REQ       0x01
#define BNEP_CTL_SETUP_RSP       0x02
#define BNEP_CTL_FILTER_NET_SET  0x03
#define BNEP_CTL_FILTER_NET_RSP  0x04
#define BNEP_CTL_FILTER_MC_SET   0x05
#define BNEP_CTL_FILTER_MC_RSP   0x06

/* ncp_BNEPState */
#define BPS_DOWN   0   /* no BNEP connection: a setup request is due */
#define BPS_SETUP  1   /* setup request sent, waiting for the response */
#define BPS_UP     2

/* ncp_WriteKind */
#define BPWK_NONE  0
#define BPWK_DATA  1   /* ncp_WritePending[0] completes with it */
#define BPWK_CTL   2

#define BNEP_HDRMAX 20 /* room in front of a frame for the largest BNEP header */

/* Protos */

struct BTPanUnit * GM_UNIQUENAME(bAttemptServiceBinding)(struct BTPanBase *nh, struct BtService *bsv);
struct BTPanUnit * GM_UNIQUENAME(bForceServiceBinding)(struct BTPanBase *nh, struct BtService *bsv);
void GM_UNIQUENAME(bReleaseServiceBinding)(struct BTPanBase *nh, struct BTPanUnit *ncp);

struct BTPanUnit * bAllocEth(void);
void bFreeEth(struct BTPanUnit *ncp);

void bDoEvent(struct BTPanUnit *ncp, ULONG events);
BOOL bWritePacket(struct BTPanUnit *ncp, struct IOSana2Req *ioreq);
BOOL bReadPacket(struct BTPanUnit *ncp, UBYTE *pktptr, ULONG len);

BOOL bLoadClassConfig(struct BTPanBase *nh);
BOOL bLoadBindingConfig(struct BTPanUnit *ncp);
LONG bOpenBindingCfgWindow(struct BTPanBase *nh, struct BTPanUnit *ncp);

void bGUITaskCleanup(struct BTPanUnit *nh);
void bGetMACAddress(UBYTE *macaddr, CONST_STRPTR tmpstr);

AROS_UFP0(void, bEthTask);
AROS_UFP0(void, bGUITask);

#include "dev.h"

#endif /* BTPAN_H */
