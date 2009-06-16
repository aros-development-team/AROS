#ifndef CDCACM_CLASS_H
#define CDCACM_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for cdcacm class
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

#include "cdcacm.h"
#include "dev.h"

/* Protos */

struct NepClassSerial * usbAttemptInterfaceBinding(struct NepSerialBase *nh, struct PsdInterface *pif);
struct NepClassSerial * usbForceInterfaceBinding(struct NepSerialBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepSerialBase *nh, struct NepClassSerial *ncp);

struct NepClassSerial * nAllocSerial(void);
void nFreeSerial(struct NepClassSerial *nch);

AROS_UFP0(void, nSerialTask);

#endif /* CDCACM_CLASS_H */
