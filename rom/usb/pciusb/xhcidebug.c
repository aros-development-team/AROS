/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

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

/* Debug functions */
#if defined(PCIUSB_XHCI_DEBUG)

#undef base
#define base (hc->hc_Device)

#if !defined(AROS_USE_LOGRES)
#if defined(DEBUG) && (DEBUG > 1)
#define XHCI_ENABLEINDEBUG
//#define XHCI_ENABLESLOTDEBUG
#define XHCI_ENABLEEPDEBUG
#define XHCI_ENABLESTATUSDEBUG
//#define XHCI_ENABLEOPRDEBUG
//#define XHCI_ENABLEIMANDEBUG
//#define XHCI_ENABLEIRDEBUG
#define XHCI_ENABLEPORTDEBUG
//#define XHCI_ENABLECCDEBUG
#endif
#endif

void xhciDumpIN(volatile struct xhci_inctx *in)
{
#if defined(XHCI_ENABLEINDEBUG)
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.DCF: %08x" DEBUGCOLOR_RESET" \n", in->dcf & ~0x3);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.ACF: %08x" DEBUGCOLOR_RESET" \n", in->acf);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.CFG = %02x" DEBUGCOLOR_RESET" \n", in->rsvd1[5] & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.INTERFACE = %02x" DEBUGCOLOR_RESET" \n", (in->rsvd1[5] >> 8) & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.ALT = %02x" DEBUGCOLOR_RESET" \n", (in->rsvd1[5] >> 16) & 0xFF);
#endif
}

void xhciDumpSlot(volatile struct xhci_slot *slot)
{
#if defined(XHCI_ENABLESLOTDEBUG)
    ULONG state = (slot->ctx[3] >> 27) & 0x1F;
    switch (state) {
    case 0:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.STATE = Disabled" DEBUGCOLOR_RESET" \n");
        break;
    case 1:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.STATE = Default" DEBUGCOLOR_RESET" \n");
        break;
    case 2:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.STATE = Addressed" DEBUGCOLOR_RESET" \n");
        break;
    case 3:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.STATE = Configured" DEBUGCOLOR_RESET" \n");
        break;
    default:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.STATE = Unknown (%02x)" DEBUGCOLOR_RESET" \n", state);
        break;
    }
    KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.PORT         = %02x" DEBUGCOLOR_RESET" \n", (slot->ctx[1] >> 16) & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: SLOT.TARGET      = %03x" DEBUGCOLOR_RESET" \n", (slot->ctx[3] >> 22) & 0x3FF);
#endif
}

