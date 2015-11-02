/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

BOOL libusb_bridge_init(struct VUSBHCIBase *VUSBHCIBase);
VOID libusb_bridge_cleanup();

int call_libusb_init(void);
void call_libusb_handler(void);

