/*

Prometheus NE2000 driver.

Copyright (C) 2001-2004 Matay

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

/// includes and defines

#define __NOLIBBASE__

#ifdef __AROS__
#include <aros/asmcall.h>
#include <aros/libcall.h>
#else
#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT
#define AROS_INTFUNC_INIT
#define AROS_INTFUNC_EXIT
#endif

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/prometheus.h>
#include <proto/timer.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/errors.h>
#include <devices/sana2.h>
#include <hardware/intbits.h>
#include <dos/dostags.h>

#include <string.h>

#include "rev.h"
#include "ne2000.h"
#include "endian.h"
#include "io.h"

#define OK           0

#define TX_BUFFER    0x40
#define RX_BUFFER    0x46
#define BUFFER_END   0x80
#define INTMASK      (INT_RXPACKET | INT_TXPACKET | INT_TXERROR)

/* PCI card IDs */

#define PCI_VENDOR_REALTEK       0x10EC
#define PCI_DEVICE_RTL8029       0x8029

// Unit flags.

#define UF_CONFIGURED   0x01
#define UF_ONLINE       0x02
#define UF_PROMISCUOUS  0x04

// Macro for registerized parameters (used in some OS functions).

#ifndef REG
#if defined(__mc68000) && !defined(__AROS__)
#define _REG(A, B) B __asm(#A)
#define REG(A, B) _REG(A, B)
#else
#define REG(A, B) B
#endif
#endif

// Macro for declaring local libraries bases.

#define USE(a) struct Library * a = dd->dd_##a;
#define USE_U(a) struct Library * a = ud->ud_##a;

// Debug on/off switch (debug off if commented).

//#define PDEBUG 1

// Macros for debug messages.

#ifdef PDEBUG
  #define USE_D(a) struct Library *##a = dd->dd_##a;
  #define USE_UD(a) struct Library *##a = ud->ud_##a;
  #define DBG(a) FPrintf(dd->debug, a "\n")
  #define DBG_U(a) FPrintf(ud->debug, a "\n")
  #define DBG_T(a) FPrintf(ud->tdebug, a "\n")
  #define DBG1(a,b) FPrintf(dd->debug, a "\n",(LONG)b)
  #define DBG1_U(a,b) FPrintf(ud->debug, a "\n",(LONG)b)
  #define DBG1_T(a,b) FPrintf(ud->tdebug, a "\n",(LONG)b)
  #define DBG2(a,b,c) FPrintf(dd->debug, a "\n",(LONG)b,(LONG)c)
  #define DBG2_U(a,b,c) FPrintf(ud->debug, a "\n",(LONG)b,(LONG)c)
  #define DBG2_T(a,b,c) FPrintf(ud->tdebug, a "\n",(LONG)b,(LONG)c)
#else
  #define USE_D(a)
  #define USE_UD(a)
  #define DBG(a)
  #define DBG_U(a)
  #define DBG_T(a)
  #define DBG1(a,b)
  #define DBG1_U(a,b)
  #define DBG1_T(a,b)
  #define DBG2(a,b,c)
  #define DBG2_U(a,b,c)
  #define DBG2_T(a,b,c)
#endif

// New Style Device support

#define NSCMD_DEVICEQUERY   0x4000

struct NSDeviceQueryResult
 {
  ULONG   DevQueryFormat;         /* this is type 0               */
  ULONG   SizeAvailable;          /* bytes available              */
  UWORD   DeviceType;             /* what the device does         */
  UWORD   DeviceSubType;          /* depends on the main type     */
  UWORD   *SupportedCommands;     /* 0 terminated list of cmd's   */
 };

#define NSDEVTYPE_SANA2       7   /* A >=SANA2R2 networking device */

///
/// device structures

struct DevData
  {
    struct Library           dd_Lib;
    APTR                     dd_SegList;
    struct Library          *dd_SysBase;
    struct Library          *dd_PrometheusBase;
    struct Library          *dd_UtilityBase;
    struct Library          *dd_DOSBase;
    struct Library          *dd_TimerBase;
    struct UnitData         *dd_Units[4];
    struct timerequest       dd_Treq;


    #ifdef PDEBUG
    BPTR                debug;
    UBYTE               dpath[128];
    #endif
  };

struct UnitData
  {
    struct Message           ud_Message;
    IPTR                     ud_Hardware;
    struct Library          *ud_SysBase;
    struct Library          *ud_PrometheusBase;
    struct Library          *ud_DOSBase;
    struct Library          *ud_TimerBase;
    struct Interrupt        *ud_Interrupt;
    struct Task             *ud_Task;
    struct MsgPort          *ud_TaskPort;
    struct MsgPort          *ud_LifeTime;
    struct MinList           ud_RxQueue;
    struct MinList           ud_TxQueue;
    struct IOSana2Req       *ud_PendingWrite;
    APTR                     ud_Board;
    ULONG                    ud_OpenCnt;
    ULONG                    ud_GoWriteMask;
    UWORD                    ud_RxBuffer[768];
    UBYTE                    ud_Name[24];
    UBYTE                    ud_EtherAddress[6];
    UBYTE                    ud_SoftAddress[6];
    struct Sana2DeviceStats  ud_DevStats;
    UBYTE                    ud_Flags;
    UBYTE                    ud_NextPage;
    BYTE                     ud_GoWriteBit;
    UBYTE                    pad;
    #ifdef PDEBUG
    BPTR                     debug;
    BPTR                     tdebug;
    #endif
  };

struct BuffFunctions
  {
    BOOL (*bf_CopyTo)(REG(a0, APTR), REG(a1, APTR), REG(d0, ULONG));
    BOOL (*bf_CopyFrom)(REG(a0, APTR), REG(a1, APTR), REG(d0, ULONG));
  };

///
/// prototypes

#ifdef __AROS__
AROS_UFP3(struct DevData *, DevInit,
   AROS_UFPA(IPTR, num, D0),
   AROS_UFPA(APTR, seglist, A0),
   AROS_UFPA(struct Library *, sysb, A6));
AROS_LD3(LONG, DevOpen,
   AROS_LDA(struct IOSana2Req *, req, A1),
   AROS_LDA(LONG, unit, D0),
   AROS_LDA(ULONG, flags, D1),
   struct DevData *, dd, 1, S2);
AROS_LD1(APTR, DevClose,
   AROS_LDA(struct IOSana2Req *, req, A1),
   struct DevData *, dd, 2, S2);
AROS_LD0(APTR, DevExpunge,
   struct DevData *, dd, 3, S2);
AROS_LD0(APTR, DevReserved,
   struct DevData *, dd, 4, S2);
AROS_LD1(VOID, DevBeginIO,
   AROS_LDA(struct IOSana2Req *, req, A1),
   struct DevData *, dd, 5, S2);
AROS_LD1(ULONG, DevAbortIO,
   AROS_LDA(struct IOSana2Req *, req, A1),
   struct DevData *, dd, 6, S2);
#else
LONG DevOpen(REG(a1, struct IOSana2Req *req), REG(d0, LONG unit),
  REG(d1, LONG flags), REG(a6, struct DevData *dd));
APTR DevClose(REG(a1, struct IOSana2Req *req), REG(a6, struct DevData *dd));
APTR DevExpunge(REG(a6, struct DevData *dd));
LONG DevReserved(void);
void DevBeginIO(REG(a1, struct IOSana2Req *req),
  REG(a6, struct DevData *dd));
ULONG DevAbortIO(REG(a1, struct IOSana2Req *req),
  REG(a6, struct DevData *dd));
