/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

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
    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    pciusbDebug("xHCI", DEBUGWARNCOLOR_SET "Device[%u]: Command %02lx %02lx %04lx %04lx %04lx!" DEBUGCOLOR_RESET "\n", ioreq->iouh_DevAddr, rt, req, idx, val, len);

    switch(rt)
    {
        case (URTF_STANDARD|URTF_DEVICE):
            switch(req)
            {
                case USR_SET_ADDRESS:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>SetAddress = %ld" DEBUGCOLOR_RESET "\n", val);
                    return(0);

                case USR_SET_CONFIGURATION:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: SetConfiguration=%ld" DEBUGCOLOR_RESET "\n", val);
                    return(0);
            }
            break;

        case (URTF_IN|URTF_STANDARD|URTF_DEVICE):
            switch(req)
            {
                case USR_GET_DESCRIPTOR:
                    switch(val>>8)
                    {
                        case UDT_DEVICE:
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>GetDescriptor>Device (%ld)" DEBUGCOLOR_RESET "\n", len);
                            return(0);

                        case UDT_CONFIGURATION:
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>GetDescriptor>Configuration (%ld)" DEBUGCOLOR_RESET "\n", len);
                            return(0);

                        case UDT_STRING:
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>GetDescriptor>String (%ld)" DEBUGCOLOR_RESET "\n", len);
                            return(0);

                        default:
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Unsupported Descriptor %04lx" DEBUGCOLOR_RESET "\n", idx);
                            return(0);
                    }
                    break;

                case USR_GET_CONFIGURATION:
                        pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>GetConfiguration ()" DEBUGCOLOR_RESET "\n");
                        return(0);
            }
            break;

        case (URTF_CLASS|URTF_OTHER):
            switch(req)
            {
                case USR_SET_FEATURE:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Other>SetFeature (%ld)" DEBUGCOLOR_RESET "\n", len);
                    return(0);

                case USR_CLEAR_FEATURE:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Other>ClearFeature (%ld)" DEBUGCOLOR_RESET "\n", len);
                    return(0);

            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_OTHER):
            switch(req)
            {
                case USR_GET_STATUS:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Other>GetStatus (%ld)" DEBUGCOLOR_RESET "\n", len);
                    return(0);


                case USR_GET_DESCRIPTOR:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Other>GetDescriptor %04lx" DEBUGCOLOR_RESET "\n", idx);
                    return(0);

            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_DEVICE):
            switch(req)
            {
                case USR_GET_STATUS:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>GetStatus (%ld)" DEBUGCOLOR_RESET "\n", len);
                    return(0);


                case USR_GET_DESCRIPTOR:
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Device: Device>GetDescriptor %04lx" DEBUGCOLOR_RESET "\n", idx);
                    return(0);
            }

    }
    pciusbDebug("xHCI", DEBUGWARNCOLOR_SET "Device: Unknown command!" DEBUGCOLOR_RESET "" DEBUGCOLOR_RESET "\n");

    return(0);
}
#endif
