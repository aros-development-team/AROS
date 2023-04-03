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

#undef base
#define base (hc->hc_Device)

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;
    ULONG cnt;

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

#if defined(TMPXHCICODE)
    for(cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
        hu->hu_PortMapX[cnt] = hc;
        hc->hc_PortNum[cnt] = cnt;
    }
#endif

    KPRINTF(20, ("ohciInit returns TRUE...\n"));
    return TRUE;
}

void xhciFree(struct PCIController *hc, struct PCIUnit *hu) {
}

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    BOOL cmdgood = FALSE;
    UWORD portreg;
    ULONG oldval;
    ULONG newval;

    KPRINTF(5, ("XHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval));

    switch(val)
    {
        case UFS_PORT_ENABLE:
            cmdgood = TRUE;
            break;

        case UFS_PORT_SUSPEND:
            cmdgood = TRUE;
            break;

        /* case UFS_PORT_OVER_CURRENT: not possible */
        case UFS_PORT_RESET:
            cmdgood = TRUE;
            break;

        case UFS_PORT_POWER:
            KPRINTF(10, ("XHCI: Powering Port\n"));
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

BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    BOOL cmdgood = FALSE;

    KPRINTF(5, ("XHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval));

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
        KPRINTF(5, ("XHCI: Port %ld CLEAR_FEATURE %08lx->%08lx\n", idx, val, val));
    }
    return cmdgood;
}

BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval)
{
    KPRINTF(5, ("XHCI: %s(0x%p, 0x%p, %04x, %04x, 0x%p)\n", __func__, hc, mptr, hciport, idx, retval));

    return FALSE;
}
