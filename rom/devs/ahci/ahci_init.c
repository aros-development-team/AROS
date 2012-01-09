/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

/* Maintainer: Jason S. McMullan <jason.mcmullan@gmail.com>
 */

#define DEBUG 0

#include <aros/debug.h>
#include <aros/atomic.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#include <string.h>

#include "ahci.h"
#include "ahci_intern.h"
#include "timer.h"

u_int32_t AhciForceGen;
u_int32_t AhciNoFeatures;

#include LC_LIBDEFS_FILE

/*
    Here shall we start. Make function static as it shouldn't be visible from
    outside.
*/
static int AHCI_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[AHCI--] AHCI_Init: ahci.device Initialization\n"));

    /*
     * I've decided to use memory pools again. Alloc everything needed from 
     * a pool, so that we avoid memory fragmentation.
     */
    LIBBASE->ahci_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (LIBBASE->ahci_MemPool == NULL)
        return FALSE;

    D(bug("[AHCI--] AHCI_Init: MemPool @ %p\n", LIBBASE->ahci_MemPool));

    /* Initialize lists */
    NEWLIST(&LIBBASE->ahci_Units);
    LIBBASE->ahci_UnitCount=0;

    /* Get some useful bases */
    LIBBASE->ahci_HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    LIBBASE->ahci_HiddPCIDeviceMethodBase = OOP_GetMethodID(IID_Hidd_PCIDevice, 0);

    LIBBASE->ahci_HiddPCIDriverMethodBase = OOP_GetMethodID(IID_Hidd_PCIDriver, 0);
    return TRUE;
}

static int AHCI_Open
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq,
    ULONG unitnum,
    ULONG flags
)
{
    struct cam_sim *tmp, *unit = NULL;

    /* 
     * Assume it failed 
     */
    iorq->io_Error = IOERR_OPENFAIL;

    ForeachNode(&LIBBASE->ahci_Units, tmp) {
        if (tmp->sim_Unit == unitnum) {
            unit = tmp;
            AROS_ATOMIC_INC(unit->sim_UseCount);
            break;
        }
    }

    if (unit == NULL)
        return FALSE;

    /*
     * set up iorequest
     */
    iorq->io_Device     = &LIBBASE->ahci_Device;
    iorq->io_Unit       = (struct Unit *)unit;
    iorq->io_Error      = 0;

    return TRUE;
}

/* Close given device */
static int AHCI_Close
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq
)
{
    struct cam_sim *unit = (struct cam_sim *)iorq->io_Unit;

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    AROS_ATOMIC_DEC(unit->sim_UseCount);

    return TRUE;
}

ADD2INITLIB(AHCI_Init, 0)
ADD2OPENDEV(AHCI_Open, 0)
ADD2CLOSEDEV(AHCI_Close, 0)

