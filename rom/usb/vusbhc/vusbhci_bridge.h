/*
    Copyright (C) 2015-2017, The AROS Development Team. All rights reserved.

    Desc: Virtual USB host controller
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
    "libusb_submit_transfer",
    "libusb_alloc_transfer",
    "libusb_free_transfer",
    "libusb_handle_events_completed",
    "libusb_control_transfer",
    "libusb_interrupt_transfer",
    "libusb_bulk_transfer",
    "libusb_set_auto_detach_kernel_driver",
    "libusb_get_device_speed",
    "libusb_claim_interface",
    "libusb_set_debug",
    "libusb_set_configuration",
    "libusb_kernel_driver_active",
    "libusb_detach_kernel_driver",
    "libusb_release_interface",
    "libusb_get_active_config_descriptor",
    "libusb_get_device",
    "libusb_get_configuration",
    "libusb_free_config_descriptor",
    "libusb_get_string_descriptor_ascii"
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
    struct libusb_transfer * (*libusb_alloc_transfer)(int iso_packets);
    void (*libusb_free_transfer)(struct libusb_transfer *transfer);
    int (*libusb_handle_events_completed)(libusb_context *ctx, int *completed);

    int (*libusb_control_transfer)(libusb_device_handle *dev_handle,
	    uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
	    unsigned char *data, uint16_t wLength, unsigned int timeout);

    int (*libusb_interrupt_transfer)(libusb_device_handle *dev_handle,
	    unsigned char endpoint, unsigned char *data, int length,
	    int *actual_length, unsigned int timeout);

    int (*libusb_bulk_transfer)(libusb_device_handle *dev_handle,
	    unsigned char endpoint, unsigned char *data, int length,
	    int *actual_length, unsigned int timeout);

    int (*libusb_set_auto_detach_kernel_driver)(libusb_device_handle *dev, int enable);

    int (*libusb_get_device_speed)(libusb_device *dev);
    int (*libusb_claim_interface)(libusb_device_handle *dev, int interface_number);
    void (*libusb_set_debug)(libusb_context *ctx, int level);
    int (*libusb_set_configuration)(libusb_device_handle *dev, int configuration);
    int (*libusb_kernel_driver_active)(libusb_device_handle *dev, int interface_number);
    int (*libusb_detach_kernel_driver)(libusb_device_handle *dev, int interface_number);
    int (*libusb_release_interface)(libusb_device_handle *dev, int interface_number);
    int (*libusb_get_active_config_descriptor)(libusb_device *dev, struct libusb_config_descriptor **config);
    libusb_device* (*libusb_get_device)(libusb_device_handle *dev);
    int (*libusb_get_configuration)(libusb_device_handle *dev, int *config);
    void (*libusb_free_config_descriptor)(struct libusb_config_descriptor *config);
    int (*libusb_get_string_descriptor_ascii)(libusb_device_handle *dev, uint8_t desc_index, unsigned char *data, int  	length); 	 	
};

//extern struct libusb_func libusb_func;

#define LIBUSBCALL(func,...) (libusb_func.func(__VA_ARGS__))

#endif /* VUSBHCI_BRIDGE_H */
