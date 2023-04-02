/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved
*/

#define DB_LEVEL 100

#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;

    struct TagItem pciActivateMem[] =
    {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    KPRINTF(20, ("ohciInit returns TRUE...\n"));
    return TRUE;
}

/* ** Root hub support functions ** */

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val)
{
    BOOL cmdgood = FALSE;
    UWORD portreg;
    ULONG oldval;
    ULONG newval;
    switch(val)
    {
        case UFS_PORT_ENABLE:
            KPRINTF(10, ("XHCI: Enabling Port (%s)\n", newval & EHPF_PORTENABLE ? "already" : "ok"));
            cmdgood = TRUE;
            break;

        case UFS_PORT_SUSPEND:
            cmdgood = TRUE;
            break;

        /* case UFS_PORT_OVER_CURRENT: not possible */
        case UFS_PORT_RESET:
            KPRINTF(10, ("XHCI: Resetting Port (%s)\n", newval & EHPF_PORTRESET ? "already" : "ok"));
            cmdgood = TRUE;
            break;

        case UFS_PORT_POWER:
            KPRINTF(10, ("XHCI: Powering Port\n"));
            newval |= EHPF_PORTPOWER;
            cmdgood = TRUE;
            break;
    }
    if(cmdgood)
    {
        KPRINTF(5, ("XHCI: Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval));
        return(0);
    }
    return cmdgood;
}

BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val)
{
    BOOL cmdgood = FALSE;
    switch(val)
    {
        case UFS_PORT_ENABLE:
            cmdgood = TRUE;
            // disable enumeration
            unit->hu_DevControllers[0] = NULL;
            break;

        case UFS_PORT_SUSPEND:
            cmdgood = TRUE;
            break;

        case UFS_PORT_POWER:
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_CONNECTION:
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_ENABLE:
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_SUSPEND:
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_OVER_CURRENT:
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_RESET:
            cmdgood = TRUE;
            break;
    }
    if(cmdgood)
    {
        KPRINTF(5, ("Port %ld CLEAR_FEATURE %08lx->%08lx\n", idx, val, val));
    }
    return cmdgood;
}

BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx)
{
    return FALSE;
}
