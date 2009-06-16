#ifndef SERIALCP210X_CLASS_H
#define SERIALCP210X_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for serialcp210x class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/serial.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "serialcp210x.h"
#include "dev.h"

/* Protos */

struct NepClassSerial * usbAttemptDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd);
struct NepClassSerial * usbForceDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepSerialBase *nh, struct NepClassSerial *ncp);

struct NepClassSerial * nAllocSerial(void);
void nFreeSerial(struct NepClassSerial *nch);

AROS_UFP0(void, nSerialTask);

#endif /* SERIALCP210X_CLASS_H */
