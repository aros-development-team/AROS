#ifndef STIR4200_CLASS_H
#define STIR4200_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for STIr4200 class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/irda.h>
#include <irda/irlap.h>

#include <libraries/gadtools.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "stir4200.h"

#include "dev.h"

/* Protos */

struct NepClassSTIr4200 * usbAttemptDeviceBinding(struct NepSTIr4200Base *nh, struct PsdDevice *pd);
struct NepClassSTIr4200 * usbForceDeviceBinding(struct NepSTIr4200Base *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepSTIr4200Base *nh, struct NepClassSTIr4200 *ncp);

struct NepClassSTIr4200 * nAllocSTIr4200(void);
void nFreeSTIr4200(struct NepClassSTIr4200 *ncp);

BOOL nSetReg(struct NepClassSTIr4200 *ncp, ULONG reg, ULONG value);

BOOL nLoadClassConfig(struct NepSTIr4200Base *nh);
BOOL nLoadBindingConfig(struct NepClassSTIr4200 *ncp);
LONG nOpenBindingCfgWindow(struct NepSTIr4200Base *nh, struct NepClassSTIr4200 *ncp);

void nGUITaskCleanup(struct NepClassSTIr4200 *nh);

AROS_UFP0(void, nSTIr4200Task);
AROS_UFP0(void, nGUITask);

#endif /* STIR4200_CLASS_H */
