#ifndef LAN78XX_CLASS_H
#define LAN78XX_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                        Includes for lan78xx class
 *----------------------------------------------------------------------------
 *                   Copyright (C) 2026, The AROS Development Team.
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "dev.h"
#include "lan78xx_regs.h"

#define DDF_CONFIGURED (1 << 2)
#define DDF_ONLINE (1 << 3)
#define DDF_OFFLINE (1 << 4)

#define DROPPED (1 << 0)
#define PACKETFILTER (1 << 1)

#define ID_ABOUT 0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG 0xaaaaaaab

struct ClsDevCfg {
    ULONG lc_ChunkID;
    ULONG lc_Length;
    ULONG lc_DefaultUnit;
    ULONG lc_MediaType;
};

struct EtherPacketHeader {
    UBYTE eph_Dest[ETHER_ADDR_SIZE];
    UBYTE eph_Src[ETHER_ADDR_SIZE];
    UWORD eph_Type;
};

struct BufMan {
    struct Node bm_Node;
    APTR bm_DMACopyFromBuf32;
    APTR bm_CopyFromBuf;
    APTR bm_DMACopyToBuf32;
    APTR bm_CopyToBuf;
    APTR bm_PacketFilter;
    struct List bm_RXQueue;
};

struct MulticastAddressRange {
    struct Node mar_Node;
    ULONG mar_UseCount;
    UBYTE mar_LowerAddr[ETHER_ADDR_SIZE];
    UBYTE mar_UpperAddr[ETHER_ADDR_SIZE];
};

struct PacketTypeStats {
    struct Node pts_Node;
    ULONG pts_PacketType;
    struct Sana2PacketTypeStats pts_Stats;
};

struct NepClassHid;

struct NepEthDevBase {
    struct Library np_Library;
    UWORD np_Flags;

    BPTR np_SegList;
    struct NepEthBase *np_ClsBase;
    struct Library *np_UtilityBase;
};

struct NepClassEth {
    struct Unit ncp_Unit;
    ULONG ncp_UnitNo;
    ULONG ncp_OpenFlags;
    struct NepEthBase *ncp_ClsBase;
    struct NepEthDevBase *ncp_DevBase;
    struct Library *ncp_Base;
    struct PsdDevice *ncp_Device;
    struct PsdConfig *ncp_Config;
    struct PsdInterface *ncp_Interface;
    struct Task *ncp_ReadySigTask;
    LONG ncp_ReadySignal;
    struct Task *ncp_Task;
    struct MsgPort *ncp_TaskMsgPort;

    struct PsdPipe *ncp_EP0Pipe;
    struct PsdEndpoint *ncp_EPOut;
    struct PsdPipe *ncp_EPOutPipe;
    IPTR ncp_EPOutMaxPktSize;
    struct PsdEndpoint *ncp_EPIn;
    struct PsdPipe *ncp_EPInPipe;
    struct MsgPort *ncp_DevMsgPort;
    UWORD ncp_UnitProdID;
    UWORD ncp_UnitVendorID;

    struct List ncp_BufManList;
    struct List ncp_EventList;
    struct List ncp_TrackList;
    struct List ncp_Multicasts;
    UBYTE ncp_MacAddress[ETHER_ADDR_SIZE];
    UBYTE ncp_ROMAddress[ETHER_ADDR_SIZE];

    UBYTE ncp_MulticastArray[8];
    ULONG ncp_StateFlags;

    ULONG ncp_Retries;
    ULONG ncp_BadMulticasts;

    UBYTE *ncp_ReadBuffer[2];
    UBYTE *ncp_WriteBuffer[2];

    UWORD ncp_ReadBufNum;
    UWORD ncp_WriteBufNum;
    UWORD ncp_RxIdleStreak;

    ULONG ncp_ChipID;
    UWORD ncp_ChipRev;
    UWORD ncp_ChipFlags; /* LAN78XX_FLAG_LAN7500, ... */
    UBYTE ncp_EepromPresent;
    UBYTE ncp_PhyNo;
    UWORD ncp_LinkSpeed;
    UBYTE ncp_FullDuplex;
    UBYTE ncp_LinkUp;

    struct Sana2DeviceStats ncp_DeviceStats;
    struct Sana2PacketTypeStats *ncp_TypeStats2048;
    struct Sana2PacketTypeStats *ncp_TypeStats2054;

    UBYTE *ncp_ReadPending;
    struct IOSana2Req *ncp_WritePending;
    struct List ncp_OrphanQueue;
    struct List ncp_WriteQueue;

    UBYTE ncp_DevIDString[128];

    BOOL ncp_UsingDefaultCfg;
    struct ClsDevCfg *ncp_CDC;

    struct Library *ncp_MUIBase;
    struct Library *ncp_PsdBase;
    struct Library *ncp_IntBase;
    struct Task *ncp_GUITask;
    struct NepClassHid *ncp_GUIBinding;

    Object *ncp_App;
    Object *ncp_MainWindow;
    Object *ncp_UnitObj;
    Object *ncp_MediaTypeObj;
    Object *ncp_UseObj;
    Object *ncp_SetDefaultObj;
    Object *ncp_CloseObj;
    Object *ncp_AboutMI;
    Object *ncp_UseMI;
    Object *ncp_SetDefaultMI;
    Object *ncp_MUIPrefsMI;
};

struct NepEthBase {
    struct Library nh_Library;
    UWORD nh_Flags;

    struct Library *nh_UtilityBase;

    struct NepEthDevBase *nh_DevBase;
    struct List nh_Units;

    struct NepClassEth nh_DummyNCP;
};

struct NepClassEth *usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
struct NepClassEth *usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp);

struct NepClassEth *nAllocEth(void);
void nFreeEth(struct NepClassEth *ncp);

BOOL nInitHardware(struct NepClassEth *ncp);
void nSetOnline(struct NepClassEth *ncp);
void nUpdateRXMode(struct NepClassEth *ncp);

void nDoEvent(struct NepClassEth *ncp, ULONG events);
BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq);
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG len);

BOOL nLoadClassConfig(struct NepEthBase *nh);
BOOL nLoadBindingConfig(struct NepClassEth *ncp);
LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp);

void nGUITaskCleanup(struct NepClassEth *nh);

LONG lan78xx_read_reg(struct NepClassEth *ncp, UWORD reg, ULONG *val);
LONG lan78xx_write_reg(struct NepClassEth *ncp, UWORD reg, ULONG val);
LONG lan78xx_phy_read(struct NepClassEth *ncp, UWORD reg, UWORD *val);
LONG lan78xx_phy_write(struct NepClassEth *ncp, UWORD reg, UWORD val);

AROS_UFP0(void, nEthTask);
AROS_UFP0(void, nGUITask);

#endif
