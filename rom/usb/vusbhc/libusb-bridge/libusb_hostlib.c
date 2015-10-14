/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/hostlib.h>
#include "libusb_hostlib.h"

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

static void *hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr) {
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

BOOL libusb_bridge_init() {

    HostLibBase = OpenResource("hostlib.resource");

    if (!HostLibBase)
        return FALSE;

    libusbhandle = hostlib_load_so("libusb.so", libusb_func_names, LIBUSB_NUM_FUNCS, (void **)&libusb_func);

    if (!libusbhandle)
        return FALSE;

    if(!LIBUSBCALL(libusb_init, NULL)) {
        bug("[LIBUSB] Checking hotplug support of libusb\n");
        if (LIBUSBCALL(libusb_has_capability, (LIBUSB_CAP_HAS_HOTPLUG))) {
            bug("[LIBUSB]  - Hotplug supported\n");
            return TRUE;
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
