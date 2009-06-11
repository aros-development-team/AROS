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

#include "hub.h"

/* Protos */

struct NepClassHub * usbAttemptDeviceBinding(struct NepHubBase *nh, struct PsdDevice *pd);
struct NepClassHub * usbForceDeviceBinding(struct NepHubBase * nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepHubBase *nh, struct NepClassHub *nch);

struct NepClassHub * nAllocHub(void);
void nFreeHub(struct NepClassHub *nch);
struct PsdDevice * nConfigurePort(struct NepClassHub *nch, UWORD port);
LONG nClearPortStatus(struct NepClassHub *nch, UWORD port);
BOOL nHubSuspendDevice(struct NepClassHub *nch, struct PsdDevice *pd);
BOOL nHubResumeDevice(struct NepClassHub *nch, struct PsdDevice *pd);
void nHandleHubMethod(struct NepClassHub *nch, struct NepHubMsg *nhm);

AROS_UFP0(void, nHubTask);

#endif /* HUB_CLASS_H */
