#ifndef DEVICES_SANA2_H
#define DEVICES_SANA2_H

/*
    Copyright (C) 2005 Neil Cafferkey
    $Id$

    Desc: Definitions for SANA-II devices
    Lang: english
*/

#include <exec/types.h>
#include <exec/io.h>
#include <devices/timer.h>


/* Constants */
/* ========= */

#define SANA2_MAX_ADDR_BITS  128
#define SANA2_MAX_ADDR_BYTES (SANA2_MAX_ADDR_BITS / 8)

#define SANA2IOB_RAW   7
#define SANA2IOB_BCAST 6
#define SANA2IOB_MCAST 5

#define SANA2IOF_RAW   (1 << SANA2IOB_RAW)
#define SANA2IOF_BCAST (1 << SANA2IOB_BCAST)
#define SANA2IOF_MCAST (1 << SANA2IOB_MCAST)

#define SANA2OPB_PROM 1
#define SANA2OPB_MINE 0

#define SANA2OPF_PROM (1 << SANA2OPB_PROM)
#define SANA2OPF_MINE (1 << SANA2OPB_MINE)

#define S2_Dummy             (TAG_USER + 0xb0000)
#define S2_CopyToBuff        (S2_Dummy + 1)
#define S2_CopyFromBuff      (S2_Dummy + 2)
#define S2_PacketFilter      (S2_Dummy + 3)
#define S2_CopyToBuff16      (S2_Dummy + 4)
#define S2_CopyFromBuff16    (S2_Dummy + 5)
#define S2_CopyToBuff32      (S2_Dummy + 6)
#define S2_CopyFromBuff32    (S2_Dummy + 7)
#define S2_DMACopyToBuff32   (S2_Dummy + 8)
#define S2_DMACopyFromBuff32 (S2_Dummy + 9)

#define S2WireType_Ethernet    1
#define S2WireType_Arcnet      7
#define S2WireType_LocalTalk  11
#define S2WireType_DyLAN      12
#define S2WireType_AmokNet   200
#define S2WireType_Liana     202
#define S2WireType_PPP       253
#define S2WireType_SLIP      254
#define S2WireType_CSLIP     255
#define S2WireType_PLIP      420

#define S2_DEVICEQUERY           (CMD_NONSTD + 0)
#define S2_GETSTATIONADDRESS     (CMD_NONSTD + 1)
#define S2_CONFIGINTERFACE       (CMD_NONSTD + 2)
#define S2_ADDMULTICASTADDRESS   (CMD_NONSTD + 5)
#define S2_DELMULTICASTADDRESS   (CMD_NONSTD + 6)
#define S2_MULTICAST             (CMD_NONSTD + 7)
#define S2_BROADCAST             (CMD_NONSTD + 8)
#define S2_TRACKTYPE             (CMD_NONSTD + 9)
#define S2_UNTRACKTYPE           (CMD_NONSTD + 10)
#define S2_GETTYPESTATS          (CMD_NONSTD + 11)
#define S2_GETSPECIALSTATS       (CMD_NONSTD + 12)
#define S2_GETGLOBALSTATS        (CMD_NONSTD + 13)
#define S2_ONEVENT               (CMD_NONSTD + 14)
#define S2_READORPHAN            (CMD_NONSTD + 15)
#define S2_ONLINE                (CMD_NONSTD + 16)
#define S2_OFFLINE               (CMD_NONSTD + 17)
#define S2_ADDMULTICASTADDRESSES (CMD_NONSTD + 0xc000)
#define S2_DELMULTICASTADDRESSES (CMD_NONSTD + 0xc001)

#define S2ERR_NO_ERROR       0
#define S2ERR_NO_RESOURCES   1
#define S2ERR_BAD_ARGUMENT   3
#define S2ERR_BAD_STATE      4
#define S2ERR_BAD_ADDRESS    5
#define S2ERR_MTU_EXCEEDED   6
#define S2ERR_NOT_SUPPORTED  8
#define S2ERR_SOFTWARE       9
#define S2ERR_OUTOFSERVICE  10
#define S2ERR_TX_FAILURE    11

#define S2WERR_GENERIC_ERROR     0
#define S2WERR_NOT_CONFIGURED    1
#define S2WERR_UNIT_ONLINE       2
#define S2WERR_UNIT_OFFLINE      3
#define S2WERR_ALREADY_TRACKED   4
#define S2WERR_NOT_TRACKED       5
#define S2WERR_BUFF_ERROR        6
#define S2WERR_SRC_ADDRESS       7
#define S2WERR_DST_ADDRESS       8
#define S2WERR_BAD_BROADCAST     9
#define S2WERR_BAD_MULTICAST    10
#define S2WERR_MULTICAST_FULL   11
#define S2WERR_BAD_EVENT        12
#define S2WERR_BAD_STATDATA     13
#define S2WERR_IS_CONFIGURED    15
#define S2WERR_NULL_POINTER     16
#define S2WERR_TOO_MANY_RETRIES 17
#define S2WERR_RCVRBLE_HDW_ERR  18

#define S2EVENT_ERROR    (1 << 0)
#define S2EVENT_TX       (1 << 1)
#define S2EVENT_RX       (1 << 2)
#define S2EVENT_ONLINE   (1 << 3)
#define S2EVENT_OFFLINE  (1 << 4)
#define S2EVENT_BUFF     (1 << 5)
#define S2EVENT_HARDWARE (1 << 6)
#define S2EVENT_SOFTWARE (1 << 7)


/* Structures */
/* ========== */

struct IOSana2Req
{
   struct IORequest ios2_Req;
   ULONG ios2_WireError;
   ULONG ios2_PacketType;
   UBYTE ios2_SrcAddr[SANA2_MAX_ADDR_BYTES];
   UBYTE ios2_DstAddr[SANA2_MAX_ADDR_BYTES];
   ULONG ios2_DataLength;
   VOID *ios2_Data;
   VOID *ios2_StatData;
   VOID *ios2_BufferManagement;
};

struct Sana2DeviceQuery
{
   ULONG SizeAvailable;
   ULONG SizeSupplied;
   ULONG DevQueryFormat;
   ULONG DeviceLevel;
   UWORD AddrFieldSize;
   ULONG MTU;
   ULONG BPS;
   ULONG HardwareType;
};

struct Sana2PacketTypeStats
{
   ULONG PacketsSent;
   ULONG PacketsReceived;
   ULONG BytesSent;
   ULONG BytesReceived;
   ULONG PacketsDropped;
};

struct Sana2SpecialStatRecord
{
   ULONG Type;
   ULONG Count;
   const TEXT *String;
};

struct Sana2SpecialStatHeader
{
   ULONG RecordCountMax;
   ULONG RecordCountSupplied;
};

struct Sana2DeviceStats
{
   ULONG PacketsReceived;
   ULONG PacketsSent;
   ULONG BadData;
   ULONG Overruns;
   ULONG Unused;
   ULONG UnknownTypesReceived;
   ULONG Reconfigurations;
   struct timeval LastStart;
};


/* Obsolete definitions */
/* ==================== */

#define SANA2IOB_QUICK IOB_QUICK

#define S2WERR_TOO_MANY_RETIRES S2WERR_TOO_MANY_RETRIES
#define S2WERR_RCVREL_HDW_ERR   S2WERR_RCVRBLE_HDW_ERR

#define S2WireType_IEEE802 6

#define S2_START CMD_NONSTD
#define S2_END   (CMD_NONSTD + 18)

#endif
