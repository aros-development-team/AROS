/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver hc port support functions
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

#undef base
#define base (hc->hc_Device)

#if defined(DEBUG) && defined(XHCI_LONGDEBUGNAK)
#define XHCI_NAKTOSHIFT         (8)
#else
#define XHCI_NAKTOSHIFT         (3)
#endif

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

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    volatile struct xhci_hccapr *xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;
    volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
    BOOL cmdgood = FALSE;
    ULONG oldval = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG newval = 0, tmpval;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, unit, hc, hciport, idx, val, retval);

    xhciDumpPort(&xhciports[hciport]);

    switch(val)
    {
        case UFS_PORT_ENABLE:
            /* only the HC can enable the port */
            if (!(oldval & XHCIF_PR_PORTSC_PED))
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: >Setting Reset to enable port" DEBUGCOLOR_RESET" \n");
                /* Reset the port */
                newval |= XHCIF_PR_PORTSC_PR;
            }
            else
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Port allready enabled" DEBUGCOLOR_RESET" \n");
            }
            cmdgood = TRUE;
            break;

        case UFS_PORT_SUSPEND:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Suspending Port" DEBUGCOLOR_RESET" \n");
            cmdgood = TRUE;
            break;

        /* case UFS_PORT_OVER_CURRENT: not possible */

        case UFS_PORT_RESET:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Performing Port Reset-:" DEBUGCOLOR_RESET" \n");
#if (1)
            if (oldval & XHCIF_PR_PORTSC_PED)
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI:     >Setting Port Power Off" DEBUGCOLOR_RESET" \n");
                newval |= XHCIF_PR_PORTSC_PED;
                do {
                    xhciports[hciport].portsc = AROS_LONG2LE(newval);
                    tmpval = AROS_LE2LONG(xhciports[hciport].portsc);
                    uhwDelayMS(100, unit);
                } while (tmpval & XHCIF_PR_PORTSC_PED);
                newval &= ~XHCIF_PR_PORTSC_PED;
            }
            if (AROS_LE2LONG(xhciports[hciport].portsc) & XHCIF_PR_PORTSC_PED)
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: PED still asserted?!?!" DEBUGCOLOR_RESET" \n");
            }
            else if (hc->hc_Devices[hciport])
            {
                struct pcisusbXHCIDevice *devCtx = hc->hc_Devices[hciport];

                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Disabling device slot (%u) and freeing resources.." DEBUGCOLOR_RESET" \n", devCtx->dc_SlotID);

                xhciCmdSlotDisable(hc, devCtx->dc_SlotID);
                hc->hc_Devices[hciport] = NULL;
#if (1)
                int epn;
                //TODO: Offload to support task..
                for (epn = 0; epn < MAX_DEVENDPOINTS; epn ++)
                {
                    if (devCtx->dc_EPAllocs[epn].dmaa_Ptr)
                    {
                        // Free Endpoint resources ...
                        FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_EPAllocs[epn].dmaa_Entry.me_Un.meu_Addr);
                    }
                }
                FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr);
                FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_SlotCtx.dmaa_Entry.me_Un.meu_Addr);
                FreeMem(devCtx, sizeof(struct pcisusbXHCIDevice));                
#endif
            }
            newval |= XHCIF_PR_PORTSC_PR;
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI:     >Setting Reset" DEBUGCOLOR_RESET" \n");
#endif
            cmdgood = TRUE;
            break;

        case UFS_PORT_POWER:
            if (!(oldval & XHCIF_PR_PORTSC_PP))
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Powering Port" DEBUGCOLOR_RESET" \n");
                newval |= XHCIF_PR_PORTSC_PP;
            }
            cmdgood = TRUE;
            break;
    }
    if(cmdgood)
    {
        if (newval)
            xhciports[hciport].portsc = AROS_LONG2LE(newval);
        newval = AROS_LE2LONG(xhciports[hciport].portsc);
        pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Port %ld SET_FEATURE $%08lx->$%08lx" DEBUGCOLOR_RESET" \n", idx, oldval, newval);
    }
    return cmdgood;
}

BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc, UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    volatile struct xhci_hccapr *xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;
    volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
    ULONG oldval = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG newval = 0;
    BOOL cmdgood = FALSE;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, unit, hc, hciport, idx, val, retval);

    xhciDumpPort(&xhciports[hciport]);

    switch(val)
    {
        case UFS_PORT_ENABLE:
            // Setting PED disables the port.
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Disabling Port" DEBUGCOLOR_RESET" \n");
            newval |= XHCIF_PR_PORTSC_PED;
            cmdgood = TRUE;
            // disable enumeration
            unit->hu_DevControllers[0] = NULL;
            break;

        case UFS_PORT_SUSPEND:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Suspending Port" DEBUGCOLOR_RESET" \n");
            cmdgood = TRUE;
            break;

        case UFS_PORT_POWER:
            if (!(oldval & XHCIF_PR_PORTSC_PP))
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Powering Down Port" DEBUGCOLOR_RESET" \n");
                newval |= XHCIF_PR_PORTSC_PP;
            }      
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_CONNECTION:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Clearing Connection Change" DEBUGCOLOR_RESET" \n");
            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_ENABLE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Clearing Enable Change" DEBUGCOLOR_RESET" \n");
            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_SUSPEND:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Clearing Suspend Change" DEBUGCOLOR_RESET" \n");
            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND;
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_OVER_CURRENT:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Clearing Over-Current Change" DEBUGCOLOR_RESET" \n");
            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT;
            cmdgood = TRUE;
            break;

        case UFS_C_PORT_RESET:
            pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Clearing Reset Change" DEBUGCOLOR_RESET" \n");
            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET;
            cmdgood = TRUE;
            break;
    }
    if(cmdgood)
    {
        if (newval)
            xhciports[hciport].portsc = AROS_LONG2LE(newval);
        newval = AROS_LE2LONG(xhciports[hciport].portsc);
        pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Port %ld CLEAR_FEATURE $%08lx->$%08lx" DEBUGCOLOR_RESET" \n", idx, oldval, newval);
    }
    return cmdgood;
}

BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr, UWORD hciport, UWORD idx, WORD *retval)
{
    volatile struct xhci_hccapr *xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;
    volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
    ULONG oldportsc = AROS_LE2LONG(xhciports[hciport].portsc);

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, mptr, hciport, idx, retval);
    
    pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: %s: xhci-portsc = $%08x" DEBUGCOLOR_RESET" \n", __func__, oldportsc);
    xhciDumpPort(&xhciports[hciport]);

    *mptr = 0;
    if (oldportsc & XHCIF_PR_PORTSC_PP) *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    if(oldportsc & XHCIF_PR_PORTSC_OCA) *mptr |= AROS_WORD2LE(UPSF_PORT_OVER_CURRENT);
    if (oldportsc & XHCIF_PR_PORTSC_CCS) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    if (oldportsc & XHCIF_PR_PORTSC_PED) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
    if ((oldportsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_LOWSPEED)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
    }
    else if ((oldportsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_HIGHSPEED)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_HIGH_SPEED);
    }
    else if ((oldportsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_SUPERSPEED)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_SUPER_SPEED);
    }
    else
    {
        // Fullspeed
    }

    if(oldportsc & XHCIF_PR_PORTSC_PR) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    if (((oldportsc >> XHCIS_PR_PORTSC_PLS) & XHCI_PR_PORTSC_PLS_SMASK) == 2)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);
    }
    if ((oldportsc >> 14) & 0x3) *mptr |= AROS_WORD2LE(UPSF_PORT_INDICATOR);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Port %ld Status $%08lx" DEBUGCOLOR_RESET" \n", idx, *mptr);

    mptr++;
    if(oldportsc & XHCIF_PR_PORTSC_PEC)
    {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if(oldportsc & XHCIF_PR_PORTSC_CSC)
    {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if(oldportsc & XHCIF_PR_PORTSC_PLC)
    {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
    }
    if(oldportsc & XHCIF_PR_PORTSC_OCC)
    {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
    }
    if ((oldportsc & XHCIF_PR_PORTSC_WRC) || (oldportsc & XHCIF_PR_PORTSC_PRC))
    {
        hc->hc_PortChangeMap[hciport] |= (UPSF_PORT_RESET);
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "xHCI: Port %ld Change $%08lx" DEBUGCOLOR_RESET" \n", idx, *mptr);

    return FALSE;
}
#endif /* PCIUSB_ENABLEXHCI */