#endif

void IoDone(struct UnitData *ud, struct IOSana2Req *req, LONG err, LONG werr);
LONG OpenDeviceLibraries(struct DevData *dd);
void CloseDeviceLibraries(struct DevData *dd);
LONG PrepareCookie(struct IOSana2Req *req, struct DevData *dd);
LONG RunTask(struct DevData *dd, struct UnitData *ud);
void ClearGlobalStats(struct UnitData *ud);
struct UnitData *OpenUnit(struct DevData *dd, LONG unit, LONG flags);
struct UnitData *InitializeUnit(struct DevData *dd, LONG unit);
void CloseUnit(struct DevData *dd, struct UnitData *ud);
void ExpungeUnit(struct DevData *dd, struct UnitData *ud);

void CmdNSDQuery(struct UnitData *ud, struct IOStdReq *req);
void S2DeviceQuery(struct UnitData *ud, struct IOSana2Req *req);
void S2GetStationAddress(struct UnitData *ud, struct IOSana2Req *req);
void S2ConfigInterface(struct UnitData *ud, struct IOSana2Req *req);
void S2Online(struct UnitData *ud, struct IOSana2Req *req);
void S2Offline(struct UnitData *ud, struct IOSana2Req *req);
void S2GetGlobalStats(struct UnitData *ud, struct IOSana2Req *req);

void HardwareReset(struct UnitData *ud);
void HardwareInit (struct UnitData *ud);
IPTR FindHardware(struct DevData *dd, WORD unit, struct UnitData *ud);
void GoOnline (struct UnitData *ud);
void GoOffline(struct UnitData *ud);
void GetHwAddress(struct UnitData *ud);
void WriteHwAddress(struct UnitData *ud);
LONG PacketReceived(struct UnitData *ud);
LONG RingBufferNotEmpty(struct UnitData *ud);

struct IOSana2Req *SearchReadRequest(struct UnitData *ud,
  struct MinList *queue, ULONG type);
void UnitTask(void);

#ifdef __AROS__
struct Library *sys_base;
#endif

///
/// tables and constants

extern struct Resident romtag;
const UBYTE IdString[] = DEV_IDSTRING;

const void *FuncTable[] =
  {
#ifdef __AROS__
    AROS_SLIB_ENTRY(DevOpen, S2, 1),
    AROS_SLIB_ENTRY(DevClose, S2, 2),
    AROS_SLIB_ENTRY(DevExpunge, S2, 3),
    AROS_SLIB_ENTRY(DevReserved, S2, 4),
    AROS_SLIB_ENTRY(DevBeginIO, S2, 5),
    AROS_SLIB_ENTRY(DevAbortIO, S2, 6),
#else
    DevOpen,
    DevClose,
    DevExpunge,
    DevReserved,
    DevBeginIO,
    DevAbortIO,
#endif
    (APTR)-1
  };

UWORD NSDSupported[] =
  {
    CMD_READ,
    CMD_WRITE,
    CMD_FLUSH,
    S2_DEVICEQUERY,
    S2_GETSTATIONADDRESS,
    S2_CONFIGINTERFACE,
    S2_BROADCAST,
    S2_GETGLOBALSTATS,
    S2_ONLINE,
    S2_OFFLINE,
    NSCMD_DEVICEQUERY,
    0
  };

///

/// DevInit()
// Called when the device is loaded into memory. Makes system library, initializes Library structure, opens
// libraries used by the device. Returns device base or NULL if init failed.

#ifdef __AROS__
AROS_UFH3(struct DevData *, DevInit,
   AROS_UFHA(IPTR, num, D0),
   AROS_UFHA(APTR, seglist, A0),
   AROS_UFHA(struct Library *, sysb, A6))
#else
struct DevData *DevInit(REG(d0, ULONG num), REG(a0, void *seglist),
  REG(a6, struct Library *sysb))
#endif
  {
    AROS_USERFUNC_INIT

    struct DevData *dd;
    struct Library *SysBase = sysb;

    if (dd = (struct DevData*)MakeLibrary(FuncTable, NULL, NULL,
      sizeof(struct DevData), 0))
      {
        dd->dd_Lib.lib_Node.ln_Type = NT_DEVICE;
        dd->dd_Lib.lib_Node.ln_Name = (TEXT *)romtag.rt_Name;
        dd->dd_Lib.lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
        dd->dd_Lib.lib_Version = DEV_VERSION;
        dd->dd_Lib.lib_Revision = DEV_REVISION;
        dd->dd_Lib.lib_IdString = (TEXT *)romtag.rt_IdString;
        dd->dd_Lib.lib_OpenCnt = 0;
        dd->dd_SegList = seglist;
        dd->dd_SysBase = SysBase;
        if (OpenDeviceLibraries(dd))
          {
            USE_D(DOSBase)
            WORD i;

            for (i = 0; i < 4; i++) dd->dd_Units[i] = NULL;

#ifdef __AROS__
            sys_base = SysBase;
#endif

            #ifdef PDEBUG
            strcpy(dd->dpath, "KCON:0/17/400/300/prm-rtl8029.device (main)/AUTO/CLOSE/WAIT");
            GetVar("PrometheusDebug", dd->dpath, 128, 0);
            dd->debug = Open(dd->dpath, MODE_NEWFILE);
            #endif

            DBG1("Device initialized, base at $%08lx.", dd);
            AddDevice((struct Device*)dd);
            return dd;
          }
        CloseDeviceLibraries(dd);
      }
    return NULL;

    AROS_USERFUNC_EXIT
  }

///
/// DevOpen()

#ifdef __AROS__
AROS_LH3(LONG, DevOpen,
   AROS_LHA(struct IOSana2Req *, req, A1),
   AROS_LHA(LONG, unit, D0),
   AROS_LHA(ULONG, flags, D1),
   struct DevData *, dd, 1, S2)
#else
LONG DevOpen(REG(a1, struct IOSana2Req *req), REG(d0, LONG unit),
  REG(d1, LONG flags), REG(a6, struct DevData *dd))
#endif
  {
    AROS_LIBFUNC_INIT

    USE_D(DOSBase)
    struct UnitData *ud;

    DBG("DevOpen() called.");
    dd->dd_Lib.lib_OpenCnt++;     // expunge protection

    if ((unit >= 0) && (unit <= 3))
      {
        if (ud = OpenUnit(dd, unit, flags))
          {
            req->ios2_Req.io_Error = 0;
            req->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            req->ios2_Req.io_Device = (struct Device*)dd;
            req->ios2_Req.io_Unit = (struct Unit*)ud;
            if (PrepareCookie(req, dd))
              {
                dd->dd_Lib.lib_Flags &= ~LIBF_DELEXP;
                DBG("DevOpen(): device opened successfully.");
                return 0;
              }
            DBG("PrepareCookie() failed.");
            CloseUnit(dd, ud);
          }
      }
    req->ios2_Req.io_Error = IOERR_OPENFAIL;
    req->ios2_Req.io_Device = (struct Device*)-1;
    req->ios2_Req.io_Unit = (struct Unit*)-1;
    dd->dd_Lib.lib_OpenCnt--;                       /* end of expunge protection */
    return IOERR_OPENFAIL;

    AROS_LIBFUNC_EXIT
  }

///
/// DevClose()

#ifdef __AROS__
AROS_LH1(APTR, DevClose,
   AROS_LHA(struct IOSana2Req *, req, A1),
   struct DevData *, dd, 2, S2)
