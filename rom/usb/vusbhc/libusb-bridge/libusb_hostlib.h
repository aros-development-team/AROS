/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libusb-1.0/libusb.h>

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
};

extern struct libusb_func libusb_func;

#define LIBUSBCALL(func,...) (libusb_func.func(__VA_ARGS__))

BOOL LIBUSB_HostLib_Init();
VOID LIBUSB_HostLib_Cleanup();
