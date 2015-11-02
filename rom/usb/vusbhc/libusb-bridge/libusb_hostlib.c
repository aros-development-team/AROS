/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/hostlib.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>

#include "libusb_hostlib.h"
#include "../vusbhci_device.h"

#include <aros/debug.h>

static const char *libusb_func_names[] = {
    "libusb_init",
    "libusb_exit",
    "libusb_has_capability",
    "libusb_hotplug_register_callback",
    "libusb_handle_events",
    "libusb_get_device_descriptor",
    "libusb_open",
    "libusb_close",
    "libusb_submit_transfer"
};

#define LIBUSB_NUM_FUNCS (sizeof(libusb_func_names) / sizeof(libusb_func_names[0]))

APTR HostLibBase;
struct libusb_func libusb_func;
static void * libusbhandle;

static void *hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr);
BOOL libusb_bridge_init();
VOID libusb_bridge_cleanup();

extern void uhwCheckRootHubChanges(struct VUSBHCIUnit *unit);

static libusb_device_handle *handle = NULL;

int hotplug_callback_event_handler(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {

    struct VUSBHCIBase *VUSBHCIBase = (struct VUSBHCIBase *)user_data;
    struct VUSBHCIUnit *unit = VUSBHCIBase->usbunit200;

    struct libusb_device_descriptor desc;
    int rc;

    mybug_unit(-1, ("Hotplug callback event!\n"));

    switch(event) {

        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
            mybug_unit(-1, ("- Device attached\n"));

            if(unit->allocated) {

                /* Tell Poseidon it's highspeed, just testing */
                unit->roothub.portstatus.wPortStatus |= AROS_WORD2LE(UPSF_PORT_CONNECTION|UPSF_PORT_HIGH_SPEED);
                unit->roothub.portstatus.wPortChange |= AROS_WORD2LE(UPSF_PORT_CONNECTION);

                uhwCheckRootHubChanges(unit);

                rc = LIBUSBCALL(libusb_get_device_descriptor, dev, &desc);
                if (LIBUSB_SUCCESS != rc) {
                    mybug_unit(-1, ("Failed to read device descriptor\n"));
                    return 0;
                }

                mybug_unit(-1, ("Device attach: %04x:%04x\n", desc.idVendor, desc.idProduct));

                LIBUSBCALL(libusb_open, dev, &handle);

            }
        break;

        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
            mybug_unit(-1, (" - Device detached\n"));

            if(unit->allocated) {

                unit->roothub.portstatus.wPortStatus &= ~UPSF_PORT_CONNECTION;
                unit->roothub.portstatus.wPortChange |= UPSF_PORT_CONNECTION;

                uhwCheckRootHubChanges(unit);
            }

            if(handle != NULL) {
                LIBUSBCALL(libusb_close, handle);
                handle = NULL;
            }

        break;

        default:
            mybug_unit(-1, (" - Unknown event arrived\n"));
        break;

    }

  return 0;
}

void *hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr) {
    void *handle;
    char *err;
    int i;

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        (bug("[LIBUSB] failed to open '%s': %s\n", sofile, err));
        return NULL;
    }else{
        bug("[LIBUSB] opened '%s'\n", sofile);
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if (err != NULL) {
            bug("[LIBUSB] failed to get symbol '%s' (%s)\n", names[i], err);
            HostLib_Close(handle, NULL);
            return NULL;
        }else{
            bug("[LIBUSB] managed to get symbol '%s'\n", names[i]);
        }
    }

    return handle;
}

