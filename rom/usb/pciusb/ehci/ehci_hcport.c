/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved

    Desc: EHCI chipset driver root hub/port support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>
#include <exec/memory.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "ehciproto.h"
#include "ohci/ohcichip.h"
#include "uhci/uhcichip.h"

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

void ehciCheckPortStatusChange(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    UWORD hciport;
    ULONG oldval;
    UWORD portreg = EHCI_PORTSC1;

    /*
     * Port-change handling operates on controller-local ports (hciport) but
     * reports changes in the unit-global root-hub port space.
     *
     * With multi-controller devices (multiple EHCI instances per PCI device,
     * and/or truncated global port tables), the local->global mapping may be
     * absent for some ports. Guard against unmapped ports to avoid accessing
     * unit-global arrays out of bounds.
     */
    UWORD maxports = hc->hc_NumPorts;
    if (maxports > MAX_ROOT_PORTS)
        maxports = MAX_ROOT_PORTS;

    for(hciport = 0; hciport < maxports; hciport++, portreg += 4) {
        UWORD gport;

        oldval = READREG32_LE(hc->hc_RegBase, portreg);
        gport = hc->hc_PortNum[hciport];

        /* Always ACK clear-on-write change bits. */
        if(oldval & EHPF_ENABLECHANGE) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
        }
        if(oldval & EHPF_CONNECTCHANGE) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
        }
        if(oldval & EHPF_RESUMEDTX) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
        }
        if(oldval & EHPF_OVERCURRENTCHG) {
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
        }
        WRITEREG32_LE(hc->hc_RegBase, portreg, oldval);

        /* Skip unit-global reporting for unmapped ports. */
        if ((gport == 0xFF) || (gport >= MAX_ROOT_PORTS)) {
            pciusbEHCIDebug("EHCI", "PCI Int Port <unmapped> (local %ld) Change %08lx\n", hciport + 1, oldval);
            continue;
        }

        /* Reflect port ownership (EHCI vs. companion) for this global port. */
        {
            struct PCIController *chc = unit->hu_PortMap11[gport];
            unit->hu_PortOwner[gport] = (oldval & EHPF_NOTPORTOWNER)
                ? (chc ? chc->hc_HCIType : HCITYPE_UHCI)
                : HCITYPE_EHCI;
        }

        pciusbEHCIDebug("EHCI", "PCI Int Port %ld (local %ld) Change %08lx\n", gport + 1, hciport + 1, oldval);
        if(hc->hc_PortChangeMap[hciport]) {
            unit->hu_RootPortChanges |= 1UL<<(gport + 1);
        }
    }

    uhwCheckRootHubChanges(unit);
}