#else
APTR DevClose(REG(a1, struct IOSana2Req *req), REG(a6, struct DevData *dd))
#endif
  {
    AROS_LIBFUNC_INIT

    USE(SysBase)
    USE_D(DOSBase)

    CloseUnit(dd, (struct UnitData*)req->ios2_Req.io_Unit);

    if (req->ios2_BufferManagement) FreeMem(req->ios2_BufferManagement,
      sizeof(struct BuffFunctions));

    if (--dd->dd_Lib.lib_OpenCnt == 0)
      {
        DBG("DevClose(): open counter reached 0.");
        if (dd->dd_Lib.lib_Flags & LIBF_DELEXP)
#ifdef __AROS__
        return AROS_LVO_CALL0(APTR, struct DevData *, dd, 2, );
#else
          return (DevExpunge(dd));
#endif
      }
    return 0;

    AROS_LIBFUNC_EXIT
  }

///
/// DevExpunge()

#ifdef __AROS__
AROS_LH0(APTR, DevExpunge,
   struct DevData *, dd, 3, S2)
#else
APTR DevExpunge(REG(a6, struct DevData *dd))
#endif
  {
    AROS_LIBFUNC_INIT

    USE(SysBase)
    USE_D(DOSBase)
    APTR seglist;

    if (dd->dd_Lib.lib_OpenCnt)
      {
        dd->dd_Lib.lib_Flags |= LIBF_DELEXP;
        return 0;
      }
    Remove((struct Node*)dd);
    CloseDeviceLibraries(dd);
    seglist = dd->dd_SegList;
    FreeMem((APTR)dd - dd->dd_Lib.lib_NegSize,
        (IPTR)dd->dd_Lib.lib_PosSize + (IPTR)dd->dd_Lib.lib_NegSize);
    DBG("DevExpunge(): expunged.");
    return seglist;

    AROS_LIBFUNC_EXIT
  }

///
/// DevReserved()

#ifdef __AROS__
AROS_LH0(APTR, DevReserved,
   struct DevData *, dd, 4, S2)
#else
LONG DevReserved (void)
#endif
  {
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
  }

///
/// DevBeginIo()

#ifdef __AROS__
AROS_LH1(VOID, DevBeginIO,
   AROS_LHA(struct IOSana2Req *, req, A1),
   struct DevData *, dd, 5, S2)
#else
void DevBeginIO(REG(a1, struct IOSana2Req *req),
  REG(a6, struct DevData *dd))
#endif
  {
    AROS_LIBFUNC_INIT

    USE(SysBase)
    USE_D(DOSBase)
    struct UnitData *ud = (struct UnitData*)req->ios2_Req.io_Unit;
    WORD i;

    switch(req->ios2_Req.io_Command)
      {
        case NSCMD_DEVICEQUERY:       CmdNSDQuery(ud, (struct IOStdReq*)req);   break;
        case S2_DEVICEQUERY:          S2DeviceQuery(ud, req);                   break;
        case S2_GETSTATIONADDRESS:    S2GetStationAddress(ud, req);             break;
        case S2_CONFIGINTERFACE:      S2ConfigInterface(ud, req);               break;
        case S2_ONLINE:               S2Online(ud, req);                        break;
        case S2_OFFLINE:              S2Offline(ud, req);                       break;
        case S2_GETGLOBALSTATS:       S2GetGlobalStats(ud, req);                break;

        case CMD_READ:
          DBG1("CMD_READ [$%08lx].", (LONG)req);
          req->ios2_Req.io_Flags &= ~IOF_QUICK;
          PutMsg(ud->ud_TaskPort, &req->ios2_Req.io_Message);
        break;

        case S2_BROADCAST:
          for (i = 0; i < 6; i++) req->ios2_DstAddr[i] = 0xFF;
        case CMD_WRITE:
          DBG1("CMD_WRITE [$%08lx].", (LONG)req);
          req->ios2_Req.io_Flags &= ~IOF_QUICK;
          PutMsg(ud->ud_TaskPort, &req->ios2_Req.io_Message);
        break;

        case CMD_FLUSH:
          DBG1("CMD_FLUSH [$%08lx].", (LONG)req);
          req->ios2_Req.io_Flags &= ~IOF_QUICK;
          PutMsg(ud->ud_TaskPort, &req->ios2_Req.io_Message);
        break;

        default:
          DBG1("DevBeginIo(): unknown command code %ld.", req->ios2_Req.io_Command);
          IoDone(ud, req, IOERR_NOCMD, S2WERR_GENERIC_ERROR);
        break;
      }
    return;

    AROS_LIBFUNC_EXIT
  }

///
/// DevAbortIO()

#ifdef __AROS__
AROS_LH1(ULONG, DevAbortIO,
   AROS_LHA(struct IOSana2Req *, req, A1),
   struct DevData *, dd, 6, S2)
#else
ULONG DevAbortIO(REG(a1, struct IOSana2Req *req),
  REG(a6, struct DevData *dd))
#endif
  {
    AROS_LIBFUNC_INIT

    USE(SysBase)
    USE_D(DOSBase)
    LONG ret = 0;
    struct UnitData *ud = (struct UnitData*)req->ios2_Req.io_Unit;
    struct MinList *list;
    struct MinNode *node;

    DBG1("DevAbortIo: aborting $%08lx.", req);
    switch (req->ios2_Req.io_Command)
      {
        case CMD_READ:
          list = &ud->ud_RxQueue;
        break;

        case CMD_WRITE:
        case S2_BROADCAST:
          list = &ud->ud_TxQueue;
        break;

        default:
          list = NULL;
      }
    if (list)
      {
        Disable();
        for (node = list->mlh_Head; node->mln_Succ; node = node->mln_Succ)
          {
            if (node == (struct MinNode*)req)
              {
                if (((struct Message*)node)->mn_Node.ln_Type != NT_REPLYMSG)
                  {
                    Remove((struct Node*)node);
                    req->ios2_Req.io_Error = IOERR_ABORTED;
                    ReplyMsg((struct Message*)node);
                    ret = 0;
                  }
              }
          }
        Enable();
      }
    else ret = IOERR_NOCMD;
    return ret;

    AROS_LIBFUNC_EXIT
  }

///

// AUXILIARY FUNCTIONS

/// IoDone(struct UnitData *ud, struct IOSana2Req *req, LONG err, LONG werr)
// Function ends IORequest with given error codes. Requests with IOF_QUICK cleared will be ReplyMsg()-ed.

void IoDone(struct UnitData *ud, struct IOSana2Req *req, LONG err, LONG werr)
  {
    USE_U(SysBase)

    req->ios2_Req.io_Error = err;
    req->ios2_WireError = werr;
    if (!(req->ios2_Req.io_Flags & IOF_QUICK))
      ReplyMsg(&req->ios2_Req.io_Message);
    return;
  }

///
/// OpenDeviceLibraries(struct DevData *dd)

LONG OpenDeviceLibraries(struct DevData *dd)
  {
    USE(SysBase)

    if (!(dd->dd_UtilityBase = OpenLibrary("utility.library", 39))) return FALSE;
    if (!(dd->dd_PrometheusBase = OpenLibrary("prometheus.library", 2))) return FALSE;
    if (!(dd->dd_DOSBase = OpenLibrary("dos.library", 38))) return FALSE;
    if (OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest*)&dd->dd_Treq, 0) == 0)
      {
        dd->dd_TimerBase = (struct Library*)dd->dd_Treq.tr_node.io_Device;
      }

    return TRUE;
  }

///
/// CloseDeviceLibraries(struct DevData *dd)

