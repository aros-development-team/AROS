/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved

    Desc: UHCI chipset driver root hub/port support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "uhciproto.h"

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

void uhciCheckPortStatusChange(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    UWORD oldval;
    UWORD hciport;

    // check for port status change for UHCI and frame rollovers

    for(hciport = 0; hciport < 2; hciport++) {
        UWORD portreg;
        UWORD idx = hc->hc_PortNum[hciport];
        // don't pay attention to UHCI port changes when pwned by EHCI
        if(unit->hu_PortOwner[idx] == HCITYPE_UHCI) {
            portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
            oldval = READIO16_LE(hc->hc_RegBase, portreg);
            if(oldval & UHPF_ENABLECHANGE) {
                pciusbUHCIDebug("UHCI", "Port %ld (%ld) Enable changed\n", idx, hciport);
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
            }
            if(oldval & UHPF_CONNECTCHANGE) {
                pciusbUHCIDebug("UHCI", "Port %ld (%ld) Connect changed\n", idx, hciport);
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                if(!(oldval & UHPF_PORTCONNECTED)) {
                    if(unit->hu_PortMap20[idx]) {
                        pciusbUHCIDebug("UHCI", "UHCI: Transferring Port %ld back to EHCI\n", idx);
                        unit->hu_PortOwner[idx] = HCITYPE_EHCI;
                    }
                }
            }
            if(oldval & UHPF_RESUMEDTX) {
                pciusbUHCIDebug("UHCI", "Port %ld (%ld) Resume changed\n", idx, hciport);
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                oldval &= ~UHPF_RESUMEDTX;
            }
            if(hc->hc_PortChangeMap[hciport]) {
                unit->hu_RootPortChanges |= 1UL<<(idx+1);
                /*KPRINTF(10, ("Port %ld (%ld) contributes %04lx to portmap %04lx\n",
                             idx, hciport, hc->hc_PortChangeMap[hciport], unit->hu_RootPortChanges));*/
            }
            WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);
        }
    }
}

BOOL uhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
    ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
    ULONG newval = oldval;
    ULONG cnt;
    BOOL cmdgood = FALSE;

    pciusbUHCIDebug("UHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    /* case UFS_PORT_CONNECTION: not possible */
    case UFS_PORT_ENABLE:
        pciusbUHCIDebug("UHCI", "Enabling Port (%s)\n", newval & UHPF_PORTENABLE ? "already" : "ok");
        newval |= UHPF_PORTENABLE;
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        newval |= UHPF_PORTSUSPEND;
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
        cmdgood = TRUE;
        break;

    /* case UFS_PORT_OVER_CURRENT: not possible */
    case UFS_PORT_RESET:
        pciusbUHCIDebug("UHCI", "Resetting Port (%s)\n", newval & UHPF_PORTRESET ? "already" : "ok");

        // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
        newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
        newval |= UHPF_PORTRESET;
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        uhwDelayMS(25, unit->hu_TimerReq);
        newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
        pciusbUHCIDebug("UHCI", "Reset=%s\n", newval & UHPF_PORTRESET ? "GOOD" : "BAD!");
        // like windows does it
        newval &= ~UHPF_PORTRESET;
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        uhwDelayMicro(50, unit->hu_TimerReq);
        newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
        pciusbUHCIDebug("UHCI", "Reset=%s\n", newval & UHPF_PORTRESET ? "BAD!" : "GOOD");
        newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTRESET);
        newval |= UHPF_PORTENABLE;
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change

        cnt = 100;
        do {
            uhwDelayMS(1, unit->hu_TimerReq);
            newval = READIO16_LE(hc->hc_RegBase, portreg);
        } while(--cnt && (!(newval & UHPF_PORTENABLE)));
        if(cnt) {
            pciusbUHCIDebug("UHCI", "Enabled after %ld ticks\n", 100-cnt);
        } else {
            pciusbWarn("UHCI", "Port refuses to be enabled!\n");
            *retval = UHIOERR_HOSTERROR;
            return TRUE;
        }
        // make enumeration possible
        unit->hu_DevControllers[0] = hc;
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        pciusbUHCIDebug("UHCI", "Powering Port\n");
        // ignore for UHCI, is always powered
        cmdgood = TRUE;
        break;

        /* case UFS_PORT_LOW_SPEED: not possible */
        /* case UFS_C_PORT_CONNECTION:
        case UFS_C_PORT_ENABLE:
        case UFS_C_PORT_SUSPEND:
        case UFS_C_PORT_OVER_CURRENT:
        case UFS_C_PORT_RESET: */
    }
    if(cmdgood) {
        pciusbUHCIDebug("UHCI", "Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval);
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
    }
    return cmdgood;
}

BOOL uhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
    ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
    ULONG newval = oldval;
    BOOL cmdgood = FALSE;

    pciusbUHCIDebug("UHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    case UFS_PORT_ENABLE:
        pciusbUHCIDebug("UHCI", "Disabling Port (%s)\n", newval & UHPF_PORTENABLE ? "ok" : "already");
        newval &= ~UHPF_PORTENABLE;
        cmdgood = TRUE;
        // disable enumeration
        unit->hu_DevControllers[0] = NULL;
        break;

    case UFS_PORT_SUSPEND:
        newval &= ~UHPF_PORTSUSPEND;
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER: // ignore for UHCI, there's no power control here
        pciusbUHCIDebug("UHCI", "Disabling Power\n");
        pciusbUHCIDebug("UHCI", "Disabling Port (%s)\n", newval & UHPF_PORTENABLE ? "ok" : "already");
        newval &= ~UHPF_PORTENABLE;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_CONNECTION:
        newval |= UHPF_CONNECTCHANGE; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_ENABLE:
        newval |= UHPF_ENABLECHANGE; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_SUSPEND: // ignore for UHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change clearing
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_OVER_CURRENT: // ignore for UHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT; // manually fake over current clearing
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_RESET: // ignore for UHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET; // manually fake reset change clearing
        cmdgood = TRUE;
        break;
    }
    if(cmdgood) {
        pciusbUHCIDebug("UHCI", "Port %ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval);
        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
        if(hc->hc_PortChangeMap[hciport]) {
            unit->hu_RootPortChanges |= 1UL<<(idx + 1);
        } else {
            unit->hu_RootPortChanges &= ~(1UL<<(idx + 1));
        }
    }
    return cmdgood;
}

BOOL uhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval)
{
    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
    UWORD oldval = READIO16_LE(hc->hc_RegBase, portreg);

    pciusbUHCIDebug("UHCI", "%s(0x%p, 0x%p, %04x, %04x, 0x%p)\n", __func__, hc, mptr, hciport, idx, retval);

    *mptr = AROS_WORD2LE(UPSF_PORT_POWER);

    if(oldval & UHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    if(oldval & UHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
    if(oldval & UHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
    if(oldval & UHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    if(oldval & UHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

    pciusbUHCIDebug("UHCI", "Port %ld is %s\n", idx, oldval & UHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED");
    pciusbUHCIDebug("UHCI", "Port %ld Status %08lx\n", idx, *mptr);

    mptr++;
    if(oldval & UHPF_ENABLECHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if(oldval & UHPF_CONNECTCHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if(oldval & UHPF_RESUMEDTX) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
    WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);

    pciusbUHCIDebug("UHCI", "Port %ld Change %08lx\n", idx, *mptr);

    return TRUE;
}

#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#undef base