BOOL ehciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    struct PCIController *chc = unit->hu_PortMap11[idx - 1];
    UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE); // these are clear-on-write!
    ULONG newval = oldval;
    ULONG cnt;
    BOOL cmdgood = FALSE;

    pciusbEHCIDebug("EHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    /* case UFS_PORT_CONNECTION: not possible */
    case UFS_PORT_ENABLE:
        pciusbEHCIDebug("EHCI", "Enabling Port (%s)\n", newval & EHPF_PORTENABLE ? "already" : "ok");
        newval |= EHPF_PORTENABLE;
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        newval |= EHPF_PORTSUSPEND;
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
        cmdgood = TRUE;
        break;

    /* case UFS_PORT_OVER_CURRENT: not possible */
    case UFS_PORT_RESET:
        pciusbEHCIDebug("EHCI", "Resetting Port (%s)\n", newval & EHPF_PORTRESET ? "already" : "ok");

        // this is an ugly blocking workaround to the inability of EHCI to clear reset automatically
        newval &= ~(EHPF_PORTSUSPEND|EHPF_PORTENABLE);
        newval |= EHPF_PORTRESET;
        WRITEREG32_LE(hc->hc_RegBase, portreg, newval);

        // Wait for reset to complete (spec is 50ms, FreeBSD source suggests 200ms, but
        // we compromise to help USB volumes become available in time to be chosen as
        // the boot device)
        uhwDelayMS(125, unit->hu_TimerReq);

        newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND|EHPF_PORTENABLE);
        pciusbEHCIDebug("EHCI", "Reset=%s\n", newval & EHPF_PORTRESET ? "BAD!" : "GOOD");
        if (newval & EHPF_PORTRESET) {
            newval &= ~EHPF_PORTRESET;
            WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
        }
        uhwDelayMS(10, unit->hu_TimerReq);
        newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND);
        pciusbEHCIDebug("EHCI", "Reset=%s\n", newval & EHPF_PORTRESET ? "BAD!" : "GOOD");
        pciusbEHCIDebug("EHCI", "Highspeed=%s\n", newval & EHPF_PORTENABLE ? "YES!" : "NO");
        pciusbEHCIDebug("EHCI", "Port status=%08lx\n", newval);
        if(!(newval & EHPF_PORTENABLE)) {
            // if not highspeed, release ownership
            pciusbEHCIDebug("EHCI", "Transferring ownership to UHCI/OHCI port %ld\n", unit->hu_PortNum11[idx - 1]);
            pciusbEHCIDebug("EHCI", "Device is %s\n", newval & EHPF_LINESTATUS_DM ? "LOWSPEED" : "FULLSPEED");
            newval |= EHPF_NOTPORTOWNER;
            if(!chc) {
                pciusbWarn("EHCI", "No companion controller - can't transfer ownership!\n");
                WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                *retval = UHIOERR_HOSTERROR;
                return TRUE;
            }
            switch(chc->hc_HCIType) {
            case HCITYPE_UHCI: {
                UWORD uhcihciport = unit->hu_PortNum11[idx - 1];
                UWORD uhciportreg = uhcihciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                ULONG __unused uhcinewval = READREG16_LE(chc->hc_RegBase, uhciportreg);

                pciusbEHCIDebug("EHCI", "UHCI Port status before handover=%04lx\n", uhcinewval);
                break;
            }

            case HCITYPE_OHCI: {
                UWORD ohcihciport = unit->hu_PortNum11[idx - 1];
                UWORD ohciportreg = OHCI_PORTSTATUS + (ohcihciport<<2);
                ULONG __unused ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);

                pciusbEHCIDebug("EHCI", "OHCI Port status before handover=%08lx\n", ohcioldval);
                pciusbEHCIDebug("EHCI", "OHCI Powering Port (%s)\n", ohcioldval & OHPF_PORTPOWER ? "already" : "ok");
                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTPOWER);
                uhwDelayMS(10, unit->hu_TimerReq);
                pciusbEHCIDebug("EHCI", "OHCI Port status after handover=%08lx\n", READREG32_LE(chc->hc_RegBase, ohciportreg));
                break;
            }
            }
            newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND);
            pciusbEHCIDebug("EHCI", "Port status (reread)=%08lx\n", newval);
            newval |= EHPF_NOTPORTOWNER;
            unit->hu_PortOwner[idx - 1] = chc ? chc->hc_HCIType : HCITYPE_UHCI;
            WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
            uhwDelayMS(90, unit->hu_TimerReq);
            pciusbEHCIDebug("EHCI", "Port status (after handover)=%08lx\n", READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND));
            // enable companion controller port
            switch(chc->hc_HCIType) {
            case HCITYPE_UHCI: {
                UWORD uhcihciport = unit->hu_PortNum11[idx - 1];
                UWORD uhciportreg = uhcihciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                ULONG uhcinewval;

                uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                pciusbEHCIDebug("EHCI", "UHCI Reset=%s\n", uhcinewval & UHPF_PORTRESET ? "BAD!" : "GOOD");
                if((uhcinewval & UHPF_PORTRESET)) { //|| (newval & EHPF_LINESTATUS_DM))
                    // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
                    pciusbWarn("EHCI", "Uhm, UHCI reset was bad!\n");
                    uhcinewval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                    uhcinewval |= UHPF_PORTRESET;
                    WRITEIO16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                    uhwDelayMS(50, unit->hu_TimerReq);
                    uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                    pciusbEHCIDebug("EHCI", "UHCI Re-Reset=%s\n", uhcinewval & UHPF_PORTRESET ? "GOOD" : "BAD!");
                    uhcinewval &= ~UHPF_PORTRESET;
                    WRITEIO16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                    uhwDelayMicro(50, unit->hu_TimerReq);
                    uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                    pciusbEHCIDebug("EHCI", "UHCI Re-Reset=%s\n", uhcinewval & UHPF_PORTRESET ? "STILL BAD!" : "GOOD");
                }
                uhcinewval &= ~UHPF_PORTRESET;
                uhcinewval |= UHPF_PORTENABLE;
                WRITEIO16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                chc->hc_PortChangeMap[uhcihciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change
                uhwDelayMS(5, unit->hu_TimerReq);
                cnt = 100;
                do {
                    uhwDelayMS(1, unit->hu_TimerReq);
                    uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg);
                } while(--cnt && (!(uhcinewval & UHPF_PORTENABLE)));
                if(cnt) {
                    pciusbEHCIDebug("EHCI", "UHCI Enabled after %ld ticks\n", 100-cnt);
                } else {
                    pciusbWarn("EHCI", "UHCI Port refuses to be enabled!\n");
                    *retval = UHIOERR_HOSTERROR;
                    return TRUE;
                }
                break;
            }

            case HCITYPE_OHCI: {
                UWORD ohcihciport = unit->hu_PortNum11[idx - 1];
                UWORD ohciportreg = OHCI_PORTSTATUS + (ohcihciport<<2);
                ULONG ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);
                pciusbEHCIDebug("EHCI", "OHCI Resetting Port (%s)\n", ohcioldval & OHPF_PORTRESET ? "already" : "ok");
                // make sure we have at least 50ms of reset time here, as required for a root hub port
                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                uhwDelayMS(10, unit->hu_TimerReq);
                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                uhwDelayMS(10, unit->hu_TimerReq);
                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                uhwDelayMS(10, unit->hu_TimerReq);
                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                uhwDelayMS(10, unit->hu_TimerReq);
                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                uhwDelayMS(15, unit->hu_TimerReq);
                ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);
                pciusbEHCIDebug("EHCI", "OHCI Reset release (%s %s)\n", ohcioldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                            ohcioldval & OHPF_PORTENABLE ? "enabled" : "not enabled");
                if(ohcioldval & OHPF_PORTRESET) {
                    uhwDelayMS(40, unit->hu_TimerReq);
                    ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);
                    pciusbEHCIDebug("EHCI", "OHCI Reset 2nd release (%s %s)\n", ohcioldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                                ohcioldval & OHPF_PORTENABLE ? "enabled" : "still not enabled");
                }
                break;
            }

            }
            // make enumeration possible
            unit->hu_DevControllers[0] = chc;
            return  TRUE;
        } else {
            newval &= ~EHPF_PORTRESET;
            WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET; // manually fake reset change
            uhwDelayMS(10, unit->hu_TimerReq);
            cnt = 100;
            do {
                uhwDelayMS(1, unit->hu_TimerReq);
                newval = READREG32_LE(hc->hc_RegBase, portreg);
            } while(--cnt && (!(newval & EHPF_PORTENABLE)));
            if(cnt) {
                pciusbEHCIDebug("EHCI", "Enabled after %ld ticks\n", 100-cnt);
            } else {
                pciusbWarn("EHCI", "Port refuses to be enabled!\n");
                *retval = UHIOERR_HOSTERROR;
                return TRUE;
            }
            // make enumeration possible
            unit->hu_DevControllers[0] = hc;
        }
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        pciusbEHCIDebug("EHCI", "Powering Port\n");
        newval |= EHPF_PORTPOWER;
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
        pciusbEHCIDebug("EHCI", "Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval);
        WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
    }
    return cmdgood;
}