void CloseDeviceLibraries(struct DevData *dd)
  {
    USE(SysBase)

    if (dd->dd_DOSBase) CloseLibrary(dd->dd_DOSBase);
    if (dd->dd_PrometheusBase) CloseLibrary(dd->dd_PrometheusBase);
    if (dd->dd_UtilityBase) CloseLibrary(dd->dd_UtilityBase);
    if (dd->dd_TimerBase) CloseDevice ((struct IORequest*)&dd->dd_Treq);
  }

///
/// PrepareCookie(struct IOSana2Req *req, struct DevData *dd)

LONG PrepareCookie(struct IOSana2Req *req, struct DevData *dd)
  {
    USE(SysBase)
    USE(UtilityBase)
    USE_D(DOSBase)

    if (req->ios2_BufferManagement)
      {
        struct BuffFunctions *bfun;

        if (bfun = AllocMem(sizeof(struct BuffFunctions), MEMF_ANY))
          {
            bfun->bf_CopyFrom = (APTR)GetTagData(S2_CopyFromBuff, (IPTR)NULL,
              (struct TagItem*)req->ios2_BufferManagement);
            bfun->bf_CopyTo = (APTR)GetTagData(S2_CopyToBuff, (IPTR)NULL,
              (struct TagItem*)req->ios2_BufferManagement);

            if (bfun->bf_CopyFrom && bfun->bf_CopyTo)
              {
                DBG1("CopyFrom [$%08lx].", bfun->bf_CopyFrom);
                req->ios2_BufferManagement = bfun;
                return TRUE;
              }
            else FreeMem(bfun, sizeof(struct BuffFunctions));
          }
      }
    return FALSE;
  }

///
/// RunTask(struct DevData *dd, struct UnitData *ud)

LONG RunTask(struct DevData *dd, struct UnitData *ud)
  {
    USE(SysBase)
    USE(DOSBase)
    const struct TagItem task_tags[] =
    {
      {NP_Entry, (IPTR)UnitTask},
      {NP_Name, (IPTR)ud->ud_Name},
      {NP_Priority, 6},
      {TAG_END, 0}
    };

    DBG("RunTask() called.");

    if(ud->ud_LifeTime = CreateMsgPort())
      {
        if (ud->ud_Task = (struct Task*)CreateNewProc(task_tags))
          {
            WORD i;

            DBG1("Task [$%08lx] started.", ud->ud_Task);
            for (i = 0; i < 50; i++)
              {
                if (!(ud->ud_TaskPort = FindPort(ud->ud_Name))) Delay(1);
                else
                  {
                    DBG("Task port detected.");
                    ud->ud_Message.mn_Node.ln_Type = NT_MESSAGE;
                    ud->ud_Message.mn_Length = sizeof(struct UnitData);
                    ud->ud_Message.mn_ReplyPort = ud->ud_LifeTime;
                    PutMsg(ud->ud_TaskPort, &ud->ud_Message);
                    return TRUE;
                  }
              }
          }
      }
    return FALSE;
  }

///
/// KillTask(struct DevData *dd, struct UnitData *ud)

void KillTask(struct DevData *dd, struct UnitData *ud)
  {
    USE(SysBase)
    USE_D(DOSBase)

    Signal(ud->ud_Task, SIGBREAKF_CTRL_C);
    WaitPort(ud->ud_LifeTime);
    GetMsg(ud->ud_LifeTime);
    DeleteMsgPort(ud->ud_LifeTime);
    DBG("Task dead.");
    return;
  }

///
/// ClearGlobalStats()

void ClearGlobalStats(struct UnitData *ud)
  {
    USE_U(TimerBase)

    ud->ud_DevStats.PacketsReceived = 0;
    ud->ud_DevStats.PacketsSent = 0;
    GetSysTime(&ud->ud_DevStats.LastStart);
    return;
  }

///
/// OpenUnit()

struct UnitData *OpenUnit(struct DevData *dd, LONG unit, LONG flags)
  {
    USE_D(DOSBase)
    struct UnitData *ud = dd->dd_Units[unit];

    DBG("OpenUnit() called.");

    /* Eliminate 'promiscuous without exclusive' flag combination. */

    if ((flags & SANA2OPF_PROM) && !(flags & SANA2OPF_MINE)) return NULL;

    /* Initialize unit if opened first time. */

    if (!ud)
      {
        if (!(ud = InitializeUnit(dd, unit))) return NULL;
      }

    /* Check exclusive flag - reject if already opened by someone else. */

    if ((flags & SANA2OPF_MINE) && ud->ud_OpenCnt) return NULL;

    /* Set promiscuous flag if requested - we konw here MINE was requested too, and noone else has opened */
    /* the unit. So we can just set it if requested. */

    if (flags & SANA2OPF_PROM) ud->ud_Flags |= UF_PROMISCUOUS;

    /* OK, increment open counter and exit with success. */

    ud->ud_OpenCnt++;
    DBG2("%s opened [%ld].", ud->ud_Name, ud->ud_OpenCnt);
    return ud;
  }

///
/// CloseUnit()

void CloseUnit(struct DevData *dd, struct UnitData *ud)
  {
    USE_D(DOSBase)

    DBG1("%s closed.", ud->ud_Name);
    if (!(--ud->ud_OpenCnt)) ExpungeUnit(dd, ud);
    return;
  }

///
/// InitializeUnit()

struct UnitData *InitializeUnit(struct DevData *dd, LONG unit)
  {
    USE(SysBase)
    #ifdef PDEBUG
    USE(DOSBase)
    #endif
    struct UnitData *ud;
    WORD i;

    DBG("InitializeUnit() called.");
    if (ud = AllocMem(sizeof(struct UnitData), MEMF_PUBLIC | MEMF_CLEAR))
      {
        if (ud->ud_Hardware = FindHardware(dd, unit, ud))
          {
            #ifdef PDEBUG
            ud->debug = dd->debug;
            #endif

            for (i = 5; i >= 0; i--)
              {
                ud->ud_SoftAddress[i] = 0x00;
                ud->ud_EtherAddress[i] = 0x00;
              }
            ud->ud_SysBase = dd->dd_SysBase;
            ud->ud_PrometheusBase = dd->dd_PrometheusBase;
            ud->ud_DOSBase = dd->dd_DOSBase;
            ud->ud_TimerBase = dd->dd_TimerBase;
            strcpy(ud->ud_Name, "prm-rtl8029.device (x)");
            ud->ud_Name[20] = '0' + unit;
            ud->ud_RxQueue.mlh_Head = (struct MinNode*)&ud->ud_RxQueue.mlh_Tail;
            ud->ud_RxQueue.mlh_Tail = NULL;
            ud->ud_RxQueue.mlh_TailPred = (struct MinNode*)&ud->ud_RxQueue.mlh_Head;
            ud->ud_TxQueue.mlh_Head = (struct MinNode*)&ud->ud_TxQueue.mlh_Tail;
            ud->ud_TxQueue.mlh_Tail = NULL;
            ud->ud_TxQueue.mlh_TailPred = (struct MinNode*)&ud->ud_TxQueue.mlh_Head;
            ud->ud_NextPage = RX_BUFFER;
            HardwareReset(ud);
            HardwareInit(ud);
            GetHwAddress(ud);
            if (RunTask(dd, ud))
              {
                dd->dd_Units[unit] = ud;
                DBG1("%s initialized.", (LONG)ud->ud_Name);
                return ud;
              }
          }
      }
    ExpungeUnit(dd, ud);
    return NULL;
  }

///
/// ExpungeUnit()