void xhciDumpEP(volatile struct xhci_ep *ep)
{
#if defined(XHCI_ENABLEEPDEBUG)
    ULONG ctx0 = ep->ctx[0];
    ULONG ctx1 = ep->ctx[1];

    /* Endpoint state (DW0 bits [2:0]) */
    UBYTE state = (ctx0 & 0x7);

    KPrintF(DEBUGCOLOR_SET "xHCI: EP.STATE = %x" DEBUGCOLOR_RESET" \n", state);
    switch (state) {
    case 0:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Disabled" DEBUGCOLOR_RESET" \n");
        break;
    case 1:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Running" DEBUGCOLOR_RESET" \n");
        break;
    case 2:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Halted" DEBUGCOLOR_RESET" \n");
        break;
    case 3:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Stopped" DEBUGCOLOR_RESET" \n");
        break;
    case 4:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Error" DEBUGCOLOR_RESET" \n");
        break;
    default:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Unknown (%x)" DEBUGCOLOR_RESET" \n", state);
        break;
    }

    /* DW0 fields */
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.MULT = %x" DEBUGCOLOR_RESET" \n",
            (ctx0 >> EPS_CTX_MULT) & 0x3);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.MAXPSTREAMS = %x" DEBUGCOLOR_RESET" \n",
            (ctx0 >> 10) & 0x1F);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.LSA = %x" DEBUGCOLOR_RESET" \n",
            (ctx0 >> 15) & 0x1);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.INTERVAL = %x" DEBUGCOLOR_RESET" \n",
            (ctx0 >> 16) & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.MAXESIT = %x" DEBUGCOLOR_RESET" \n",
            (ctx0 >> 24) & 0xFF);

    /* DW1: CErr, Type, HID, MaxBurst, MaxPacketSize */
    UBYTE cerr = (ctx1 >> EPS_CTX_CERR) & EP_CTX_CERR_MASK;
    UBYTE type = (ctx1 >> EPS_CTX_TYPE) & 0x7;
    UBYTE hid  = (ctx1 >> 7) & 0x1;
    UBYTE maxburst = (ctx1 >> 8) & 0xFF;
    UWORD maxpacket = (ctx1 >> EPS_CTX_PACKETMAX) & 0xFFFF;

    KPrintF(DEBUGCOLOR_SET "xHCI: EP.CERR = %x" DEBUGCOLOR_RESET" \n", cerr);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.TYPE = %x" DEBUGCOLOR_RESET" \n", type);
    switch (type) {
    case 1:
        KPrintF(DEBUGCOLOR_SET "xHCI: > OUT Isoch" DEBUGCOLOR_RESET" \n");
        break;
    case 2:
        KPrintF(DEBUGCOLOR_SET "xHCI: > OUT Bulk" DEBUGCOLOR_RESET" \n");
        break;
    case 3:
        KPrintF(DEBUGCOLOR_SET "xHCI: > OUT Int" DEBUGCOLOR_RESET" \n");
        break;
    case 4:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Control" DEBUGCOLOR_RESET" \n");
        break;
    case 5:
        KPrintF(DEBUGCOLOR_SET "xHCI: > IN Isoch" DEBUGCOLOR_RESET" \n");
        break;
    case 6:
        KPrintF(DEBUGCOLOR_SET "xHCI: > IN Bulk" DEBUGCOLOR_RESET" \n");
        break;
    case 7:
        KPrintF(DEBUGCOLOR_SET "xHCI: > IN Int" DEBUGCOLOR_RESET" \n");
        break;
    default:
        KPrintF(DEBUGCOLOR_SET "xHCI: > INVALID" DEBUGCOLOR_RESET" \n");
        break;
    }

    KPrintF(DEBUGCOLOR_SET "xHCI: EP.HID = %x" DEBUGCOLOR_RESET" \n", hid);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.MAXBURST = %x" DEBUGCOLOR_RESET" \n", maxburst);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.MAXPACKET = %x" DEBUGCOLOR_RESET" \n", maxpacket);

    /* Dequeue pointer DCS bit */
    ULONG deq_lo = AROS_LE2LONG(ep->deq.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: EP.DCS = %x" DEBUGCOLOR_RESET" \n",
            deq_lo & EPF_CTX_DEQ_DCS);
#endif
}

void xhciDumpStatus(ULONG status)
{
#if defined(XHCI_ENABLESTATUSDEBUG)
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.STATUS: %08lx" DEBUGCOLOR_RESET" \n", status);

    if (status & XHCIF_USBSTS_HCH)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Host Controller Halted" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_HSE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Host System Error" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_HCE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Host Controller Error" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_EINT)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Event Interrupt Pending" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_PCD)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Port Change Detect" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_SSS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Save State Status" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_RSS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Restore State Status" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_SRE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Save/Restore Error" DEBUGCOLOR_RESET" \n");
    if (status & XHCIF_USBSTS_CNR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Controller Not Ready" DEBUGCOLOR_RESET" \n");
#endif
}

void xhciDumpOpR(volatile struct xhci_hcopr *hcopr)
{
#if defined(XHCI_ENABLEOPRDEBUG)
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.COMMAND: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd));
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.PAGESIZE: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->pagesize));
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.DNCTRL: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->dnctl));
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.CRCR: %08x%08x" DEBUGCOLOR_RESET" \n", hcopr->crcr.addr_hi, hcopr->crcr.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.DCBAAP: %08x%08x" DEBUGCOLOR_RESET" \n", hcopr->dcbaap.addr_hi, hcopr->dcbaap.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.CONFIG: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->config));
#endif
}

void xhciDumpIMAN(ULONG iman)
{
#if defined(XHCI_ENABLEIMANDEBUG)
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.IMAN: %08x" DEBUGCOLOR_RESET" \n", iman);
    if (iman & XHCIF_IR_IMAN_IE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Interrupts enabled" DEBUGCOLOR_RESET" \n");
    if (iman & XHCIF_IR_IMAN_IP)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Interrupts pending" DEBUGCOLOR_RESET" \n");
#endif
}

void xhciDumpIR(volatile struct xhci_ir *xhciir)
{
#if defined(XHCI_ENABLEIRDEBUG)
    xhciDumpIMAN(xhciir->iman);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.IMOD: %08x" DEBUGCOLOR_RESET" \n", xhciir->imod);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.ERSTSZ: %08x" DEBUGCOLOR_RESET" \n", xhciir->erstsz);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.ERDP: %08x%08x" DEBUGCOLOR_RESET" \n", xhciir->erdp.addr_hi, xhciir->erdp.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.ERSTBA: %08x%08x" DEBUGCOLOR_RESET" \n", xhciir->erstba.addr_hi, xhciir->erstba.addr_lo);
