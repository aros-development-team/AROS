#ifndef BLUETOOTH_CLASS_H
#define BLUETOOTH_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for BT class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/bluetoothhci.h>
#include <devices/newstyle.h>
#include <libraries/usbclass.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "bluetooth.h"
#include "dev.h"

/* Protos */

struct NepClassBT * usbAttemptInterfaceBinding(struct NepBTBase *nh, struct PsdInterface *pif);
struct NepClassBT * usbForceInterfaceBinding(struct NepBTBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepBTBase *nh, struct NepClassBT *ncp);

struct NepClassBT * nAllocBT(void);
void nFreeBT(struct NepClassBT *ncp);

LONG nOpenBindingCfgWindow(struct NepBTBase *nh, struct NepClassBT *ncp);

void nGUITaskCleanup(struct NepClassBT *nh);

BOOL nLoadClassConfig(struct NepBTBase *nh);
BOOL nLoadBindingConfig(struct NepClassBT *ncp);

AROS_UFP0(void, nBTTask);
AROS_UFP0(void, nGUITask);

#endif /* BLUETOOTH_CLASS_H */
