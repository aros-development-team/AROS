#ifndef DFU_CLASS_H
#define DFU_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for DFU class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <devices/usb_dfu.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "dfu.h"

/* Protos */

struct NepClassDFU * usbAttemptInterfaceBinding(struct NepDFUBase *nh, struct PsdInterface *pif);
struct NepClassDFU * usbForceInterfaceBinding(struct NepDFUBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepDFUBase *nh, struct NepClassDFU *nch);

LONG nOpenBindingCfgWindow(struct NepDFUBase *nh, struct NepClassDFU *nch);

void nFWDownload(struct NepClassDFU *nch);
void nFWUpload(struct NepClassDFU *nch);
void nDetach(struct NepClassDFU *nch);

STRPTR nGetStatus(struct NepClassDFU *nch);

void nGUITaskCleanup(struct NepClassDFU *nch);

AROS_UFP0(void, nGUITask);

#endif /* DFU_CLASS_H */
