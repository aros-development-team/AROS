/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver debug functions
*/

#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

#undef base
#define base (hc->hc_Device)

/* Debug functions */
static UQUAD xhciDebugReadAddress(const volatile struct xhci_address *addr)
{
    UQUAD value = 0;

    if(!addr)
        return value;

    value |= ((UQUAD)AROS_LE2LONG(addr->addr_hi) << 32);
    value |= (UQUAD)AROS_LE2LONG(addr->addr_lo);

    return value;
}

#if defined(PCIUSB_XHCI_DEBUG)
void xhciDebugDumpDCBAAEntry(struct PCIController *hc, ULONG slotid)
{
    if(!hc)
        return;

    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);

    if(!xhcic || !xhcic->xhc_DCBAAp)
        return;

    if(slotid > xhcic->xhc_NumSlots)
        return;

    volatile struct xhci_address *dcbaa = (volatile struct xhci_address *)xhcic->xhc_DCBAAp;
    UQUAD ptr = xhciDebugReadAddress(&dcbaa[slotid]);

    pciusbDebug("xHCI",
                DEBUGCOLOR_SET "DCBAA[%lu] -> 0x%08lx%08lx" DEBUGCOLOR_RESET" \n",
                slotid,
                (ULONG)(ptr >> 32),
                (ULONG)(ptr & 0xFFFFFFFF));
}