BOOL libusb_bridge_init(struct VUSBHCIBase *VUSBHCIBase) {

    int rc;

    HostLibBase = OpenResource("hostlib.resource");

    if (!HostLibBase)
        return FALSE;

    libusbhandle = hostlib_load_so("libusb.so", libusb_func_names, LIBUSB_NUM_FUNCS, (void **)&libusb_func);

    if (!libusbhandle)
        return FALSE;

    if(!LIBUSBCALL(libusb_init, NULL)) {
        bug("[LIBUSB] Checking hotplug support of libusb\n");
        if (LIBUSBCALL(libusb_has_capability, LIBUSB_CAP_HAS_HOTPLUG)) {
            bug("[LIBUSB]  - Hotplug supported\n");

            rc = (LIBUSBCALL(libusb_hotplug_register_callback,
                            NULL,
                            (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                            0,
                            LIBUSB_HOTPLUG_MATCH_ANY,
                            LIBUSB_HOTPLUG_MATCH_ANY,
                            LIBUSB_HOTPLUG_MATCH_ANY,
                            hotplug_callback_event_handler,
                            (void *)VUSBHCIBase,
                            NULL)
            );

            if(rc == LIBUSB_SUCCESS) {
                bug("[LIBUSB]  - Hotplug callback installed rc = %d\n", rc);
                return TRUE;
            }

            bug("[LIBUSB]  - Hotplug callback installation failure! rc = %d\n", rc);

        } else {
            bug("[LIBUSB]  - Hotplug not supported, failing...\n");
            LIBUSBCALL(libusb_exit, NULL);
        }
        libusb_bridge_cleanup();
    }

    return FALSE;
}

VOID libusb_bridge_cleanup() {

    HostLib_Close(libusbhandle, NULL);
}

int do_libusb_transfer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    UWORD bmRequestType      = (ioreq->iouh_SetupData.bmRequestType) & (URTF_STANDARD | URTF_CLASS | URTF_VENDOR);
    UWORD bmRequestDirection = (ioreq->iouh_SetupData.bmRequestType) & (URTF_IN | URTF_OUT);
    UWORD bmRequestRecipient = (ioreq->iouh_SetupData.bmRequestType) & (URTF_DEVICE | URTF_INTERFACE | URTF_ENDPOINT | URTF_OTHER);

    UWORD bRequest           = (ioreq->iouh_SetupData.bRequest);
    UWORD wValue             = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD wIndex             = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD wLength            = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);

    switch (ioreq->iouh_Req.io_Command) {
        case UHCMD_CONTROLXFER:
            mybug_unit(-1, ("cmdControlXFer\n"));
        break;
        case UHCMD_INTXFER:
            mybug_unit(-1, ("cmdIntXFer\n"));
        break;
        case UHCMD_BULKXFER:
            mybug_unit(-1, ("cmdBulkXFer\n"));
        break;
        case UHCMD_ISOXFER:
            mybug_unit(-1, ("cmdISOXFer\n"));
        break;
    }

    mybug_unit(-1, ("bmRequestDirection "));
    switch (bmRequestDirection) {
        case URTF_IN:
            mybug(-1, ("URTF_IN\n"));
            break;
        case URTF_OUT:
            mybug(-1, ("URTF_OUT\n"));
            break;
    }

    mybug_unit(-1, ("bmRequestType "));
    switch(bmRequestType) {
        case URTF_STANDARD:
            mybug(-1, ("URTF_STANDARD\n"));
            break;
        case URTF_CLASS:
            mybug(-1, ("URTF_CLASS\n"));
            break;
        case URTF_VENDOR:
            mybug(-1, ("URTF_VENDOR\n"));
            break;
    }

    mybug_unit(-1, ("bmRequestRecipient "));
    switch (bmRequestRecipient) {
        case URTF_DEVICE:
            mybug(-1, ("URTF_DEVICE\n"));
            break;
        case URTF_INTERFACE:
            mybug(-1, ("URTF_INTERFACE\n"));
            break;
        case URTF_ENDPOINT:
            mybug(-1, ("URTF_ENDPOINT\n"));
            break;
        case URTF_OTHER:
            mybug(-1, ("URTF_OTHER\n"));
            break;
    }

    mybug_unit(-1, ("bRequest "));
    switch(bRequest) {
        case USR_GET_STATUS:
            bug("USR_GET_STATUS\n");
            break;
        case USR_CLEAR_FEATURE:
            mybug(-1, ("USR_CLEAR_FEATURE\n"));
            break;
        case USR_SET_FEATURE:
            mybug(-1, ("USR_SET_FEATURE\n"));
            break;
        case USR_SET_ADDRESS:
            mybug(-1, ("USR_SET_ADDRESS\n"));
            break;
        case USR_GET_DESCRIPTOR:
            mybug(-1, ("USR_GET_DESCRIPTOR\n"));
            break;
        case USR_SET_DESCRIPTOR:
            mybug(-1, ("USR_SET_DESCRIPTOR\n"));
            break;
        case USR_GET_CONFIGURATION:
            mybug(-1, ("USR_GET_CONFIGURATION\n"););
            break;
        case USR_SET_CONFIGURATION:
            mybug(-1, ("USR_SET_CONFIGURATION\n"));
            break;
        case USR_GET_INTERFACE:
            mybug(-1, ("USR_GET_INTERFACE\n"));
            break;
        case USR_SET_INTERFACE:
            mybug(-1, ("USR_SET_INTERFACE\n"));
            break;
        case USR_SYNCH_FRAME:
            mybug(-1, ("USR_SYNCH_FRAME\n"));
            break;
    }

    mybug_unit(-1, ("wIndex %x\n", wIndex));
    mybug_unit(-1, ("wValue %x\n", wValue));
    mybug_unit(-1, ("wLength %d\n", wLength));

    return 0;    
}

