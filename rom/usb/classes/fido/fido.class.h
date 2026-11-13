#ifndef FIDO_CLASS_H
#define FIDO_CLASS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Includes for fido.class
*/

#include "common.h"

#include <devices/newstyle.h>
#include <devices/usb_hid.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "fido.h"
#include "dev.h"

/* Protos */

struct NepClassFido * usbAttemptInterfaceBinding(struct NepFidoBase *nh, struct PsdInterface *pif);
struct NepClassFido * usbForceInterfaceBinding(struct NepFidoBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepFidoBase *nh, struct NepClassFido *ncp);

struct NepClassFido * nAllocFido(void);
void nFreeFido(struct NepClassFido *ncp);

AROS_UFP0(void, nFidoTask);

#endif /* FIDO_CLASS_H */