void ExpungeUnit(struct DevData *dd, struct UnitData *ud)
  {
    USE(SysBase)
    USE_D(DOSBase)
    WORD unit;

    if (ud)
      {
       unit = ud->ud_Name[20] - '0';
       if (ud->ud_Flags & UF_ONLINE) GoOffline(ud);
       if (ud->ud_Task) KillTask(dd, ud);
       FreeMem(ud, sizeof(struct UnitData));
       dd->dd_Units[unit] = NULL;
       DBG1("%s expunged.", ud->ud_Name);
      }
    return;
  }

///

// IMMEDIATE DEVICE COMMANDS

/// S2DeviceQuery()

void S2DeviceQuery(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_UD(DOSBase)
    struct Sana2DeviceQuery *query = req->ios2_StatData;

    DBG_U("S2_DEVICEQUERY.");
    if (query)
      {
        if (query->SizeAvailable >= sizeof(struct Sana2DeviceQuery))
          {
            query->SizeSupplied = sizeof(struct Sana2DeviceQuery);
            query->DevQueryFormat = 0;
            query->DeviceLevel = 0;
            query->AddrFieldSize = 48;
            query->MTU = 1500;
            query->BPS = 10000000;
            query->HardwareType = S2WireType_Ethernet;
            IoDone(ud, req, OK, OK);
          }
        else IoDone(ud, req, S2ERR_BAD_ARGUMENT, S2WERR_GENERIC_ERROR);
      }
    else IoDone(ud, req, S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    return;
  }

///
/// S2GetStationAddress()

void S2GetStationAddress(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_U(SysBase)
    USE_UD(DOSBase)

    DBG_U("S2_GETSTATIONADDRESS.");
    CopyMem(ud->ud_SoftAddress, req->ios2_SrcAddr, 6);
    CopyMem(ud->ud_EtherAddress, req->ios2_DstAddr, 6);
    IoDone(ud, req, OK, OK);
    return;
  }

///
/// S2Online()

void S2Online(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_UD(DOSBase)

    DBG_U("S2_ONLINE.");
    ClearGlobalStats(ud);
    if (!(ud->ud_Flags & UF_ONLINE))
      {
        GoOnline(ud);
        ud->ud_Flags |= UF_ONLINE;
        IoDone(ud, req, OK, OK);
      }
    else IoDone(ud, req, S2ERR_BAD_STATE, S2WERR_UNIT_ONLINE);
    return;
  }

///
/// S2ConfigInterface()

BOOL address_has_all(UBYTE *addr, UBYTE num)
  {
    WORD i;

    for (i = 5; i >= 0; i--)
      {
        if (addr[i] != num) return FALSE;
      }
    return TRUE;
  }


void S2ConfigInterface(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_U(SysBase)
    USE_UD(DOSBase)

    DBG_U("S2_CONFIGINTERFACE.");
    ClearGlobalStats(ud);
    if (ud->ud_Flags & UF_CONFIGURED)
      {
        IoDone(ud, req, S2ERR_BAD_STATE, S2WERR_IS_CONFIGURED);
      }
    else if (address_has_all(req->ios2_SrcAddr, 0x00) ||
      address_has_all(req->ios2_SrcAddr, 0xFF))
      {
        IoDone(ud, req, S2ERR_BAD_ADDRESS, S2WERR_SRC_ADDRESS);
      }
    else
      {
        HardwareInit(ud);
        CopyMem(req->ios2_SrcAddr, ud->ud_SoftAddress, 6);
        WriteHwAddress(ud);
        if (!(ud->ud_Flags & UF_ONLINE))
          {
            GoOnline(ud);
            ud->ud_Flags |= UF_ONLINE;
          }
        ud->ud_Flags |= UF_CONFIGURED;
        IoDone(ud, req, OK, OK);
      }
    return;
 }

///
/// S2Offline()

void S2Offline(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_UD(DOSBase)

    DBG_U("S2_OFFLINE.");
    if (ud->ud_Flags & UF_ONLINE)
      {
        GoOffline(ud);
        ud->ud_Flags &= ~UF_ONLINE;
        IoDone(ud, req, OK, OK);
      }
    else IoDone(ud, req, S2ERR_BAD_STATE, S2WERR_UNIT_OFFLINE);
    return;
  }

///
/// S2GetGlobalStats()

void S2GetGlobalStats(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_U(SysBase)

    if (req->ios2_StatData)
      {
        CopyMem(&ud->ud_DevStats, req->ios2_StatData, sizeof(struct Sana2DeviceStats));
        IoDone(ud, req, OK, OK);
      }
    else IoDone(ud, req, S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
  }

///
/// CmdNSDQuery()

void CmdNSDQuery(struct UnitData *ud, struct IOStdReq *req)
  {
    USE_U(SysBase)
    USE_UD(DOSBase)
    struct NSDeviceQueryResult *qdata;
    LONG error = OK;

    DBG_U("NSCMD_DEVICEQUERY.");
    if (req->io_Length >= sizeof(struct NSDeviceQueryResult))
      {
        if (qdata = (struct NSDeviceQueryResult*)req->io_Data)
          {
            if ((qdata->DevQueryFormat == 0) && (qdata->SizeAvailable == 0))
              {
                qdata->SizeAvailable = sizeof(struct NSDeviceQueryResult);
                qdata->DeviceType = NSDEVTYPE_SANA2;
                qdata->DeviceSubType = 0;
                qdata->SupportedCommands = NSDSupported;
                req->io_Actual = sizeof(struct NSDeviceQueryResult);
              }
            else error = IOERR_BADLENGTH;
          }
        else error = IOERR_BADADDRESS;
      }
    else error = IOERR_BADLENGTH;

    /* I don't use IoDone() here, because it writes to ios2_WireError */
    /* but this request can be simple IOStdReq one.                   */

    req->io_Error = error;
    if (!(req->io_Flags & IOF_QUICK)) ReplyMsg(&req->io_Message);
    return;
  }

///

// HARDWARE ACCESS FUNCTIONS

/// IntCode()

#ifdef __AROS__
AROS_INTH1(IntCode, struct UnitData *, ud)
#else
LONG IntCode(REG(a1, struct UnitData *ud))
#endif
  {
    AROS_INTFUNC_INIT

    USE_U(SysBase)
    UBYTE intstatus;
    struct IOSana2Req *req;
    LONG my_int = 0;

    while (intstatus = (BYTEIN(ud->ud_Hardware + NE2000_INT_STATUS)
      & INTMASK))
      {
        if (intstatus & INT_TXERROR)
          {
            BYTEOUT(ud->ud_Hardware + NE2000_INT_STATUS, INT_TXERROR);
            req = ud->ud_PendingWrite;
            IoDone(ud, req, S2ERR_TX_FAILURE, S2WERR_TOO_MANY_RETIRES);
            Signal(ud->ud_Task, ud->ud_GoWriteMask);
            my_int = 1;
          }

        if (intstatus & INT_TXPACKET)
          {
            BYTEOUT(ud->ud_Hardware + NE2000_INT_STATUS, INT_TXPACKET);
            req = ud->ud_PendingWrite;
            ud->ud_DevStats.PacketsSent++;
            IoDone(ud, req, OK, OK);
            Signal(ud->ud_Task, ud->ud_GoWriteMask);
            my_int = 1;
          }

        if (intstatus & INT_RXPACKET)
          {
            ULONG offset, len;
            UWORD mask = 0xFFFF;
            WORD i;

            while (RingBufferNotEmpty(ud))
              {
                BYTEOUT(ud->ud_Hardware + NE2000_INT_STATUS, INT_RXPACKET);
                len = PacketReceived(ud);
                if (req = SearchReadRequest(ud, &ud->ud_RxQueue,
                  BEWord(ud->ud_RxBuffer[6])))
                  {
                    if (req->ios2_Req.io_Flags & SANA2IOF_RAW) offset = 0;
                    else offset = 7;
#ifdef __AROS__
                    AROS_UFC3(BOOL, ((struct BuffFunctions*)
                      req->ios2_BufferManagement)->bf_CopyTo,
                      AROS_UFCA(APTR, req->ios2_Data, A0),
                      AROS_UFCA(APTR, &ud->ud_RxBuffer[offset], A1),
                      AROS_UFCA(ULONG, len - (offset << 1), D0));
#else
                    ((struct BuffFunctions*)req->ios2_BufferManagement)->
                      bf_CopyTo(req->ios2_Data, &ud->ud_RxBuffer[offset],
                      len - (offset << 1));
#endif
                    CopyMem(&ud->ud_RxBuffer[0], req->ios2_DstAddr, 6);
                    CopyMem(&ud->ud_RxBuffer[3], req->ios2_SrcAddr, 6);
                    req->ios2_DataLength = len - (offset << 1);
                    for (i = 2; i >= 0; i--) mask &= ud->ud_RxBuffer[i];
                    if (mask == 0xFFFF)
                      req->ios2_Req.io_Flags |= SANA2IOF_BCAST;
                    ud->ud_DevStats.PacketsReceived++;
                    IoDone(ud, req, OK, OK);
                  }
              }
            my_int = 1;
          }
      }
    return my_int;

    AROS_INTFUNC_EXIT
  }

///
/// InstallInterrupt()

LONG InstallInterrupt(struct UnitData *ud)
  {
    USE_U(SysBase)
    USE_U(PrometheusBase)
    struct Interrupt *intr;

    if (intr = AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR))
      {
        intr->is_Node.ln_Type = NT_INTERRUPT;
        intr->is_Node.ln_Name = ud->ud_Name;
        intr->is_Data = ud;
        intr->is_Code = (APTR)IntCode;
        Prm_AddIntServer(ud->ud_Board, intr);
        ud->ud_Interrupt = intr;
        return TRUE;
      }
    return FALSE;
  }

///
/// RemoveInterrupt()

void RemoveInterrupt(struct UnitData *ud)
  {
    USE_U(SysBase)
    USE_U(PrometheusBase)

    Prm_RemIntServer(ud->ud_Board, ud->ud_Interrupt);
    FreeMem(ud->ud_Interrupt, sizeof(struct Interrupt));
    return;
  }

///
/// FindHardware()

IPTR FindHardware(struct DevData *dd, WORD unit, struct UnitData *ud)
  {
    USE(PrometheusBase)
    WORD u = unit;
    APTR board = NULL;
    IPTR hwbase;

    while (u-- >= 0)
      {
        board = Prm_FindBoardTags(board,
          PRM_Vendor, PCI_VENDOR_REALTEK,
          PRM_Device, PCI_DEVICE_RTL8029,
        TAG_END);
        if (!board) break;
      }

    if (board)
      {
        ud->ud_Board = board;
        Prm_GetBoardAttrsTags(board,
          PRM_MemoryAddr0, (IPTR)&hwbase,
        TAG_END);
        Prm_SetBoardAttrsTags(board,
          PRM_BoardOwner, (IPTR)dd,
        TAG_END);
        return hwbase;
      }
    return 0;
  }

///
/// HardwareInit()

void HardwareInit (struct UnitData *ud)
  {
    IPTR hw = ud->ud_Hardware;

    BYTEOUT(hw + NE2000_COMMAND, 0x21);
    BYTEOUT(hw + NE2000_DATA_CONFIG,
      DTCFG_FIFO_8 | DTCFG_WIDE | DTCFG_LOOPSEL);
    BYTEOUT(hw + NE2000_DMA_COUNTER0, 0);
    BYTEOUT(hw + NE2000_DMA_COUNTER1, 0);
    BYTEOUT(hw + NE2000_TX_CONFIG, 0x02);
    BYTEOUT(hw + NE2000_PAGE_START, RX_BUFFER);
    BYTEOUT(hw + NE2000_PAGE_STOP, BUFFER_END);
    BYTEOUT(hw + NE2000_BOUNDARY, BUFFER_END - 1);
    BYTEOUT(hw + NE2000_INT_STATUS, 0xFF);
    BYTEOUT(hw + NE2000_INT_MASK, 0x00);
    BYTEOUT(hw + NE2000_COMMAND, 0x61);
    BYTEOUT(hw + NE2000_CURRENT_PAGE, RX_BUFFER);
    BYTEOUT(hw + NE2000_COMMAND, 0x21);
    return;
  }

///
/// HardwareReset()

void HardwareReset (struct UnitData *ud)
  {
    USE_U(DOSBase)
    IPTR hw = ud->ud_Hardware;
    UBYTE trash;

    WORDOUT(hw + NE2000_RESET_PORT, trash);
    Delay(1);
    trash = WORDIN(hw + NE2000_RESET_PORT);
    HardwareInit(ud);
    return;
  }

///
/// GoOnline()

void GoOnline (struct UnitData *ud)
  {
    IPTR hw = ud->ud_Hardware;

    HardwareReset(ud);
    WriteHwAddress(ud);
    InstallInterrupt(ud);
    BYTEOUT(hw + NE2000_COMMAND, 0x22);
    BYTEOUT(hw + NE2000_TX_CONFIG, 0x00);
    BYTEOUT(hw + NE2000_RX_CONFIG, RXCFG_BCAST | RXCFG_MCAST |
       ((ud->ud_Flags & UF_PROMISCUOUS) ? RXCFG_PROM : 0));
    BYTEOUT(hw + NE2000_INT_STATUS, 0xFF);
    BYTEOUT(hw + NE2000_INT_MASK, INTMASK);

    return;
  }

///
/// GoOffline()

void GoOffline(struct UnitData *ud)
  {
    IPTR hw = ud->ud_Hardware;

    BYTEOUT(hw + NE2000_COMMAND, 0x21);
    BYTEOUT(hw + NE2000_TX_CONFIG, 0x02);
    BYTEOUT(hw + NE2000_RX_CONFIG, 0x20);
    BYTEOUT(hw + NE2000_INT_STATUS, 0xFF);
    BYTEOUT(hw + NE2000_INT_MASK, 0);
    RemoveInterrupt(ud);
    return;
  }

///
/// BoardShutdown()

void BoardShutdown(struct UnitData *ud)
  {
    IPTR hw = ud->ud_Hardware;

    GoOffline(ud);
    BYTEOUT(hw + NE2000_INT_MASK, 0);
    BYTEOUT(hw + NE2000_INT_STATUS, 0xFF);
    return;
  }

///
/// GetPacketHeader()

ULONG GetPacketHeader(IPTR ne, UBYTE page)
  {
    ULONG hdr;

    BYTEOUT(ne + NE2000_DMA_COUNTER0, 4);
    BYTEOUT(ne + NE2000_DMA_COUNTER1, 0);
    BYTEOUT(ne + NE2000_DMA_START_ADDR0, 0);
    BYTEOUT(ne + NE2000_DMA_START_ADDR1, page);
    BYTEOUT(ne + NE2000_COMMAND,
      COMMAND_PAGE0 | COMMAND_START | COMMAND_READ);
    hdr = LEWORDIN(ne + NE2000_DMA_PORT);
    hdr |= LEWORDIN(ne + NE2000_DMA_PORT) << 16;
    return hdr;
  }

///
/// GetPacket()

/*GetPacket(volatile struct Ne2000 *ne, UBYTE startpage, UWORD len, UWORD *buffer)*/
VOID GetPacket(IPTR ne, UBYTE startpage, UWORD len, UWORD *buffer)
  {
    UWORD count;

    BYTEOUT(ne + NE2000_DMA_COUNTER0, len & 0xFF);
    BYTEOUT(ne + NE2000_DMA_COUNTER1, len >> 8);
    BYTEOUT(ne + NE2000_DMA_START_ADDR0, 4);
    BYTEOUT(ne + NE2000_DMA_START_ADDR1, startpage);
    BYTEOUT(ne + NE2000_COMMAND, COMMAND_PAGE0 | COMMAND_START | COMMAND_READ);

    for (count = (len + 1) >> 1; count; count--)
      {
        *buffer++ = WORDIN(ne + NE2000_DMA_PORT);
      }
    return;
  }

///
/// PacketReceived()

LONG PacketReceived(struct UnitData *ud)
  {
    ULONG header, len;

    header = GetPacketHeader(ud->ud_Hardware, ud->ud_NextPage);
    len = header >> 16;
    GetPacket(ud->ud_Hardware, ud->ud_NextPage, len, (UWORD*)ud->ud_RxBuffer);
    BYTEOUT(ud->ud_Hardware + NE2000_BOUNDARY, ud->ud_NextPage);
    ud->ud_NextPage = (header >> 8) & 0xFF;
    BYTEOUT(ud->ud_Hardware + NE2000_INT_STATUS, INT_RXPACKET);
    return len;
  }

///
/// BufferOverflow()

void BufferOverflow(struct UnitData *ud)
  {
    struct Library *DOSBase = ud->ud_DOSBase;
    IPTR hw = ud->ud_Hardware;
    UBYTE txp, resent = FALSE, intstatus;

    txp = BYTEIN(hw + NE2000_COMMAND) & COMMAND_TXP;
    BYTEOUT(hw + NE2000_COMMAND, COMMAND_PAGE0 | COMMAND_ABORT | COMMAND_STOP);
    Delay(1);
    BYTEOUT(hw + NE2000_DMA_COUNTER0, 0);
    BYTEOUT(hw + NE2000_DMA_COUNTER1, 0);

    if (txp)
      {
        intstatus = BYTEIN(hw + NE2000_INT_STATUS);
        if (!(intstatus & (INT_TXPACKET | INT_TXERROR))) resent = TRUE;
      }

    BYTEOUT(hw + NE2000_TX_CONFIG, TXCFG_LOOP_INT);
    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE1 | COMMAND_ABORT | COMMAND_START);
    BYTEOUT(hw + NE2000_CURRENT_PAGE, RX_BUFFER);
    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE0 | COMMAND_ABORT | COMMAND_START);
    BYTEOUT(hw + NE2000_BOUNDARY, BUFFER_END - 1);
    ud->ud_NextPage = RX_BUFFER;

    BYTEOUT(hw + NE2000_TX_CONFIG, TXCFG_LOOP_NONE);
    if (resent) BYTEOUT(hw + NE2000_COMMAND, COMMAND_PAGE0 | COMMAND_START |
      COMMAND_ABORT | COMMAND_TXP);
    BYTEOUT(hw + NE2000_INT_STATUS, INT_OVERFLOW);

    return;
  }

