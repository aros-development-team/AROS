#ifndef PALMPDA_CLASS_H
#define PALMPDA_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for palmpda class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/serial.h>

#include <dos/dostags.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "palmpda.h"
#include "dev.h"

/* Protos */

struct NepClassSerial * usbAttemptDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd);
struct NepClassSerial * usbForceDeviceBinding(struct NepSerialBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepSerialBase *nh, struct NepClassSerial *ncp);

struct NepClassSerial * nAllocSerial(void);
void nFreeSerial(struct NepClassSerial *nch);

BOOL nLoadClassConfig(struct NepSerialBase *nh);
LONG nOpenCfgWindow(struct NepSerialBase *nh);

void nGUITaskCleanup(struct NepSerialBase *nh);

AROS_UFP0(void, nSerialTask);
AROS_UFP0(void, nGUITask);

#endif /* PALMPDA_CLASS_H */
