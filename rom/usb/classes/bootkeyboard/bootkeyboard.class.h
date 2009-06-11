#ifndef BOOTKEYBOARD_CLASS_H
#define BOOTKEYBOARD_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                       Includes for Bootkeyboard class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/usb_hid.h>

#include "bootkeyboard.h"

/* Protos */

struct NepClassHid * usbAttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
struct NepClassHid * usbForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch);

void nParseKeys(struct NepClassHid *nch, UBYTE *buf);

struct NepClassHid * nAllocHid(void);
void nFreeHid(struct NepClassHid *nch);

BOOL nLoadClassConfig(struct NepHidBase *nh);
LONG nOpenCfgWindow(struct NepHidBase *nh);

void nGUITaskCleanup(struct NepHidBase *nh);

AROS_UFP0(void, nHidTask);
AROS_UFP0(void, nGUITask);

#endif /* BOOTKEYBOARD_CLASS_H */