///
/// SendPacket()

void SendPacket(struct UnitData *ud, struct IOSana2Req *req)
  {
    USE_U(SysBase)
    USE_UD(DOSBase)
    IPTR hw = ud->ud_Hardware;
    UBYTE ethbuffer[1536], *datapointer;
    UWORD *ethdata = (UWORD*)ethbuffer;
    ULONG data_len = req->ios2_DataLength;
    UWORD cycles;

    /* If not raw packets, fill in Dst, Src and Type fields of Ethernet frame. 'datapointer' is a variable */
    /* holding address of data to copy from network stack. If packet is raw, datapointer points to start   */
    /* of ethbuffer, otherwise points to ef_Data field (first byte after Ethernet header.                  */

    if (!(req->ios2_Req.io_Flags & SANA2IOF_RAW))
      {
        struct EthFrame *ef = (struct EthFrame*)ethbuffer;

        CopyMem(req->ios2_DstAddr, ef->ef_DestAddr, 6);
        CopyMem(ud->ud_EtherAddress, ef->ef_SrcAddr, 6);
        ef->ef_Type = MakeBEWord(req->ios2_PacketType);
        datapointer = ef->ef_Data;
      }
    else datapointer = ethbuffer;

    /* Copy data from network stack using supplied CopyFrom() function. */

#ifdef __AROS__
    AROS_UFC3(BOOL, ((struct BuffFunctions*)
      req->ios2_BufferManagement)->bf_CopyFrom,
      AROS_UFCA(APTR, datapointer, A0),
      AROS_UFCA(APTR, req->ios2_Data, A1),
      AROS_UFCA(ULONG, data_len, D0));
#else
    ((struct BuffFunctions*)req->ios2_BufferManagement)->bf_CopyFrom(
      datapointer, req->ios2_Data, data_len);
#endif

    /* Now we need length of data to send to hardware. IORequest ios2_DataLength does not include header   */
    /* length if packet is not RAW. So we should add it.                                                   */

    if (!(req->ios2_Req.io_Flags & SANA2IOF_RAW)) data_len += 14;

    /* Packet sent to Ethernet hardware should be at least 60 bytes long (4 bytes of CRC will be appended  */
    /* by hardware giving 64 bytes). If our packet is shorter we should extend it with spaces.             */

    while (data_len < 60) ethbuffer[data_len++] = ' ';

    /* Now the packet is ready to send it to NIC buffer. It is done by Remote Write DMA command. Firstly   */
    /* write address and counter should be initialized, then command register.                              */

    Disable();
    BYTEOUT(hw + NE2000_DMA_COUNTER0, data_len & 0xFF);
    BYTEOUT(hw + NE2000_DMA_COUNTER1, data_len >> 8);
    BYTEOUT(hw + NE2000_DMA_START_ADDR0, 0);
    BYTEOUT(hw + NE2000_DMA_START_ADDR1, TX_BUFFER);
    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE0 | COMMAND_START | COMMAND_WRITE);

    /* Now we can send packet data to DMAPort word by word. */

    for (cycles = (data_len + 1) >> 1; cycles; cycles--)
      WORDOUT(hw + NE2000_DMA_PORT, *ethdata++);

    /* Send packet to the wire. Register setup first. */

    BYTEOUT(hw + NE2000_TX_PAGE_START, TX_BUFFER);
    BYTEOUT(hw + NE2000_TX_COUNTER0, data_len & 0xFF);
    BYTEOUT(hw + NE2000_TX_COUNTER1, data_len >> 8);

    /* Three, two, one, go! */

    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE0 | COMMAND_START | COMMAND_ABORT | COMMAND_TXP);
    Enable();

    /* OK. Packet was sent (successfully or not). Hardware will respond with TXPACKET or TXERROR interrupt  */
    /* then we will be able to reply IORequest in the interrupt server.                                    */

    DBG_T("Packet sent.");
    return;
  }

