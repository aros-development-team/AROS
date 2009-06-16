#ifndef PRINTER_CLASS_H
#define PRINTER_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for Printer class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/parallel.h>
#include <libraries/gadtools.h>

#include <devices/usb_printer.h>
#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "printer.h"
#include "dev.h"

/* Protos */

struct NepClassPrinter * usbAttemptInterfaceBinding(struct NepPrinterBase *nh, struct PsdInterface *pif);
struct NepClassPrinter * usbForceInterfaceBinding(struct NepPrinterBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepPrinterBase *nh, struct NepClassPrinter *ncp);

struct NepClassPrinter * nAllocPrinter(void);
void nFreePrinter(struct NepClassPrinter *nch);

BOOL nLoadClassConfig(struct NepPrinterBase *nh);
LONG nOpenCfgWindow(struct NepPrinterBase *nh);

void nGUITaskCleanup(struct NepPrinterBase *nh);

AROS_UFP0(void, nPrinterTask);
AROS_UFP0(void, nGUITask);

#endif /* PRINTER_CLASS_H */
