/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver main pciusb interface
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (base->hd_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

WORD xhciPrepareTransfer(struct IOUsbHWReq *ioreq,
                         struct PCIUnit *unit,
                         struct PCIDevice *base)
{
    /* Only decode control transfers for now */
    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER) {
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

        switch (rt) {
        /* =========================
         * STANDARD, DEVICE
         * ========================= */
        case (URTF_STANDARD | URTF_DEVICE):
            switch (req) {
            case USR_SET_ADDRESS:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: SetAddress = %ld"
                            DEBUGCOLOR_RESET "\n", (LONG)val);
                return 0;

            case USR_SET_CONFIGURATION:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: SetConfiguration = %ld"
                            DEBUGCOLOR_RESET "\n", (LONG)val);
                return 0;

            case USR_CLEAR_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: ClearFeature (feature=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)val);
                return 0;

            case USR_SET_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: SetFeature (feature=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)val);
                return 0;

            case USR_SET_DESCRIPTOR:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: SetDescriptor (type=%ld index=%ld len=%ld)"
                            DEBUGCOLOR_RESET "\n",
                            (LONG)(val >> 8), (LONG)(val & 0xff), (LONG)len);
                return 0;
            }
            break;

        case (URTF_IN | URTF_STANDARD | URTF_DEVICE):
            switch (req) {
            case USR_GET_STATUS:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: GetStatus (len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)len);
                return 0;

            case USR_GET_DESCRIPTOR:
                switch (val >> 8) {
                case UDT_DEVICE:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET
                                "Std Dev: GetDescriptor>Device (len=%ld)"
                                DEBUGCOLOR_RESET "\n", (LONG)len);
                    return 0;

                case UDT_CONFIGURATION:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET
                                "Std Dev: GetDescriptor>Configuration (len=%ld)"
                                DEBUGCOLOR_RESET "\n", (LONG)len);
                    return 0;

                case UDT_STRING:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET
                                "Std Dev: GetDescriptor>String (len=%ld)"
                                DEBUGCOLOR_RESET "\n", (LONG)len);
                    return 0;

                default:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET
                                "Std Dev: GetDescriptor>Unsupported type=%02lx index=%02lx"
                                DEBUGCOLOR_RESET "\n",
                                (ULONG)(val >> 8), (ULONG)(val & 0xff));
                    return 0;
                }
                break;

            case USR_GET_CONFIGURATION:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Dev: GetConfiguration()"
                            DEBUGCOLOR_RESET "\n");
                return 0;
            }
            break;

        /* =========================
         * STANDARD, INTERFACE
         * ========================= */
        case (URTF_STANDARD | URTF_INTERFACE):
            switch (req) {
            case USR_CLEAR_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std If: ClearFeature (feature=%ld, interface=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
                return 0;

            case USR_SET_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std If: SetFeature (feature=%ld, interface=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
                return 0;

            case USR_SET_INTERFACE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std If: SetInterface (interface=%ld, alt=%ld)"
                            DEBUGCOLOR_RESET "\n",
                            (LONG)idx, (LONG)val);
                return 0;
            }
            break;

        case (URTF_IN | URTF_STANDARD | URTF_INTERFACE):
            switch (req) {
            case USR_GET_STATUS:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std If: GetStatus (interface=%ld, len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)idx, (LONG)len);
                return 0;

            case USR_GET_INTERFACE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std If: GetInterface (interface=%ld, len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)idx, (LONG)len);
                return 0;
            }
            break;

        /* =========================
         * STANDARD, ENDPOINT
         * ========================= */
        case (URTF_STANDARD | URTF_ENDPOINT):
            switch (req) {
            case USR_CLEAR_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Ep: ClearFeature (feature=%ld, ep=0x%02lx)"
                            DEBUGCOLOR_RESET "\n", (LONG)val, (ULONG)idx);
                return 0;

            case USR_SET_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Ep: SetFeature (feature=%ld, ep=0x%02lx)"
                            DEBUGCOLOR_RESET "\n", (LONG)val, (ULONG)idx);
                return 0;
            }
            break;

        case (URTF_IN | URTF_STANDARD | URTF_ENDPOINT):
            switch (req) {
            case USR_GET_STATUS:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Ep: GetStatus (ep=0x%02lx, len=%ld)"
                            DEBUGCOLOR_RESET "\n", (ULONG)idx, (LONG)len);
                return 0;

            case USR_SYNCH_FRAME:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Std Ep: SynchFrame (ep=0x%02lx, len=%ld)"
                            DEBUGCOLOR_RESET "\n", (ULONG)idx, (LONG)len);
                return 0;
            }
            break;

        /* =========================
         * CLASS, OTHER / DEVICE / INTERFACE / ENDPOINT
         * ========================= */

        /* Class request to “Other” recipient (as you already had) */
        case (URTF_CLASS | URTF_OTHER):
            switch (req) {
            case USR_SET_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Other: SetFeature (feature=%ld, index=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
                return 0;

            case USR_CLEAR_FEATURE:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Other: ClearFeature (feature=%ld, index=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)val, (LONG)idx);
                return 0;
            }
            break;

        case (URTF_IN | URTF_CLASS | URTF_OTHER):
            switch (req) {
            case USR_GET_STATUS:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Other: GetStatus (index=%ld, len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)idx, (LONG)len);
                return 0;

            case USR_GET_DESCRIPTOR:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Other: GetDescriptor (index=%ld, type=%ld, len=%ld)"
                            DEBUGCOLOR_RESET "\n",
                            (LONG)idx, (LONG)(val >> 8), (LONG)len);
                return 0;
            }
            break;

        /* Class, device */
        case (URTF_CLASS | URTF_DEVICE):
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class Dev: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                        DEBUGCOLOR_RESET "\n",
                        (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
            return 0;

        case (URTF_IN | URTF_CLASS | URTF_DEVICE):
            switch (req) {
            case USR_GET_STATUS:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Dev: GetStatus (len=%ld)"
                            DEBUGCOLOR_RESET "\n", (LONG)len);
                return 0;

            case USR_GET_DESCRIPTOR:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Dev: GetDescriptor (type=%ld idx=%ld len=%ld)"
                            DEBUGCOLOR_RESET "\n",
                            (LONG)(val >> 8), (LONG)(val & 0xff), (LONG)len);
                return 0;

            default:
                pciusbDebug("xHCI", DEBUGCOLOR_SET
                            "Class Dev: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                            DEBUGCOLOR_RESET "\n",
                            (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
                return 0;
            }

        /* Class, interface */
        case (URTF_CLASS | URTF_INTERFACE):
        case (URTF_CLASS | URTF_ENDPOINT):
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class %s: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                        DEBUGCOLOR_RESET "\n",
                        (rt & URTF_INTERFACE) ? "If" : "Ep",
                        (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
            return 0;

        case (URTF_IN | URTF_CLASS | URTF_INTERFACE):
        case (URTF_IN | URTF_CLASS | URTF_ENDPOINT):
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Class IN %s: bReq=%02lx val=%04lx idx=%04lx len=%ld"
                        DEBUGCOLOR_RESET "\n",
                        (rt & URTF_INTERFACE) ? "If" : "Ep",
                        (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
            return 0;

        /* =========================
         * VENDOR requests (generic logging)
         * ========================= */
        case (URTF_VENDOR | URTF_DEVICE):
        case (URTF_VENDOR | URTF_INTERFACE):
        case (URTF_VENDOR | URTF_ENDPOINT):
        case (URTF_VENDOR | URTF_OTHER):
        case (URTF_IN | URTF_VENDOR | URTF_DEVICE):
        case (URTF_IN | URTF_VENDOR | URTF_INTERFACE):
        case (URTF_IN | URTF_VENDOR | URTF_ENDPOINT):
        case (URTF_IN | URTF_VENDOR | URTF_OTHER):
            pciusbDebug("xHCI", DEBUGCOLOR_SET
                        "Vendor req: rt=%02lx bReq=%02lx val=%04lx idx=%04lx len=%ld"
                        DEBUGCOLOR_RESET "\n",
                        (ULONG)rt, (ULONG)req, (ULONG)val, (ULONG)idx, (LONG)len);
            return 0;

        /* =========================
         * Fallback
         * ========================= */
        default:
            pciusbDebug("xHCI", DEBUGWARNCOLOR_SET
                        "Device: Unhandled setup: rt=%02lx req=%02lx idx=%04lx val=%04lx len=%04lx"
                        DEBUGCOLOR_RESET "\n",
                        (ULONG)rt, (ULONG)req,
                        (ULONG)idx, (ULONG)val, (ULONG)len);
            return 0;
        }

        /* If we somehow fall out of the switch on a known rt */
        pciusbDebug("xHCI", DEBUGWARNCOLOR_SET
                    "Device: Unknown command!"
                    DEBUGCOLOR_RESET "\n");
    }

    return 0;
}

#endif
