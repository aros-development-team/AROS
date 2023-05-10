/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver debug functions
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

#if !defined(AROS_USE_LOGRES)
//#define XHCI_ENABLEINDEBUG
//#define XHCI_ENABLESLOTDEBUG
//#define XHCI_ENABLEEPDEBUG
//#define XHCI_ENABLEIRDEBUG
//#define XHCI_ENABLEPORTDEBUG
//#define XHCI_ENABLECCDEBUG
#endif

/* Debug functions */
#if defined(DEBUG) && (DEBUG > 0)

void xhciDumpIN(volatile struct xhci_inctx *in)
{
#if defined(XHCI_ENABLEINDEBUG)
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IN.DCF: %08x" DEBUGCOLOR_RESET" \n", in->dcf & ~0x3));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IN.ACF: %08x" DEBUGCOLOR_RESET" \n", in->acf));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IN.CFG = %02x" DEBUGCOLOR_RESET" \n", in->rsvd1[5] & 0xFF));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IN.INTERFACE = %02x" DEBUGCOLOR_RESET" \n", (in->rsvd1[5] >> 8) & 0xFF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: IN.ALT = %02x" DEBUGCOLOR_RESET" \n", (in->rsvd1[5] >> 16) & 0xFF));
#endif
}

void xhciDumpSlot(volatile struct xhci_slot *slot)
{
#if defined(XHCI_ENABLESLOTDEBUG)
    switch ((slot->ctx[3] >> 27) & 0xF)
    {
        case 0:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.STATE = Disabled/Enabled" DEBUGCOLOR_RESET" \n"));
            break;
        case 1:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.STATE = Default" DEBUGCOLOR_RESET" \n"));
            break;
        case 2:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.STATE = Addressed" DEBUGCOLOR_RESET" \n"));
            break;
        case 3:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.STATE = Configured" DEBUGCOLOR_RESET" \n"));
            break;
        default:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.STATE = Unknown (%x)" DEBUGCOLOR_RESET" \n", (slot->ctx[3] >> 27) & 0xF));
            break;
    }
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.PORT         = %02x" DEBUGCOLOR_RESET" \n", (slot->ctx[1] >> 16) & 0xFF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: SLOT.TARGET      = %03x" DEBUGCOLOR_RESET" \n", (slot->ctx[3] >> 22) & 0x3FF));
#endif
}

void xhciDumpEP(volatile struct xhci_ep *ep)
{
#if defined(XHCI_ENABLEEPDEBUG)
    // Endpoint info..
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.STATE = %x" DEBUGCOLOR_RESET" \n", ep->ctx[0] & 0x7));
    switch (ep->ctx[0] & 0x7)
    {
        case 0:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Disabled" DEBUGCOLOR_RESET" \n"));
            break;
        case 1:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Running" DEBUGCOLOR_RESET" \n"));
            break;
        case 2:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Halted" DEBUGCOLOR_RESET" \n"));
            break;
        case 3:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Stopped" DEBUGCOLOR_RESET" \n"));
            break;
        case 4:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Error" DEBUGCOLOR_RESET" \n"));
            break;
        default:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Unknown (%x)" DEBUGCOLOR_RESET" \n", ep->ctx[0] & 0x7));
            break;
    }
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.MULT = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[0] >> 8) & 0x3));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.MAXPSTREAMS = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[0] >> 10) & 0xF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.LSA = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[0] >> 15) & 0x1));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.INTERVAL = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[0] >> 16) & 0xFF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.MAXESIT = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[0] >> 24) & 0xFF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.CERR = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[1] >> 1) & 0x3));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.TYPE = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[1] >> 3) & 0x7));
    switch ((ep->ctx[1] >> 3) & 0x7)
    {
        case 1:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > OUT Isoch" DEBUGCOLOR_RESET" \n"));
            break;
        case 2:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > OUT Bulk" DEBUGCOLOR_RESET" \n"));
            break;
        case 3:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > OUT Int" DEBUGCOLOR_RESET" \n"));
            break;
        case 4:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > Control" DEBUGCOLOR_RESET" \n"));
            break;
        case 5:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > IN Isoch" DEBUGCOLOR_RESET" \n"));
            break;
        case 6:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > IN Bulk" DEBUGCOLOR_RESET" \n"));
            break;
        case 7:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > IN Int" DEBUGCOLOR_RESET" \n"));
            break;
        default:
            KPRINTF(10, (DEBUGCOLOR_SET "xHCI: > INVALID" DEBUGCOLOR_RESET" \n"));
            break;
    }
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.HID = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[1] >> 7) & 0x1));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.MAXBURST = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[1] >> 8) & 0xFF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.MAXPACKET = %x" DEBUGCOLOR_RESET" \n", (ep->ctx[1] >> 16) & 0xFF));
    KPRINTF(10, (DEBUGCOLOR_SET "xHCI: EP.DCS = %x" DEBUGCOLOR_RESET" \n", ep->deq.addr_lo  & 0x1));
#endif
}

void xhciDumpStatus(ULONG status)
{
#if defined(XHCI_ENABLESTATUSDEBUG)
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.STATUS: %08x" DEBUGCOLOR_RESET" \n", status));
    if (status & XHCIF_USBSTS_HCE)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Host Controller Error" DEBUGCOLOR_RESET" \n"));
    if (status & XHCIF_USBSTS_HSE)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Host System Error" DEBUGCOLOR_RESET" \n"));
    if (status & XHCIF_USBSTS_PCD)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Port Change Detected" DEBUGCOLOR_RESET" \n"));    
#endif
}

