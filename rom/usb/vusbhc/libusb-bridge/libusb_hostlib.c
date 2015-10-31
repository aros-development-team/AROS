/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/hostlib.h>
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
    "libusb_close"
};

#define LIBUSB_NUM_FUNCS (sizeof(libusb_func_names) / sizeof(libusb_func_names[0]))

APTR HostLibBase;
struct libusb_func libusb_func;
static void * libusbhandle;

static void *hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr);
BOOL libusb_bridge_init();
VOID libusb_bridge_cleanup();

static libusb_device_handle *handle = NULL;
int done = 0;

int hotplug_callback_event_handler(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
    bug("[LIBUSB] Hotplug callback event!\n");

    struct libusb_device_descriptor desc;
    int rc;

    switch(event) {

        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
            bug("[LIBUSB]  - Device attached\n");
            done++;

            rc = LIBUSBCALL(libusb_get_device_descriptor, dev, &desc);
            if (LIBUSB_SUCCESS != rc) {
                bug("[LIBUSB] Failed to read device descriptor\n");
                return 0;
            }

            bug("Device attach: %04x:%04x\n", desc.idVendor, desc.idProduct);

            LIBUSBCALL(libusb_open, dev, &handle);

        break;

        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
            bug("[LIBUSB]  - Device detached\n");
            done++;

            if(handle != NULL) {
                LIBUSBCALL(libusb_close, handle);
                handle = NULL;
            }

        break;

        default:
            bug("[LIBUSB]  - Unknown event arrived\n");
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

BOOL libusb_bridge_init(struct VUSBHCIUnit *unit) {

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
                            NULL,
                            NULL)
            );

            if(rc == LIBUSB_SUCCESS) {
                bug("[LIBUSB]  - Hotplug callback installed rc = %d\n", rc);

                //while (done < 2) {
                //    LIBUSBCALL(libusb_handle_events, NULL);
                //}

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
