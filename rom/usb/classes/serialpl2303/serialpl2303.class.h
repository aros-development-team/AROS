#ifndef SERIALPL2303_CLASS_H
#define SERIALPL2303_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for serialpl2303 class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/serial.h>

#include <devices/usb_cdc.h>
#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "serialpl2303.h"
#include "dev.h"

/* Protos */

struct NepClassSerial * usbAttemptDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd);
struct NepClassSerial * usbForceDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepSerialBase *nh, struct NepClassSerial *ncp);

struct NepClassSerial * nAllocSerial(void);
void nFreeSerial(struct NepClassSerial *nch);

AROS_UFP0(void, nSerialTask);

#endif /* SERIALPL2303_CLASS_H */
