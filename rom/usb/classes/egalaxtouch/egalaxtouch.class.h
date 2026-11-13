#ifndef EGALAXTOUCH_CLASS_H
#define EGALAXTOUCH_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for egalaxtouch class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "egalaxtouch.h"

/* Protos */

struct NepClassHid * usbAttemptDeviceBinding(struct NepHidBase *nh, struct PsdDevice *pd);
struct NepClassHid * usbForceDeviceBinding(struct NepHidBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepHidBase *nh, struct NepClassHid *nch);

struct NepClassHid * nAllocHid(void);
void nFreeHid(struct NepClassHid *nch);

BOOL nLoadClassConfig(struct NepHidBase *nh);
BOOL nLoadBindingConfig(struct NepClassHid *nch);
LONG nOpenBindingCfgWindow(struct NepHidBase *nh, struct NepClassHid *nch);

void nGUITaskCleanup(struct NepClassHid *nch);

AROS_UFP0(void, nHidTask);
AROS_UFP0(void, nGUITask);

#endif /* EGALAXTOUCH_CLASS_H */
