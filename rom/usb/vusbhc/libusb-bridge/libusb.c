/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "libusb.h"
#include "libusb_hostlib.h"

int call_libusb_init() {
    return LIBUSBCALL(libusb_init, NULL);
}

