#ifndef PTP_CLASS_H
#define PTP_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for PTP class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <dos/dosextens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "ptp.h"

/* Protos */

struct NepClassPTP * usbAttemptInterfaceBinding(struct NepPTPBase *nh, struct PsdInterface *pif);
struct NepClassPTP * usbForceInterfaceBinding(struct NepPTPBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepPTPBase *nh, struct NepClassPTP *nch);

struct NepClassPTP * nAllocPTP(void);
void nFreePTP(struct NepClassPTP *nch);

BOOL nLoadClassConfig(struct NepPTPBase *nh);
BOOL nLoadBindingConfig(struct NepClassPTP *nch);

LONG nOpenBindingCfgWindow(struct NepPTPBase *nh, struct NepClassPTP *nch);

void nGUITaskCleanup(struct NepClassPTP *nch);

AROS_UFP0(void, nPTPTask);
AROS_UFP0(void, nGUITask);

#endif /* PTP_CLASS_H */