void xhciDebugControlTransfer(struct IOUsbHWReq *ioreq)
{
    if(ioreq->iouh_Req.io_Command != UHCMD_CONTROLXFER)
        return;

    UWORD rt  = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    pciusbDebug("xHCI",
                DEBUGWARNCOLOR_SET
                "Device[%u]: Command %02lx %02lx %04lx %04lx %04lx!"
                DEBUGCOLOR_RESET "\n",
                ioreq->iouh_DevAddr,
                (ULONG)rt, (ULONG)req, (ULONG)idx, (ULONG)val, (ULONG)len);

    switch(rt) {
    /* =========================
     * STANDARD, DEVICE
     * ========================= */
    case(URTF_STANDARD | URTF_DEVICE):
        switch(req) {
        case USR_SET_ADDRESS:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: SetAddress = %ld"
                        DEBUGCOLOR_RESET "\n", (LONG)val);
            return;

        case USR_SET_CONFIGURATION:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: SetConfiguration = %ld"
                        DEBUGCOLOR_RESET "\n", (LONG)val);
            return;

        case USR_CLEAR_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: ClearFeature (feature=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)val);
            return;

        case USR_SET_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: SetFeature (feature=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)val);
            return;

        case USR_SET_DESCRIPTOR:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: SetDescriptor (type=%ld index=%ld len=%ld)"
                        DEBUGCOLOR_RESET "\n",
                        (LONG)(val >> 8), (LONG)(val & 0xff), (LONG)len);
            return;
        }
        break;

    case(URTF_IN | URTF_STANDARD | URTF_DEVICE):
        switch(req) {
        case USR_GET_STATUS:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: GetStatus (len=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)len);
            return;

        case USR_GET_DESCRIPTOR:
            switch(val >> 8) {
            case UDT_DEVICE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: GetDescriptor>Device (len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)len);
                return;

            case UDT_CONFIGURATION:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: GetDescriptor>Configuration (len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)len);
                return;

            case UDT_STRING:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: GetDescriptor>String (len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)len);
                return;

            default:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: GetDescriptor>Unsupported type=%02lx index=%02lx"
                            DEBUGCOLOR_RESET "\n",
                            (ULONG)(val >> 8), (ULONG)(val & 0xff));
                return;
            }
            break;

        case USR_GET_CONFIGURATION:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Dev: GetConfiguration()"
                        DEBUGCOLOR_RESET "\n");
            return;
        }
        break;

    /* =========================
     * STANDARD, INTERFACE
     * ========================= */
    case(URTF_STANDARD | URTF_INTERFACE):
        switch(req) {
        case USR_CLEAR_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std If: ClearFeature (feature=%ld, interface=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
            return;

        case USR_SET_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std If: SetFeature (feature=%ld, interface=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
            return;

        case USR_SET_INTERFACE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std If: SetInterface (interface=%ld, alt=%ld)"
                        DEBUGCOLOR_RESET "\n",
                        (LONG)idx, (LONG)val);
            return;
        }
        break;

    case(URTF_IN | URTF_STANDARD | URTF_INTERFACE):
        switch(req) {
        case USR_GET_STATUS:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std If: GetStatus (interface=%ld, len=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)idx, (LONG)len);
            return;

        case USR_GET_INTERFACE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std If: GetInterface (interface=%ld, len=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)idx, (LONG)len);
            return;
        }
        break;

    /* =========================
     * STANDARD, ENDPOINT
     * ========================= */
    case(URTF_STANDARD | URTF_ENDPOINT):
        switch(req) {
        case USR_CLEAR_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Ep: ClearFeature (feature=%ld, ep=0x%02lx)"
                        DEBUGCOLOR_RESET "\n", (LONG)val, (ULONG)idx);
            return;

        case USR_SET_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Ep: SetFeature (feature=%ld, ep=0x%02lx)"
                        DEBUGCOLOR_RESET "\n", (LONG)val, (ULONG)idx);
            return;
        }
        break;

    case(URTF_IN | URTF_STANDARD | URTF_ENDPOINT):
        switch(req) {
        case USR_GET_STATUS:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Ep: GetStatus (ep=0x%02lx, len=%ld)"
                        DEBUGCOLOR_RESET "\n", (ULONG)idx, (LONG)len);
            return;

        case USR_SYNCH_FRAME:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Std Ep: SynchFrame (ep=0x%02lx, len=%ld)"
                        DEBUGCOLOR_RESET "\n", (ULONG)idx, (LONG)len);
            return;
        }
        break;

    /* =========================
     * CLASS, OTHER / DEVICE / INTERFACE / ENDPOINT
     * ========================= */

    /* Class request to ?Other? recipient (as you already had) */
    case(URTF_CLASS | URTF_OTHER):
        switch(req) {
        case USR_SET_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Other: SetFeature (feature=%ld, index=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
            return;

        case USR_CLEAR_FEATURE:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Other: ClearFeature (feature=%ld, index=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
            return;
        }
        break;

    case(URTF_IN | URTF_CLASS | URTF_OTHER):
        switch(req) {
        case USR_GET_STATUS:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Other: GetStatus (index=%ld, len=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)idx, (LONG)len);
            return;

        case USR_GET_DESCRIPTOR:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Other: GetDescriptor (index=%ld, type=%ld, len=%ld)"
                        DEBUGCOLOR_RESET "\n",
                        (LONG)idx, (LONG)(val >> 8), (LONG)len);
            return;
        }
        break;

    /* Class, device */
    case(URTF_CLASS | URTF_DEVICE):
        pciusbDebug("xHCI", DEBUGCOLOR_SET
                    "Class Dev: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                    DEBUGCOLOR_RESET "\n",
                    (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
        return;

    case(URTF_IN | URTF_CLASS | URTF_DEVICE):
        switch(req) {
        case USR_GET_STATUS:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Dev: GetStatus (len=%ld)"
                        DEBUGCOLOR_RESET "\n", (LONG)len);
            return;

        case USR_GET_DESCRIPTOR:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Dev: GetDescriptor (type=%ld idx=%ld len=%ld)"
                        DEBUGCOLOR_RESET "\n",
                        (LONG)(val >> 8), (LONG)(val & 0xff), (LONG)len);
            return;

        default:
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Dev: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                        DEBUGCOLOR_RESET "\n",
                        (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
            return;
        }

    /* Class, interface */
    case(URTF_CLASS | URTF_INTERFACE):
    case(URTF_CLASS | URTF_ENDPOINT):
        pciusbDebug("xHCI", DEBUGCOLOR_SET
                    "Class %s: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                    DEBUGCOLOR_RESET "\n",
                    (rt & URTF_INTERFACE) ? "If" : "Ep",
                    (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
        return;

    case(URTF_IN | URTF_CLASS | URTF_INTERFACE):
    case(URTF_IN | URTF_CLASS | URTF_ENDPOINT):
        pciusbDebug("xHCI", DEBUGCOLOR_SET
                    "Class IN %s: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                    DEBUGCOLOR_RESET "\n",
                    (rt & URTF_INTERFACE) ? "If" : "Ep",
                    (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
        return;

    /* =========================
     * VENDOR requests (generic logging)
     * ========================= */
    case(URTF_VENDOR | URTF_DEVICE):
    case(URTF_VENDOR | URTF_INTERFACE):
    case(URTF_VENDOR | URTF_ENDPOINT):
    case(URTF_VENDOR | URTF_OTHER):
    case(URTF_IN | URTF_VENDOR | URTF_DEVICE):
    case(URTF_IN | URTF_VENDOR | URTF_INTERFACE):
    case(URTF_IN | URTF_VENDOR | URTF_ENDPOINT):
    case(URTF_IN | URTF_VENDOR | URTF_OTHER):
        pciusbDebug("xHCI", DEBUGCOLOR_SET
                    "Vendor req: rt=%02lx bReq=%02lx val=%04lx idx=%04lx len=%ld"
                    DEBUGCOLOR_RESET "\n",
                    (ULONG)rt, (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
        return;

    /* =========================
     * Fallback
     * ========================= */
    default:
        pciusbDebug("xHCI", DEBUGWARNCOLOR_SET
                    "Device: Unhandled setup: rt=%02lx req=%02lx idx=%04lx val=%04lx len=%04lx"
                    DEBUGCOLOR_RESET "\n",
                    (ULONG)rt, (ULONG)req,
                    (ULONG)idx, (ULONG)val, (ULONG)len);
        return;
    }

    /* If we somehow fall out of the switch on a known rt */
    pciusbDebug("xHCI", DEBUGWARNCOLOR_SET
                "Device: Unknown command!"
                DEBUGCOLOR_RESET "\n");
}
#endif

#if defined(XHCI_ENABLEINDEBUG)
void xhciDumpIN(volatile struct xhci_inctx *in)
{
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.DCF: %08x" DEBUGCOLOR_RESET" \n", in->dcf & ~0x3);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.ACF: %08x" DEBUGCOLOR_RESET" \n", in->acf);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.CFG = %02x" DEBUGCOLOR_RESET" \n", in->rsvd1[5] & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.INTERFACE = %02x" DEBUGCOLOR_RESET" \n", (in->rsvd1[5] >> 8) & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: IN.ALT = %02x" DEBUGCOLOR_RESET" \n", (in->rsvd1[5] >> 16) & 0xFF);
}
#endif

#if defined(XHCI_ENABLESLOTDEBUG)
void xhciDumpSlot(volatile struct xhci_slot *slot, int slotid)
{
    ULONG state = (slot->ctx[3] >> 27) & 0x1F;
    switch(state) {
    case 0:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].STATE = Disabled" DEBUGCOLOR_RESET" \n", slotid);
        break;
    case 1:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].STATE = Default" DEBUGCOLOR_RESET" \n", slotid);
        break;
    case 2:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].STATE = Addressed" DEBUGCOLOR_RESET" \n", slotid);
        break;
    case 3:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].STATE = Configured" DEBUGCOLOR_RESET" \n", slotid);
        break;
    default:
        KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].STATE = Unknown (%02x)" DEBUGCOLOR_RESET" \n", slotid, state);
        break;
    }
    KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].PORT         = %02x" DEBUGCOLOR_RESET" \n", slotid, (slot->ctx[1] >> 16) & 0xFF);
    KPrintF(DEBUGCOLOR_SET "xHCI: SLOT[%d].TARGET      = %03x" DEBUGCOLOR_RESET" \n", slotid, (slot->ctx[3] >> 22) & 0x3FF);
}

