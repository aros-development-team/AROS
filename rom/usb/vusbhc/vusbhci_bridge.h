/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Virtual USB host controller
    Lang: English
*/

#ifndef VUSBHCI_BRIDGE_H
#define VUSBHCI_BRIDGE_H

#include <libusb-1.0/libusb.h>

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

struct libusb_func {
    int (*libusb_init)(libusb_context **ctx);
    void (*libusb_exit)(libusb_context *ctx);
    int (*libusb_has_capability)(uint32_t capability);
    int (*libusb_hotplug_register_callback)(libusb_context *ctx,
        libusb_hotplug_event events,
        libusb_hotplug_flag flags,
        int vendor_id,
        int product_id,
        int dev_class,
        libusb_hotplug_callback_fn cb_fn,
        void *user_data,
        libusb_hotplug_callback_handle *handle);
    int (*libusb_handle_events)(libusb_context *ctx);
    int (*libusb_get_device_descriptor)(libusb_device *dev,
        struct libusb_device_descriptor *desc);
    int (*libusb_open)(libusb_device *dev, libusb_device_handle **handle);
    void (*libusb_close)(libusb_device_handle *dev_handle);
    int (*libusb_submit_transfer)(struct libusb_transfer *transfer);
};

//extern struct libusb_func libusb_func;

#define LIBUSBCALL(func,...) (libusb_func.func(__VA_ARGS__))

#endif /* VUSBHCI_BRIDGE_H */
