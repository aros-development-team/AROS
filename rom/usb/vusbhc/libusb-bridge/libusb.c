/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "libusb.h"
#include "libusb_hostlib.h"

int call_libusb_init() {
    return LIBUSBCALL(libusb_init, NULL);
}

void call_libusb_handler() {
    LIBUSBCALL(libusb_handle_events, NULL);
}
