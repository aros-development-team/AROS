/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id: bootwait.c 55802 2019-03-08 21:47:59Z wawa $
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <aros/asmcall.h>
#include <exec/resident.h>
#include <libraries/expansionbase.h>

#include LC_LIBDEFS_FILE

#include "scsi.h"

#if defined(__AROSPLATFORM_SMP__)
#include <aros/types/spinlock_s.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#endif

extern const char scsi_LibName[];
extern const char scsi_LibID[];
extern const int scsi_End;

AROS_UFP3(static APTR, scsi_Wait,
	  AROS_UFPA(void *, dummy, D0),
	  AROS_UFPA(BPTR, segList, A0),
	  AROS_UFPA(struct ExecBase *, SysBase, A6));

const struct Resident scsi_BootWait =
{
    RTC_MATCHWORD,
    (struct Resident *)&scsi_BootWait,
    (void *)&scsi_End,
    RTF_COLDSTART,
    VERSION_NUMBER,
    NT_TASK,
    -49, /* dosboot.resource is -50 */
    "SCSI boot wait",
    &scsi_LibID[6],
    &scsi_Wait,
};

/*
 * The purpose of this delay is to wait until device detection is done
 * before boot sequence enters DOS bootstrap. Without this we reach the
 * bootstrap earlier than devices are detected (and BootNodes inserted).
 * As a result, we end up in unbootable system.
 * Actually, i dislike this solution a bit. I think something else has
 * to be implemented. However i do not know what. Even if we rewrite
 * adding BootNodes, bootmenu still has to wait until all nodes are added.
 * Making device detection synchronous is IMHO not a good option, it will
 * increase booting time of our OS.
 */

AROS_UFH3(static APTR, scsi_Wait,
	  AROS_UFPA(void *, dummy, D0),
	  AROS_UFPA(BPTR, segList, A0),
	  AROS_UFPA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct scsiBase *SCSIBase;
#if defined(__AROSPLATFORM_SMP__)
    void *ExecLockBase = OpenResource("execlock.resource");
#endif

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ObtainSystemLock(&SysBase->DeviceList, SPINLOCK_MODE_READ, LOCKF_FORBID);
    else
        Forbid();
#else
    Forbid();
#endif

    /* We do not want to deal with IORequest and units, so just FindName() */
    SCSIBase = (struct scsiBase *)FindName(&SysBase->DeviceList, scsi_LibName);

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ReleaseSystemLock(&SysBase->DeviceList, LOCKF_FORBID);
    else
        Permit();
#else
    Permit();
#endif

    if (SCSIBase)
    {
        D(bug("[SCSI  ] Waiting for device detection to complete...\n"));
        ObtainSemaphore(&SCSIBase->DetectionSem);
        ReleaseSemaphore(&SCSIBase->DetectionSem);
    }
    
    return NULL;
    
    AROS_USERFUNC_EXIT
}