#endif
}

void xhciDumpPort(volatile struct xhci_pr *xhcipr)
{
#if defined(XHCI_ENABLEPORTDEBUG)
    ULONG portsc = AROS_LE2LONG(xhcipr->portsc);
    KPrintF(DEBUGCOLOR_SET "xHCI: PR.PORTSC = $%08x" DEBUGCOLOR_RESET" \n", portsc);
    if (portsc & XHCIF_PR_PORTSC_PP)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Powered" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_PED)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Enabled" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_CCS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Connected" DEBUGCOLOR_RESET" \n");

    if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_FULLSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > Full Speed" DEBUGCOLOR_RESET" \n");
    } else if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_LOWSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > Low Speed" DEBUGCOLOR_RESET" \n");
    } else if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_HIGHSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > High Speed" DEBUGCOLOR_RESET" \n");
    } else if ((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_SUPERSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > Super Speed" DEBUGCOLOR_RESET" \n");
    }

    if (portsc & XHCIF_PR_PORTSC_OCA)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Over Current Active" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_PR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Port Reset" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_LWS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Link State Write Strobe" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_CSC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Connect Status Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_PEC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Enable/Disable Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_WRC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Warm Reset Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_OCC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Over Current Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_PRC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Reset Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_PLC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Link State Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_CEC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Config Error Changed" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_CAS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Cold Attach Status" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_WCE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Wake on Connect" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_WDE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Wake on Disconnect" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_WOE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Wake on Over Current" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_DR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Device Removable" DEBUGCOLOR_RESET" \n");

    if (portsc & XHCIF_PR_PORTSC_WPR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Warm Reset" DEBUGCOLOR_RESET" \n");
#endif
}

void xhciDumpCC(UBYTE cc)
{
#if defined(XHCI_ENABLECCDEBUG)
    KPrintF(DEBUGCOLOR_SET "xHCI: CC = $%02x" DEBUGCOLOR_RESET" \n", cc);
    switch (cc) {
    case 0:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Invalid" DEBUGCOLOR_RESET" \n");
        break;
    case 1:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Success" DEBUGCOLOR_RESET" \n");
        break;
    case 2:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Data Buffer Error" DEBUGCOLOR_RESET" \n");
        break;
    case 3:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Babble Detected Error" DEBUGCOLOR_RESET" \n");
        break;
    case 4:
        KPrintF(DEBUGCOLOR_SET "xHCI: > USB Transaction Error" DEBUGCOLOR_RESET" \n");
        break;
    case 5:
        KPrintF(DEBUGCOLOR_SET "xHCI: > TRB Error" DEBUGCOLOR_RESET" \n");
        break;
    case 6:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Stall Error" DEBUGCOLOR_RESET" \n");
        break;
    case 7:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Resource Error" DEBUGCOLOR_RESET" \n");
        break;
    case 8:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Bandwidth Error" DEBUGCOLOR_RESET" \n");
        break;
    case 9:
        KPrintF(DEBUGCOLOR_SET "xHCI: > No Slots Available Error" DEBUGCOLOR_RESET" \n");
        break;
    case 10:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Invalid Stream Type Error" DEBUGCOLOR_RESET" \n");
        break;
    case 11:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Slot Not Enabled Error" DEBUGCOLOR_RESET" \n");
        break;
    case 12:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Endpoint Not Enabled Error" DEBUGCOLOR_RESET" \n");
        break;
    case 13:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Short Packet" DEBUGCOLOR_RESET" \n");
        break;
    case 14:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Ring Underrun" DEBUGCOLOR_RESET" \n");
        break;
    case 15:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Ring Overrun" DEBUGCOLOR_RESET" \n");
        break;
    case 16:
        KPrintF(DEBUGCOLOR_SET "xHCI: > VF Event Ring Full" DEBUGCOLOR_RESET" \n");
        break;
    case 17:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Parameter Error" DEBUGCOLOR_RESET" \n");
        break;
    case 18:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Bandwidth Overrun Error" DEBUGCOLOR_RESET" \n");
        break;
    case 19:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Context State Error" DEBUGCOLOR_RESET" \n");
        break;
    case 20:
        KPrintF(DEBUGCOLOR_SET "xHCI: > No Ping Response Error" DEBUGCOLOR_RESET" \n");
        break;
    default:
        KPrintF(DEBUGCOLOR_SET "xHCI: > Unknown CC" DEBUGCOLOR_RESET" \n");
        break;
    };
#endif
}

static UQUAD xhciDebugReadAddress(const volatile struct xhci_address *addr)
{
    UQUAD value = 0;

    if (!addr)
        return value;

    value |= ((UQUAD)AROS_LE2LONG(addr->addr_hi) << 32);
    value |= (UQUAD)AROS_LE2LONG(addr->addr_lo);

    return value;
}

void xhciDebugDumpDCBAAEntry(struct PCIController *hc, ULONG slotid)
{
    if (!hc || !hc->hc_DCBAAp)
        return;

    if (slotid > hc->hc_NumSlots)
        return;

    volatile struct xhci_address *dcbaa = (volatile struct xhci_address *)hc->hc_DCBAAp;
    UQUAD ptr = xhciDebugReadAddress(&dcbaa[slotid]);

    pciusbXHCIDebug("xHCI",
        DEBUGCOLOR_SET "DCBAA[%lu] -> 0x%08lx%08lx" DEBUGCOLOR_RESET" \n",
        slotid,
        (ULONG)(ptr >> 32),
        (ULONG)(ptr & 0xFFFFFFFF));
}

void xhciDebugDumpSlotContext(struct PCIController *hc, volatile struct xhci_slot *slot)
{
    if (!hc || !slot)
        return;

    ULONG ctx0 = AROS_LE2LONG(slot->ctx[0]);
    ULONG ctx1 = AROS_LE2LONG(slot->ctx[1]);
    ULONG ctx3 = AROS_LE2LONG(slot->ctx[3]);
    ULONG route = ctx0 & SLOT_CTX_ROUTE_MASK;
    ULONG speed = (ctx0 >> SLOTS_CTX_SPEED) & 0xF;
    ULONG port = (ctx1 >> 16) & 0xFF;
    ULONG target = (ctx3 >> 22) & 0x3FF;
    ULONG state = (ctx3 >> 27) & 0x1F;

    pciusbXHCIDebug("xHCI",
        DEBUGCOLOR_SET
        "Slot Ctx: route=0x%05lx port=%lu speed=%lu target=0x%03lx state=%lu"
        DEBUGCOLOR_RESET" \n",
        route,
        port,
        speed,
        target,
        state);
}

void xhciDebugDumpEndpointContext(struct PCIController *hc,
                                  volatile struct xhci_ep *ep,
                                  ULONG epid)
{
#if defined(PCIUSB_XHCI_DEBUG)
    if (!hc || !ep)
        return;

    ULONG ctx0   = AROS_LE2LONG(ep->ctx[0]);
    ULONG ctx1   = AROS_LE2LONG(ep->ctx[1]);
    ULONG length = AROS_LE2LONG(ep->length);
    UQUAD deq    = xhciDebugReadAddress(&ep->deq);

    ULONG ep_state = ctx0 & 0x7;
    ULONG mult     = (ctx0 >> EPS_CTX_MULT) & 0x3;       /* DW0[9:8] */
    ULONG interval = (ctx0 >> 16) & 0xFF;                /* DW0[23:16] */
    ULONG maxesit  = (ctx0 >> 24) & 0xFF;                /* DW0[31:24] */

    ULONG cerr     = (ctx1 >> EPS_CTX_CERR) & EP_CTX_CERR_MASK; /* DW1[1:0] */
    ULONG type     = (ctx1 >> EPS_CTX_TYPE) & 0x7;               /* DW1[4:2] */
    ULONG maxpkt   = (ctx1 >> EPS_CTX_PACKETMAX) & 0xFFFF;       /* DW1[31:16] */

    pciusbXHCIDebug("xHCI",
        DEBUGCOLOR_SET
        "EP Ctx %lu: state=%lu type=%lu mult=%lu interval=%lu maxESIT=%lu "
        "CErr=%lu maxpkt=%lu\n"
        "          deq=0x%08lx%08lx length=0x%08lx"
        DEBUGCOLOR_RESET" \n",
        epid,
        ep_state,
        type,
        mult,
        interval,
        maxesit,
        cerr,
        maxpkt,
        (ULONG)(deq >> 32),
        (ULONG)(deq & 0xFFFFFFFF),
        length);
#else
    (void)hc;
    (void)ep;
    (void)epid;
#endif
}
#endif /* PCIUSB_XHCI_DEBUG */
#endif /* PCIUSB_ENABLEXHCI */
