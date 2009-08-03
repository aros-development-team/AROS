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
BOOL GM_UNIQUENAME(nLoadBindingConfig)(struct NepClassMS *ncm);
LONG GM_UNIQUENAME(nOpenBindingCfgWindow)(struct NepMSBase *nh, struct NepClassMS *ncm);

void GM_UNIQUENAME(nGUITaskCleanup)(struct NepClassMS *ncm);
BOOL GM_UNIQUENAME(nStoreConfig)(struct NepClassMS *ncm);

LONG nScsiDirect(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nScsiDirectBulk(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nScsiDirectCBI(struct NepClassMS *ncm, struct SCSICmd *scsicmd);
LONG nBulkReset(struct NepClassMS *ncm);
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
void CheckFATPartition(struct NepClassMS *ncm, ULONG startblock);
void ProcessRDB(struct NepClassMS *ncm);
void AutoMountCD(struct NepClassMS *ncm);
void CheckISO9660(struct NepClassMS *ncm);

void AutoDetectMaxTransfer(struct NepClassMS *ncm);

AROS_UFP0(void, GM_UNIQUENAME(nMSTask));
AROS_UFP0(void, GM_UNIQUENAME(nRemovableTask));
AROS_UFP0(void, GM_UNIQUENAME(nGUITask));

AROS_UFP3(LONG, GM_UNIQUENAME(LUNListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct NepClassMS *, ncm, A1));
          
#endif /* MASSSTORAGE_CLASS_H */