BOOL ehciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE); // these are clear-on-write!
    ULONG newval = oldval;
    BOOL cmdgood = FALSE;

    pciusbEHCIDebug("EHCI", "%s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)\n", __func__, unit, hc, hciport, idx, val, retval);

    switch(val) {
    case UFS_PORT_ENABLE:
        pciusbEHCIDebug("EHCI", "Disabling Port (%s)\n", newval & EHPF_PORTENABLE ? "ok" : "already");
        newval &= ~EHPF_PORTENABLE;
        cmdgood = TRUE;
        // disable enumeration
        unit->hu_DevControllers[0] = NULL;
        break;

    case UFS_PORT_SUSPEND:
        newval &= ~EHPF_PORTSUSPEND;
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER: // ignore for UHCI, there's no power control here
        pciusbEHCIDebug("EHCI", "Disabling Power (%s)\n", newval & EHPF_PORTPOWER ? "ok" : "already");
        pciusbEHCIDebug("EHCI", "Disabling Port (%s)\n", newval & EHPF_PORTENABLE ? "ok" : "already");
        newval &= ~(EHPF_PORTENABLE|EHPF_PORTPOWER);
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_CONNECTION:
        newval |= EHPF_CONNECTCHANGE; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_ENABLE:
        newval |= EHPF_ENABLECHANGE; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_SUSPEND: // ignore for EHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change clearing
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_OVER_CURRENT:
        newval |= EHPF_OVERCURRENTCHG; // clear-on-write!
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT; // manually fake over current clearing
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_RESET: // ignore for EHCI, there's no bit indicating this
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET; // manually fake reset change clearing
        cmdgood = TRUE;
        break;
    }
    if(cmdgood) {
        pciusbEHCIDebug("EHCI", "Port %ld CLEAR_FEATURE %08lx->%08lx\n", idx, oldval, newval);
        WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
        if(hc->hc_PortChangeMap[hciport]) {
            unit->hu_RootPortChanges |= 1UL<<idx;
        } else {
            unit->hu_RootPortChanges &= ~(1UL<<idx);
        }
    }
    return cmdgood;
}