void xhciDumpOpR(volatile struct xhci_hcopr *hcopr)
{
#if defined(XHCI_ENABLEOPRDEBUG)
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.COMMAND: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd)));
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.PAGESIZE: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->pagesize)));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.DNCTRL: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->dnctl)));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.CRCR: %08x%08x" DEBUGCOLOR_RESET" \n", hcopr->crcr.addr_hi, hcopr->crcr.addr_lo));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.DCBAAP: %08x%08x" DEBUGCOLOR_RESET" \n", hcopr->dcbaap.addr_hi, hcopr->dcbaap.addr_lo));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: OPR.CONFIG: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->config)));
#endif
}

void xhciDumpIMAN(ULONG iman)
{
#if defined(XHCI_ENABLEIMANDEBUG)
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IR.IMAN: %08x" DEBUGCOLOR_RESET" \n", iman));
    if (iman & XHCIF_IR_IMAN_IE)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Interrupts enabled" DEBUGCOLOR_RESET" \n"));
    if (iman & XHCIF_IR_IMAN_IP)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Interrupts pending" DEBUGCOLOR_RESET" \n"));    
#endif
}

void xhciDumpIR(volatile struct xhci_ir *xhciir)
{
#if defined(XHCI_ENABLEIRDEBUG)
    xhciDumpIMAN(xhciir->iman);
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IR.IMOD: %08x" DEBUGCOLOR_RESET" \n", xhciir->imod));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IR.ERSTSZ: %08x" DEBUGCOLOR_RESET" \n", xhciir->erstsz));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IR.ERDP: %08x%08x" DEBUGCOLOR_RESET" \n", xhciir->erdp.addr_hi, xhciir->erdp.addr_lo));
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: IR.ERSTBA: %08x%08x" DEBUGCOLOR_RESET" \n", xhciir->erstba.addr_hi, xhciir->erstba.addr_lo));
#endif
}

void xhciDumpPort(volatile struct xhci_pr *xhcipr)
{
#if defined(XHCI_ENABLEPORTDEBUG)
    ULONG portsc = AROS_LE2LONG(xhcipr->portsc);
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: PR.PORTSC = $%08x" DEBUGCOLOR_RESET" \n", portsc));
    if (portsc & XHCIF_PR_PORTSC_PP)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Powered" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_PED)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Enabled" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_CCS)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Connected" DEBUGCOLOR_RESET" \n"));

    if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_FULLSPEED)
    {
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Full Speed" DEBUGCOLOR_RESET" \n"));
    }
    else if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_LOWSPEED)
    {
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Low Speed" DEBUGCOLOR_RESET" \n"));
    }
    else if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_HIGHSPEED)
    {
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > High Speed" DEBUGCOLOR_RESET" \n"));
    }
    else if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_SUPERSPEED)
    {
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Super Speed" DEBUGCOLOR_RESET" \n"));
    }

    if (portsc & XHCIF_PR_PORTSC_OCA)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Over Current Active" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_PR)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Port Reset" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_LWS)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Link State Write Strobe" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_CSC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Connect Status Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_PEC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Enable/Disable Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_WRC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Warm Reset Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_OCC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Over Current Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_PRC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Reset Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_PLC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Link State Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_CEC)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Config Error Changed" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_CAS)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Cold Attach Status" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_WCE)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Wake on Connect" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_WDE)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Wake on Disconnect" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_WOE)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Wake on Over Current" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_DR)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Device Removable" DEBUGCOLOR_RESET" \n"));

    if (portsc & XHCIF_PR_PORTSC_WPR)
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Warm Reset" DEBUGCOLOR_RESET" \n"));
#endif
}

void xhciDumpCC(UBYTE cc)
{
#if defined(XHCI_ENABLECCDEBUG)
    KPRINTF(20, (DEBUGCOLOR_SET "xHCI: CC = $%02x" DEBUGCOLOR_RESET" \n", cc));
    switch (cc)
    {
    case 0:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Invalid" DEBUGCOLOR_RESET" \n"));
        break;
    case 1:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Success" DEBUGCOLOR_RESET" \n"));
        break;
    case 2:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Data Buffer Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 3:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Babble Detected Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 4:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > USB Transaction Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 5:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > TRB Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 6:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Stall Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 7:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Resource Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 8:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Bandwidth Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 9:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > No Slots Available Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 10:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Invalid Stream Type Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 11:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Slot Not Enabled Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 12:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Endpoint Not Enabled Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 13:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Short Packet" DEBUGCOLOR_RESET" \n"));
        break;
    case 14:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Ring Underrun" DEBUGCOLOR_RESET" \n"));
        break;
    case 15:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Ring Overrun" DEBUGCOLOR_RESET" \n"));
        break;
    case 16:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > VF Event Ring Full" DEBUGCOLOR_RESET" \n"));
        break;
    case 17:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Parameter Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 18:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Bandwidth Overrun Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 19:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Context State Error" DEBUGCOLOR_RESET" \n"));
        break;
    case 20:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > No Ping Response Error" DEBUGCOLOR_RESET" \n"));
        break;
    default:
        KPRINTF(20, (DEBUGCOLOR_SET "xHCI: > Unknown CC" DEBUGCOLOR_RESET" \n"));
        break;
    };
#endif
}
#endif
#endif /* PCIUSB_ENABLEXHCI */
