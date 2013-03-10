#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/resident.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

#include "ata.h"

extern const char ata_LibName[];
extern const char ata_LibID[];
extern const int ata_End;

AROS_UFP3(static APTR, ata_Wait,
	  AROS_UFPA(void *, dummy, D0),
	  AROS_UFPA(BPTR, segList, A0),
	  AROS_UFPA(struct ExecBase *, SysBase, A6));

const struct Resident ata_BootWait =
{
    RTC_MATCHWORD,
    (struct Resident *)&ata_BootWait,
    (void *)&ata_End,
    RTF_COLDSTART,
    VERSION_NUMBER,
    NT_TASK,
    -49, /* dosboot.resource is -50 */
    "ATA boot wait",
    &ata_LibID[6],
    &ata_Wait,
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

AROS_UFH3(static APTR, ata_Wait,
	  AROS_UFPA(void *, dummy, D0),
	  AROS_UFPA(BPTR, segList, A0),
	  AROS_UFPA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct ataBase *ATABase;

    /* We do not want to deal with IORequest and units, so just FindName() */
    ATABase = (struct ataBase *)FindName(&SysBase->DeviceList, ata_LibName);
    if (ATABase)
    {
        D(bug("[ATA  ] Waiting for device detection to complete...\n"));
        ObtainSemaphore(&ATABase->DetectionSem);
        ReleaseSemaphore(&ATABase->DetectionSem);
    }
    
    return NULL;
    
    AROS_USERFUNC_EXIT
}
