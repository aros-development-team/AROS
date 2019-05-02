/*
    Copyright © 2004-2018, The AROS Development Team. All rights reserved.
    $Id: lowlevel.c 55802 2019-03-08 21:47:59Z wawa $

    Desc:
    Lang: English
*/

/*
 * TODO:
 * - put a critical section around DMA transfers (shared dma channels)
 */

#include <aros/debug.h>

#include <proto/exec.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <devices/timer.h>

#include "scsi.h"
#include "scsi_bus.h"
#include "timer.h"

// use #define xxx(a) D(a) to enable particular sections.
#if DEBUG
#define DIRQ(a) D(a)
#define DIRQ_MORE(a)
#define DUMP(a) D(a)
#define DUMP_MORE(a)
#define DSCSI(a) D(a)
#define DATAPI(a) D(a)
#define DINIT(a) D(a)
#else
#define DIRQ(a)      do { } while (0)
#define DIRQ_MORE(a) do { } while (0)
#define DUMP(a)      do { } while (0)
#define DUMP_MORE(a) do { } while (0)
#define DSCSI(a)      do { } while (0)
#define DATAPI(a)    do { } while (0)
#define DINIT(a)
#endif
/* Errors that shouldn't happen */
#define DERROR(a) a

/*
 * Initial device configuration that suits *all* cases
 */
void scsi_init_unit(struct scsi_Bus *bus, struct scsi_Unit *unit, UBYTE u)
{
    struct scsiBase *SCSIBase = bus->sb_Base;
    OOP_Object *obj = OOP_OBJECT(SCSIBase->busClass, bus);

    unit->su_Bus       = bus;
    unit->pioInterface = bus->pioInterface;
    unit->su_UnitNum   = bus->sb_BusNum << 1 | u;      // b << 8 | u
    unit->su_DevMask   = 0xa0 | (u << 4);

    DINIT(bug("[SCSI%02u] scsi_init_unit: bus %u unit %d\n", unit->su_UnitNum, bus->sb_BusNum, u));

#if (0)
    /* Set PIO transfer functions, either 16 or 32 bits */
    if (SCSIBase->scsi_32bit && OOP_GET(obj, aHidd_SCSIBus_Use32Bit))
        Unit_Enable32Bit(unit);
    else
        Unit_Disable32Bit(unit);
#endif
}

BOOL scsi_setup_unit(struct scsi_Bus *bus, struct scsi_Unit *unit)
{
    /*
     * this stuff always goes along the same way
     * WARNING: NO INTERRUPTS AT THIS POINT!
     */
    UBYTE u;

    DINIT(bug("[SCSI  ] scsi_setup_unit(%d)\n", unit->su_UnitNum));
#if (0)
    scsi_SelectUnit(unit);

    if (FALSE == scsi_WaitBusyTO(unit, 1, FALSE, FALSE, NULL))
    {
        DINIT(bug("[SCSI%02ld] scsi_setup_unit: ERROR: Drive not ready for use. Keeping functions stubbed\n", unit->su_UnitNum));
        return FALSE;
    }

    u = unit->su_UnitNum & 1;
    switch (bus->sb_Dev[u])
    {
        /*
         * safe fallback settings
         */
        case DEV_SATAPI:
        case DEV_ATAPI:
        case DEV_SATA:
        case DEV_ATA:
            unit->su_Identify = scsi_Identify;
            break;

        default:
            DINIT(bug("[SCSI%02ld] scsi_setup_unit: Unsupported device %lx. All functions will remain stubbed.\n", unit->su_UnitNum, bus->sb_Dev[u]));
            return FALSE;
    }

    DINIT(bug("[SCSI  ] scsi_setup_unit: Enabling IRQs\n"));
    PIO_OutAlt(bus, 0x0, scsi_AltControl);

    /*
     * now make unit self diagnose
     */
    if (unit->su_Identify(unit) != 0)
    {
        return FALSE;
    }
#endif

    return TRUE;
}

void scsi_InitBus(struct scsi_Bus *bus)
{
    struct scsiBase *SCSIBase = bus->sb_Base;
    OOP_Object *obj = OOP_OBJECT(SCSIBase->busClass, bus);
    IPTR haveAltIO;
    UBYTE tmp1, tmp2;
    UWORD i;

    /*
     * initialize timer for the sake of scanning
     */
    bus->sb_Timer = scsi_OpenTimer(bus->sb_Base);

#if (0)
    OOP_GetAttr(obj, aHidd_SCSIBus_UseIOAlt, &haveAltIO);
    bus->haveAltIO = haveAltIO != 0;
#endif

    DINIT(bug("[SCSI  ] scsi_InitBus(%p)\n", bus));

    bus->sb_Dev[0] = DEV_NONE;
    bus->sb_Dev[1] = DEV_NONE;

    /* Check if device 0 and/or 1 is present on this bus. It may happen that
       a single drive answers for both device addresses, but the phantom
       drive will be filtered out later */
    for (i = 0; i < MAX_BUSUNITS; i++)
    {
        /* Select device and disable IRQs */
#if (0)
        PIO_Out(bus, DEVHEAD_VAL | (i << 4), scsi_DevHead);
#endif
        scsi_WaitTO(bus->sb_Timer, 0, 400, 0);
        PIO_OutAlt(bus, SCSICTLF_INT_DISABLE, scsi_AltControl);

        /* Write some pattern to registers. This is a variant of a more
           common technique, with the difference that we don't use the
           sector count register because some bad ATAPI drives disallow
           writing to it */
        PIO_Out(bus, 0x55, scsi_LBALow);
        PIO_Out(bus, 0xaa, scsi_LBAMid);
        PIO_Out(bus, 0xaa, scsi_LBALow);
        PIO_Out(bus, 0x55, scsi_LBAMid);
        PIO_Out(bus, 0x55, scsi_LBALow);
        PIO_Out(bus, 0xaa, scsi_LBAMid);

        tmp1 = PIO_In(bus, scsi_LBALow);
        tmp2 = PIO_In(bus, scsi_LBAMid);
        DB2(bug("[SCSI  ] scsi_InitBus: Reply 0x%02X 0x%02X\n", tmp1, tmp2));

        if ((tmp1 == 0x55) && (tmp2 == 0xaa))
            bus->sb_Dev[i] = DEV_UNKNOWN;
        DINIT(bug("[SCSI  ] scsi_InitBus: Device type = 0x%02X\n", bus->sb_Dev[i]));
    }
#if (0)
    scsi_ResetBus(bus);
#endif
    scsi_CloseTimer(bus->sb_Timer);
    DINIT(bug("[SCSI  ] scsi_InitBus: Finished\n"));
}