BOOL ehciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval)
{
    UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

    pciusbEHCIDebug("EHCI", "%s(0x%p, 0x%p, %04x, %04x, 0x%p)\n", __func__, hc, mptr, hciport, idx, retval);

    *mptr = 0;
    if(oldval & EHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    if(oldval & EHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE|UPSF_PORT_HIGH_SPEED);
    if((oldval & (EHPF_LINESTATUS_DM|EHPF_PORTCONNECTED|EHPF_PORTENABLE)) ==
            (EHPF_LINESTATUS_DM|EHPF_PORTCONNECTED)) {
        pciusbEHCIDebug("EHCI", "Port %ld is LOWSPEED\n", idx);
        // we need to detect low speed devices prior to reset
        *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
    }

    if(oldval & EHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    if(oldval & EHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);
    if(oldval & EHPF_PORTPOWER) *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    if(oldval & EHPM_PORTINDICATOR) *mptr |= AROS_WORD2LE(UPSF_PORT_INDICATOR);

    pciusbEHCIDebug("EHCI", "Port %ld Status %08lx\n", idx, *mptr);

    mptr++;
    if(oldval & EHPF_ENABLECHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if(oldval & EHPF_CONNECTCHANGE) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if(oldval & EHPF_RESUMEDTX) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
    }
    if(oldval & EHPF_OVERCURRENTCHG) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
    WRITEREG32_LE(hc->hc_RegBase, portreg, oldval);

    pciusbEHCIDebug("EHCI", "Port %ld Change %08lx\n", idx, *mptr);

    return TRUE;
}

#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#undef base