void xhciDebugDumpSlotContext(struct PCIController *hc, volatile struct xhci_slot *slot)
{
    if(!hc || !slot)
        return;

    ULONG ctx0 = AROS_LE2LONG(slot->ctx[0]);
    ULONG ctx1 = AROS_LE2LONG(slot->ctx[1]);
    ULONG ctx3 = AROS_LE2LONG(slot->ctx[3]);
    ULONG route = ctx0 & SLOT_CTX_ROUTE_MASK;
    ULONG speed = (ctx0 >> SLOTS_CTX_SPEED) & 0xF;
    ULONG port = (ctx1 >> 16) & 0xFF;
    ULONG target = (ctx3 >> 22) & 0x3FF;
    ULONG state = (ctx3 >> 27) & 0x1F;

    pciusbDebug("xHCI",
                DEBUGCOLOR_SET
                "Slot Ctx: route=0x%05lx port=%lu speed=%lu target=0x%03lx state=%lu"
                DEBUGCOLOR_RESET" \n",
                route,
                port,
                speed,
                target,
                state);
}
#endif

#if defined(XHCI_ENABLEEPDEBUG)
void xhciDumpEP(volatile struct xhci_ep *ep)
{
    ULONG ctx0 = ep->ctx[0];
    ULONG ctx1 = ep->ctx[1];

    /* Endpoint state (DW0 bits [2:0]) */
    UBYTE state = (ctx0 & 0x7);

    KPrintF(DEBUGCOLOR_SET "xHCI: EP.STATE = %x" DEBUGCOLOR_RESET" \n", state);
    switch(state) {
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
    switch(type) {
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
}

void xhciDumpEndpointCtx(struct PCIController *hc,
                         struct pciusbXHCIDevice *devCtx,
                         ULONG epid,
                         const char *reason)
{
    if(!hc || !devCtx || !devCtx->dc_SlotCtx.dmaa_Ptr)
        return;

    /* epid 0 is "no endpoint"; EP contexts start at EPID 1 */
    if(epid == 0 || epid > MAX_DEVENDPOINTS)
        return;

    /* Device (output) context:
     *   ctx[0] = Slot Context
     *   ctx[1] = Endpoint 1
     *   ctx[2] = Endpoint 2
     *   ...
     */
    UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;

    volatile UBYTE *sbase =
        (volatile UBYTE *)devCtx->dc_SlotCtx.dmaa_Ptr;

    volatile struct xhci_slot *slot =
        (volatile struct xhci_slot *)(sbase + 0 * ctxsize);
    volatile struct xhci_ep *ep =
        (volatile struct xhci_ep *)(sbase + epid * ctxsize);

    pciusbXHCIDebugEP("xHCI",
                      DEBUGCOLOR_SET "Dumping output ctx for EPID %lu (%s) slot @ 0x%p, ep @ 0x%p"
                      DEBUGCOLOR_RESET" \n",
                      epid,
                      reason ? reason : "current",
                      slot,
                      ep);

    xhciDumpSlot(slot, devCtx->dc_SlotID);
    xhciDumpEP(ep);
}

void xhciDebugDumpEndpointContext(struct PCIController *hc,
                                  volatile struct xhci_ep *ep,
                                  ULONG epid)
{
    if(!hc || !ep)
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

    pciusbDebug("xHCI",
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
}
#endif

#if defined(XHCI_ENABLESTATUSDEBUG)
void xhciDumpStatus(ULONG status)
{
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.STATUS: %08lx" DEBUGCOLOR_RESET" \n", status);

    if(status & XHCIF_USBSTS_HCH)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Host Controller Halted" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_HSE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Host System Error" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_HCE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Host Controller Error" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_EINT)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Event Interrupt Pending" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_PCD)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Port Change Detect" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_SSS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Save State Status" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_RSS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Restore State Status" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_SRE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Save/Restore Error" DEBUGCOLOR_RESET" \n");
    if(status & XHCIF_USBSTS_CNR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Controller Not Ready" DEBUGCOLOR_RESET" \n");
}
#endif

#if defined(XHCI_ENABLEOPRDEBUG)
void xhciDumpOpR(volatile struct xhci_hcopr *hcopr)
{
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.COMMAND: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd));
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.PAGESIZE: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->pagesize));
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.DNCTRL: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->dnctl));
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.CRCR: %08x%08x" DEBUGCOLOR_RESET" \n", hcopr->crcr.addr_hi, hcopr->crcr.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.DCBAAP: %08x%08x" DEBUGCOLOR_RESET" \n", hcopr->dcbaap.addr_hi,
            hcopr->dcbaap.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: OPR.CONFIG: %08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->config));
}
#endif

