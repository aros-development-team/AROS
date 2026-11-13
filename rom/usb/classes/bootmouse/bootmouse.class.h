#ifndef BOOTMOUSE_CLASS_H
#define BOOTMOUSE_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for Bootmouse class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/usb_hid.h>
#include <devices/rawkeycodes.h>

#include "bootmouse.h"

/* Protos */

struct NepClassHid * usbAttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
struct NepClassHid * usbForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch);

struct NepClassHid * nAllocHid(void);
void nFreeHid(struct NepClassHid *nch);

BOOL nLoadClassConfig(struct NepHidBase *nh);
BOOL nLoadBindingConfig(struct NepClassHid *nch);

LONG nOpenBindingCfgWindow(struct NepHidBase *nh, struct NepClassHid *nch);

void nGUITaskCleanup(struct NepClassHid *nch);

AROS_UFP0(void, nHidTask);
AROS_UFP0(void, nGUITask);

#endif /* BOOTMOUSE_CLASS_H */
