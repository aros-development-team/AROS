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
#include <dos/filehandler.h>

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

int ahci_RegisterPort(struct ahci_port *ap)
{
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_AHCIBase;
    struct cam_sim *unit;

    unit = AllocPooled(AHCIBase->ahci_MemPool, sizeof(*unit));
    if (!unit)
        return ENOMEM;

    ap->ap_sim = unit;
    unit->sim_Port = ap;
    unit->sim_Unit = AHCIBase->ahci_UnitCount++;
    InitSemaphore(&unit->sim_Lock);

    AddTail((struct List *)&AHCIBase->ahci_Units, (struct Node *)unit);

    return 0;
}

int ahci_UnregisterPort(struct ahci_port *ap)
{
    struct ahci_softc *sc = ap->ap_sc;
    struct AHCIBase *AHCIBase;
    struct cam_sim *unit = ap->ap_sim;

    D(bug("ahci_UnregisterPort: %p\n", ap));

    if (sc == NULL) {
        D(bug("No softc?\n"));
        return 0;
    }

    AHCIBase = sc->sc_dev->dev_AHCIBase;

    /* FIXME: Stop IO on this device? */

    Remove((struct Node *)unit);
    FreePooled(AHCIBase->ahci_MemPool, unit, sizeof(*unit));

    return 0;
}

/* Add a bootnode using expansion.library */
BOOL ahci_RegisterVolume(struct ahci_port *port)
{
    struct ata_port *at = port->ap_ata[0];
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    TEXT dosdevname[4] = "HA0";
    const ULONG DOS_ID = AROS_MAKE_ID('D','O','S','\001');
    const ULONG CDROM_ID = AROS_MAKE_ID('C','D','V','D');

    D(bug("ahci_RegisterVolume: port = %p, at = %p, unit = %d\n", port, at, port->ap_sim ? port->ap_sim->sim_Unit : -1));

    if (at == NULL || port->ap_type == ATA_PORT_T_NONE)
        return FALSE;

    /* This should be dealt with using some sort of volume manager or such. */
    switch (port->ap_type)
    {
        case ATA_PORT_T_DISK:
            break;
        case ATA_PORT_T_ATAPI:
            dosdevname[0] = 'C';
            break;
        default:
            D(bug("[AHCI>>]:-ahci_RegisterVolume called on unknown devicetype\n"));
            return FALSE;
    }

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
                                                        40L);

    if (ExpansionBase)
    {
        IPTR pp[4 + DE_BOOTBLOCKS + 1];

        if (port->ap_num < 10)
            dosdevname[2] += port->ap_num;
        else
            dosdevname[2] = 'A' + (port->ap_num - 10);
    
        pp[0] 		    = (IPTR)dosdevname;
        pp[1]		    = (IPTR)MOD_NAME_STRING;
        pp[2]		    = port->ap_sim->sim_Unit;
        pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK    + 4] = at->at_identify.sector_size;
        pp[DE_NUMHEADS     + 4] = at->at_identify.nheads;
        pp[DE_SECSPERBLOCK + 4] = 1;
        pp[DE_BLKSPERTRACK + 4] = at->at_identify.nsectors;
        pp[DE_RESERVEDBLKS + 4] = 2;
        pp[DE_LOWCYL       + 4] = 0;
        pp[DE_HIGHCYL      + 4] = at->at_identify.ncyls-1;
        pp[DE_NUMBUFFERS   + 4] = 10;
        pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC;
        pp[DE_MAXTRANSFER  + 4] = 0x00200000;
        pp[DE_MASK         + 4] = ~3;
        pp[DE_BOOTPRI      + 4] = (port->ap_type == ATA_PORT_T_DISK) ? 0 : 10;
        pp[DE_DOSTYPE      + 4] = (port->ap_type == ATA_PORT_T_DISK) ? DOS_ID : CDROM_ID;
        pp[DE_BOOTBLOCKS   + 4] = 2;
    
        devnode = MakeDosNode(pp);

        if (devnode) {
            D(bug("[AHCI>>]:-ahci_RegisterVolume: '%s' C/H/S=%d/%d/%d, %s unit %d\n",
                        AROS_BSTR_ADDR(devnode->dn_Name), at->at_identify.ncyls, at->at_identify.nheads,  at->at_identify.nsectors, MOD_NAME_STRING, port->ap_sim->sim_Unit));
            AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
            D(bug("[AHCI>>]:-ahci_RegisterVolume: done\n"));
            return TRUE;
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

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