#if defined(XHCI_ENABLEIMANDEBUG)
void xhciDumpIMAN(ULONG iman)
{
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.IMAN: %08x" DEBUGCOLOR_RESET" \n", iman);
    if(iman & XHCIF_IR_IMAN_IE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Interrupts enabled" DEBUGCOLOR_RESET" \n");
    if(iman & XHCIF_IR_IMAN_IP)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Interrupts pending" DEBUGCOLOR_RESET" \n");
}
#endif

#if defined(XHCI_ENABLEIRDEBUG)
void xhciDumpIR(volatile struct xhci_ir *xhciir)
{
    xhciDumpIMAN(xhciir->iman);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.IMOD: %08x" DEBUGCOLOR_RESET" \n", xhciir->imod);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.ERSTSZ: %08x" DEBUGCOLOR_RESET" \n", xhciir->erstsz);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.ERDP: %08x%08x" DEBUGCOLOR_RESET" \n", xhciir->erdp.addr_hi, xhciir->erdp.addr_lo);
    KPrintF(DEBUGCOLOR_SET "xHCI: IR.ERSTBA: %08x%08x" DEBUGCOLOR_RESET" \n", xhciir->erstba.addr_hi,
            xhciir->erstba.addr_lo);
}
#endif

#if defined(XHCI_ENABLEPORTDEBUG)
void xhciDumpPort(volatile struct xhci_pr *xhcipr)
{
    ULONG portsc = AROS_LE2LONG(xhcipr->portsc);
    KPrintF(DEBUGCOLOR_SET "xHCI: PR.PORTSC = $%08x" DEBUGCOLOR_RESET" \n", portsc);
    if(portsc & XHCIF_PR_PORTSC_PP)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Powered" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_PED)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Enabled" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_CCS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Connected" DEBUGCOLOR_RESET" \n");

    if((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_FULLSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > Full Speed" DEBUGCOLOR_RESET" \n");
    } else if((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_LOWSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > Low Speed" DEBUGCOLOR_RESET" \n");
    } else if((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_HIGHSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > High Speed" DEBUGCOLOR_RESET" \n");
    } else if((portsc & (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) == XHCIF_PR_PORTSC_SUPERSPEED) {
        KPrintF(DEBUGCOLOR_SET "xHCI: > Super Speed" DEBUGCOLOR_RESET" \n");
    }

    if(portsc & XHCIF_PR_PORTSC_OCA)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Over Current Active" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_PR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Port Reset" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_LWS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Link State Write Strobe" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_CSC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Connect Status Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_PEC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Enable/Disable Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_WRC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Warm Reset Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_OCC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Over Current Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_PRC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Reset Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_PLC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Link State Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_CEC)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Config Error Changed" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_CAS)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Cold Attach Status" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_WCE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Wake on Connect" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_WDE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Wake on Disconnect" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_WOE)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Wake on Over Current" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_DR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Device Removable" DEBUGCOLOR_RESET" \n");

    if(portsc & XHCIF_PR_PORTSC_WPR)
        KPrintF(DEBUGCOLOR_SET "xHCI: > Warm Reset" DEBUGCOLOR_RESET" \n");
}
#endif

#if defined(XHCI_ENABLECCDEBUG)
void xhciDumpCC(UBYTE cc)
{
    KPrintF(DEBUGCOLOR_SET "xHCI: CC = $%02x" DEBUGCOLOR_RESET" \n", cc);
    switch(cc) {
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
}
#endif