///
/// GetHwAddress()

void GetHwAddress(struct UnitData *ud)
  {
    USE_UD(DOSBase)
    IPTR hw = ud->ud_Hardware;
    WORD i;

    BYTEOUT(hw + NE2000_DMA_COUNTER0, 6);
    BYTEOUT(hw + NE2000_DMA_COUNTER1, 0);
    BYTEOUT(hw + NE2000_DMA_START_ADDR0, 0);
    BYTEOUT(hw + NE2000_DMA_START_ADDR1, 0);
    BYTEOUT(hw + NE2000_COMMAND, COMMAND_READ);
    for (i = 0; i < 6; i++)
      ud->ud_EtherAddress[i] = WORDIN(hw + NE2000_DMA_PORT);

    #ifdef PDEBUG
      DBG_U("\thardware address (read):");
      for (i = 0; i < 6; i++) DBG1_U("\t%02lx", ud->ud_EtherAddress[i]);
    #endif

    return;
  }

///
/// WriteHwAddress()

void WriteHwAddress(struct UnitData *ud)
  {
    USE_UD(DOSBase)
    IPTR hw = ud->ud_Hardware;
    WORD i;

    #ifdef PDEBUG
      DBG_U("\thardware address (write):");
      for (i = 0; i < 6; i++) DBG1_U("\t%02lx", ud->ud_SoftAddress[i]);
    #endif

    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE1 | COMMAND_ABORT | COMMAND_STOP);
    for (i = 0; i < 6; i++)
      {
        BYTEOUT(hw + NE2000_PHYSICAL_ADDR0 + i, ud->ud_SoftAddress[i]);
      }
    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE0 | COMMAND_ABORT | COMMAND_STOP);
    return;
  }

