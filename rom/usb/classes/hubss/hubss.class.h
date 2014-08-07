#ifndef HUB_CLASS_H
#define HUB_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for hub class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/usb_hub.h>

#include "hubss.h"

/* Protos */

struct NepClassHub * GM_UNIQUENAME(usbAttemptDeviceBinding)(struct NepHubBase *nh, struct PsdDevice *pd);
struct NepClassHub * GM_UNIQUENAME(usbForceDeviceBinding)(struct NepHubBase * nh, struct PsdDevice *pd);
void GM_UNIQUENAME(usbReleaseDeviceBinding)(struct NepHubBase *nh, struct NepClassHub *nch);

struct NepClassHub * GM_UNIQUENAME(nAllocHub)(void);
void GM_UNIQUENAME(nFreeHub)(struct NepClassHub *nch);
struct PsdDevice * GM_UNIQUENAME(nConfigurePort)(struct NepClassHub *nch, UWORD port);
LONG GM_UNIQUENAME(nClearPortStatus)(struct NepClassHub *nch, UWORD port);
BOOL GM_UNIQUENAME(nHubSuspendDevice)(struct NepClassHub *nch, struct PsdDevice *pd);
BOOL GM_UNIQUENAME(nHubResumeDevice)(struct NepClassHub *nch, struct PsdDevice *pd);
void GM_UNIQUENAME(nHandleHubMethod)(struct NepClassHub *nch, struct NepHubMsg *nhm);

AROS_UFP0(void, GM_UNIQUENAME(nHubTask));

#endif /* HUB_CLASS_H */
