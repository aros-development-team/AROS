/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved

    Desc: OHCI chipset driver root hub/port support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>
#include <exec/memory.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "ohciproto.h"

#ifdef base
#undef base
#endif
#define base (hc->hc_Device)
#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (hc->hc_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

void ohciCheckPortStatusChange(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    UWORD hciport;
    ULONG oldval;
    UWORD portreg = OHCI_PORTSTATUS;
    BOOL clearbits = FALSE;

    if(READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS) & OISF_HUBCHANGE) {
        // some OHCI implementations will keep the interrupt bit stuck until
        // all port changes have been cleared, which is wrong according to the
        // OHCI spec. As a workaround we will clear all change bits, which should
        // be no problem as the port changes are reflected in the PortChangeMap
        // array.
        clearbits = TRUE;
    }
    for(hciport = 0; hciport < hc->hc_NumPorts; hciport++, portreg += 4) {
        oldval = READREG32_LE(hc->hc_RegBase, portreg);
        if(oldval & OHPF_OVERCURRENTCHG) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
        }
        if(oldval & OHPF_RESETCHANGE) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
        }
        if(oldval & OHPF_ENABLECHANGE) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
        }
        if(oldval & OHPF_CONNECTCHANGE) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
        }
        if(oldval & OHPF_RESUMEDTX) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND;
        }
        if(clearbits) {
            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_CONNECTCHANGE|OHPF_ENABLECHANGE|OHPF_RESUMEDTX|OHPF_OVERCURRENTCHG|OHPF_RESETCHANGE);
        }

        pciusbOHCIDebug("OHCI", "PCI Int Port %ld (glob %ld) Change %08lx\n",
                        hciport, hc->hc_PortNum[hciport] + 1, oldval);
        if(hc->hc_PortChangeMap[hciport]) {
            unit->hu_RootPortChanges |= 1UL<<(hc->hc_PortNum[hciport] + 1);
        }
    }
    uhwCheckRootHubChanges(unit);
    if(clearbits) {
        // again try to get rid of any bits that may be causing the interrupt
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_OVERCURRENTCHG);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_HUBCHANGE);
    }
}

BOOL ohciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);
    BOOL cmdgood = FALSE;

    pciusbOHCIDebug("OHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    /* case UFS_PORT_CONNECTION: not possible */
    case UFS_PORT_ENABLE:
        pciusbOHCIDebug("OHCI", "Enabling Port (%s)\n", oldval & OHPF_PORTENABLE ? "already" : "ok");
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTENABLE);
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        pciusbOHCIDebug("OHCI", "Suspending Port (%s)\n", oldval & OHPF_PORTSUSPEND ? "already" : "ok");
        //hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTSUSPEND);
        cmdgood = TRUE;
        break;

    /* case UFS_PORT_OVER_CURRENT: not possible */
    case UFS_PORT_RESET:
        pciusbOHCIDebug("OHCI", "Resetting Port (%s)\n", oldval & OHPF_PORTRESET ? "already" : "ok");
        // make sure we have at least 50ms of reset time here, as required for a root hub port
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
        uhwDelayMS(10, unit->hu_TimerReq);
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
        uhwDelayMS(10, unit->hu_TimerReq);
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
        uhwDelayMS(10, unit->hu_TimerReq);
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
        uhwDelayMS(10, unit->hu_TimerReq);
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
        uhwDelayMS(15, unit->hu_TimerReq);
        oldval = READREG32_LE(hc->hc_RegBase, portreg);
        pciusbOHCIDebug("OHCI", "Reset release (%s %s)\n", oldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                    oldval & OHPF_PORTENABLE ? "enabled" : "not enabled");
        if(oldval & OHPF_PORTRESET) {
            uhwDelayMS(40, unit->hu_TimerReq);
            oldval = READREG32_LE(hc->hc_RegBase, portreg);
            pciusbOHCIDebug("OHCI", "Reset 2nd release (%s %s)\n", oldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                        oldval & OHPF_PORTENABLE ? "enabled" : "still not enabled");
        }
        // make enumeration possible
        unit->hu_DevControllers[0] = hc;
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        pciusbOHCIDebug("OHCI", "Powering Port (%s)\n", oldval & OHPF_PORTPOWER ? "already" : "ok");
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTPOWER);
        cmdgood = TRUE;
        break;

        /* case UFS_PORT_LOW_SPEED: not possible */
        /* case UFS_C_PORT_CONNECTION:
        case UFS_C_PORT_ENABLE:
        case UFS_C_PORT_SUSPEND:
        case UFS_C_PORT_OVER_CURRENT:
        case UFS_C_PORT_RESET: */
    }
    return cmdgood;
}

BOOL ohciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
    ULONG __unused oldval = READREG32_LE(hc->hc_RegBase, portreg);
    BOOL cmdgood = FALSE;

    pciusbOHCIDebug("OHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    case UFS_PORT_ENABLE:
        pciusbOHCIDebug("OHCI", "Disabling Port (%s)\n", oldval & OHPF_PORTENABLE ? "ok" : "already");
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTDISABLE);
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        pciusbOHCIDebug("OHCI", "Resuming Port (%s)\n", oldval & OHPF_PORTSUSPEND ? "ok" : "already");
        //hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_RESUME);
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        pciusbOHCIDebug("OHCI", "Unpowering Port (%s)\n", oldval & OHPF_PORTPOWER ? "ok" : "already");
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTUNPOWER);
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_CONNECTION:
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_CONNECTCHANGE);
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_ENABLE:
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_ENABLECHANGE);
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_SUSPEND:
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_RESUMEDTX);
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_OVER_CURRENT:
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_OVERCURRENTCHG);
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_RESET:
        WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_RESETCHANGE);
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET;
        cmdgood = TRUE;
        break;
    }
    return cmdgood;
}

BOOL ohciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval)
{
    UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

    pciusbOHCIDebug("OHCI", "%s(0x%p, 0x%p, %04x, %04x, 0x%p)\n", __func__, hc, mptr, hciport, idx, retval);

    *mptr = 0;
    if(oldval & OHPF_PORTPOWER) *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    if(oldval & OHPF_OVERCURRENT) *mptr |= AROS_WORD2LE(UPSF_PORT_OVER_CURRENT);
    if(oldval & OHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    if(oldval & OHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
    if(oldval & OHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
    if(oldval & OHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    if(oldval & OHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

    pciusbOHCIDebug("OHCI", "Port %ld (glob. %ld) is %s\n", hciport, idx, oldval & OHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED");
    pciusbOHCIDebug("OHCI", "Port %ld Status %08lx (%08lx)\n", idx, *mptr, oldval);

    mptr++;
    if(oldval & OHPF_OVERCURRENTCHG) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
    }
    if(oldval & OHPF_RESETCHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
    }
    if(oldval & OHPF_ENABLECHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if(oldval & OHPF_CONNECTCHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if(oldval & OHPF_RESUMEDTX) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND;
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);

    pciusbOHCIDebug("OHCI", "Port %ld Change %08lx\n", idx, *mptr);

    return TRUE;
}

#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#undef base