///
/// RingBufferNotEmpty()

LONG RingBufferNotEmpty(struct UnitData *ud)
  {
    UBYTE current;
    IPTR hw = ud->ud_Hardware;

    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE1 | COMMAND_ABORT | COMMAND_START);
    current = BYTEIN(hw + NE2000_CURRENT_PAGE);
    BYTEOUT(hw + NE2000_COMMAND,
      COMMAND_PAGE0 | COMMAND_ABORT | COMMAND_START);

    if (ud->ud_NextPage == current) return FALSE;
    else return TRUE;
  }

///

// SUBTASK CODE

/// SearchReadRequest()

struct IOSana2Req *SearchReadRequest(struct UnitData *ud,
  struct MinList *queue, ULONG type)
  {
    struct Library *SysBase = ud->ud_SysBase;
    struct IOSana2Req *req, *found = NULL /*, *x*/ ;

    for (req = (struct IOSana2Req*)queue->mlh_Head;
      req->ios2_Req.io_Message.mn_Node.ln_Succ;
      req = (struct IOSana2Req*)req->ios2_Req.io_Message.mn_Node.ln_Succ)
      {
        if (req->ios2_PacketType == type)
          {
            Remove((struct Node*)req);
            found = req;
            break;
          }
      }
    return found;
  }

///
/// FlushQueues()

void FlushQueues(struct UnitData *ud)
  {
    USE_U(SysBase)
    USE_UD(DOSBase)
    struct IOSana2Req *xreq;

    for (;;)
      {
        Disable();
        xreq = (struct IOSana2Req*)RemHead((struct List*)&ud->ud_RxQueue);
        Enable();
        if (!xreq) break;
        DBG1_T("<- READ [$08%lx] [F].", xreq);
        IoDone(ud, xreq, IOERR_ABORTED, S2WERR_GENERIC_ERROR);
      }

    for (;;)
      {
        Disable();
        xreq = (struct IOSana2Req*)RemHead((struct List*)&ud->ud_TxQueue);
        Enable();
        if (!xreq) break;
        DBG1_T("<- WRITE [$08%lx] [F].", xreq);
        IoDone(ud, xreq, IOERR_ABORTED, S2WERR_GENERIC_ERROR);
      }
    return;
  }

///
/// MainLoop()

void MainLoop(struct UnitData *ud, struct MsgPort *port)
  {
    USE_U(SysBase)
    USE_UD(DOSBase)
    ULONG signals, sigmask;
    struct IOSana2Req *req;

    #ifdef PDEBUG
    UBYTE wname[60], *ptr;

    ptr = strcpy(wname, "CON:400/17/400/300/");
    ptr = strcpy(ptr, ud->ud_Name);
    strcpy(ptr,"/AUTO/CLOSE/WAIT");
    ud->tdebug = Open(wname, MODE_NEWFILE);
    #endif

    sigmask = 1 << port->mp_SigBit;
    for (;;)
      {
        DBG_T("Waiting...");
        signals = Wait(SIGBREAKF_CTRL_C | sigmask | ud->ud_GoWriteMask);

        if (signals & ud->ud_GoWriteMask)
          {
            DBG_T("GoWrite");
            Disable();
            req = (struct IOSana2Req*)RemHead((struct List*)&ud->ud_TxQueue);
            Enable();
            ud->ud_PendingWrite = req;
            if (req) SendPacket(ud, req);
          }

        if (signals & SIGBREAKF_CTRL_C)
          {
            DBG1_T("TASK: %s task killed.", ud->ud_Name);
            return;
          }

        if (signals & sigmask)
          {
            struct IOSana2Req *req;

            DBG_T("port");
            while (req = (struct IOSana2Req*)GetMsg(port))
              {
                switch (req->ios2_Req.io_Command)
                  {
                    case CMD_READ:
                      DBG1_T("-> READ [$%08lx].", req);
                      Disable();
                      AddTail((struct List*)&ud->ud_RxQueue, (struct Node*)req);
                      Enable();
                    break;

                    case S2_BROADCAST:
                    case CMD_WRITE:
                      DBG1_T("-> WRITE [$%08lx].", req);
                      if (ud->ud_PendingWrite)
                        {
                          Disable();
                          AddTail((struct List*)&ud->ud_TxQueue, (struct Node*)req);
                          Enable();
                        }
                      else
                        {
                          ud->ud_PendingWrite = req;
                          SendPacket(ud, req);
                          DBG_T("Packet sent 2.");
                        }
                    break;

                    case CMD_FLUSH:
                      DBG1_T("-> FLUSH [$%08lx].", req);
                      FlushQueues(ud);
                      DBG1_T("<- FLUSH [$%08lx].", req);
                      IoDone(ud, req, OK, OK);
                    break;

                    default:
                      DBG2_T("-> Unknown ($%lx) [$%08lx].", req->ios2_Req.io_Command, req);
                  }
              }
          }
      }
  }

///
/// UnitTask()

void UnitTask(void)
  {
    struct Library *SysBase;
    struct Task *task;
    struct MsgPort *port;
    struct UnitData *ud;

    #ifdef PDEBUG
    struct Library *DOSBase;
    #endif

#ifdef __AROS__
    SysBase = sys_base;
#else
    SysBase = *(struct Library**)4;
#endif
    task = FindTask(NULL);
    if (port = CreateMsgPort())
      {
        port->mp_Node.ln_Name = task->tc_Node.ln_Name;
        port->mp_Node.ln_Pri = 20;
        AddPort(port);
        WaitPort(port);
        RemPort(port);
        ud = (struct UnitData*)GetMsg(port);
        if ((ud->ud_GoWriteBit = AllocSignal(-1)) != -1)
          {
            ud->ud_GoWriteMask = 1 << ud->ud_GoWriteBit;
            MainLoop(ud, port);
            FreeSignal(ud->ud_GoWriteBit);
          }
        DeleteMsgPort(port);
        Forbid();
        ReplyMsg(&ud->ud_Message);
      }
    return;
  }

///

