#ifndef MASSSTORAGE_CLASS_H
#define MASSSTORAGE_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for MS class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <libraries/expansion.h>
#include <libraries/configregs.h>
#include <libraries/configvars.h>
#include <libraries/asl.h>

#include <dos/dostags.h>
#include <scsi/commands.h>
#include <scsi/values.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <resources/filesysres.h>

#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <devices/usb_massstorage.h>
#include <libraries/usbclass.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "massstorage.h"
#include "dev.h"

/* TRUE for "device sent more data than requested". UHCI/OHCI/EHCI HCDs report
   this as UHIOERR_OVERFLOW; xhci folds the same wire condition into Babble
   Detected (UHIOERR_BABBLE). The transports treat both as benign truncation. */
static inline BOOL nIsOverflowErr(LONG ioerr)
{
    return (ioerr == UHIOERR_OVERFLOW) || (ioerr == UHIOERR_BABBLE);
}

#ifndef TD_READ64
#define TD_READ64     24
#define TD_WRITE64    25
#define TD_SEEK64     26
#define TD_FORMAT64   27
#endif

/* Protos */

struct NepClassMS * GM_UNIQUENAME(usbAttemptInterfaceBinding)(struct NepMSBase *nh, struct PsdInterface *pif);
struct NepClassMS * GM_UNIQUENAME(usbForceInterfaceBinding)(struct NepMSBase *nh, struct PsdInterface *pif);
void GM_UNIQUENAME(usbReleaseInterfaceBinding)(struct NepMSBase *nh, struct NepClassMS *ncm);

struct NepClassMS * GM_UNIQUENAME(nAllocMS)(void);
void GM_UNIQUENAME(nFreeMS)(struct NepClassMS *ncm);

BOOL GM_UNIQUENAME(nLoadClassConfig)(struct NepMSBase *nh);
BOOL GM_UNIQUENAME(nLoadBindingConfig)(struct NepClassMS *ncm, BOOL announce);
LONG GM_UNIQUENAME(nOpenBindingCfgWindow)(struct NepMSBase *nh, struct NepClassMS *ncm);

void GM_UNIQUENAME(nGUITaskCleanup)(struct NepClassMS *ncm);
BOOL GM_UNIQUENAME(nStoreConfig)(struct NepClassMS *ncm);
BOOL nTryFallback(struct Library *ps, struct NepClassMS *ncm, ULONG flag, CONST_STRPTR name, BOOL enable, LONG rc);
BOOL nApplyFallback(struct NepClassMS *ncm, ULONG flag, CONST_STRPTR name, BOOL enable, LONG rc);

void nSetNakTimeout(struct NepClassMS *ncm, struct PsdPipe *pp, ULONG timeout_ms);
ULONG nNakTimeoutMs(struct NepClassMS *ncm);
void nApplyNakTimeout(struct NepClassMS *ncm, ULONG timeout_ms);
void nWakeUnitTask(struct NepClassMS *ncm);

void GM_UNIQUENAME(nHexString)(UBYTE *src, ULONG len, UBYTE *buf);

LONG nScsiDirect(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nScsiDirectBulk(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nScsiDirectCBI(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nScsiDirectUAS(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nBulkReset(struct NepClassMS *ncm);
LONG nBulkClear(struct NepClassMS *ncm);
void nLockXFer(struct NepClassMS *ncm);
void nUnlockXFer(struct NepClassMS *ncm);
LONG nRead64(struct NepClassMS *ncm, struct IOStdReq *ioreq);
LONG nWrite64(struct NepClassMS *ncm, struct IOStdReq *ioreq);
LONG nFormat64(struct NepClassMS *ncm, struct IOStdReq *ioreq);
LONG nSeek64(struct NepClassMS *ncm, struct IOStdReq *ioreq);
LONG nGetGeometry(struct NepClassMS *ncm, struct IOStdReq *ioreq);
LONG nGetWriteProtect(struct NepClassMS *ncm);
LONG nStartStop(struct NepClassMS *ncm, struct IOStdReq *ioreq);

BOOL nStartRemovableTask(struct Library *ps, struct NepMSBase *nh);
struct NepMSBase * GM_UNIQUENAME(nAllocRT)(void);
void GM_UNIQUENAME(nFreeRT)(struct NepMSBase *nh);
void nUnmountPartition(struct NepClassMS *ncm);
LONG nIOCmdTunnel(struct NepClassMS *ncm, struct IOStdReq *ioreq);
LONG nScsiDirectTunnel(struct NepClassMS *ncm, struct SCSICmd *scsicmd);

BPTR CreateSegment(struct NepClassMS *ncm, const ULONG *MyData);
struct DeviceNode * FindMatchingDevice(struct NepClassMS *ncm, struct DosEnvec *envec);
BOOL CheckFATPartition(struct NepClassMS *ncm, ULONG startblock);
void ProcessRDB(struct NepClassMS *ncm);
BOOL AutoMountCD(struct NepClassMS *ncm);

void AutoDetectMaxTransfer(struct NepClassMS *ncm);

AROS_UFP0(void, GM_UNIQUENAME(nMSTask));
AROS_UFP0(void, GM_UNIQUENAME(nRemovableTask));
AROS_UFP0(void, GM_UNIQUENAME(nGUITask));

AROS_UFP3(LONG, GM_UNIQUENAME(LUNListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct NepClassMS *, ncm, A1));

BOOL CheckPartitions(struct NepClassMS *ncm);

#endif /* MASSSTORAGE_CLASS_H */
