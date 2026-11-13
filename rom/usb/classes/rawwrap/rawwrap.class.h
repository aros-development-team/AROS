#ifndef RAWWRAP_CLASS_H
#define RAWWRAP_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for RawWrap class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/serial.h>
#include <libraries/gadtools.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "rawwrap.h"
#include "dev.h"

/* Protos */

struct NepClassRawWrap * usbAttemptInterfaceBinding(struct NepRawWrapBase *nh, struct PsdInterface *pif);
struct NepClassRawWrap * usbForceInterfaceBinding(struct NepRawWrapBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepRawWrapBase *nh, struct NepClassRawWrap *ncp);

struct NepClassRawWrap * nAllocRawWrap(void);
void nFreeRawWrap(struct NepClassRawWrap *ncp);

BOOL nLoadClassConfig(struct NepRawWrapBase *nh);
BOOL nLoadBindingConfig(struct NepClassRawWrap *ncp);
LONG nOpenBindingCfgWindow(struct NepRawWrapBase *nh, struct NepClassRawWrap *ncp);

void nGUITaskCleanup(struct NepClassRawWrap *nh);

AROS_UFP0(void, nRawWrapTask);
AROS_UFP0(void, nGUITask);

#endif /* RAWWRAP_CLASS_H */
