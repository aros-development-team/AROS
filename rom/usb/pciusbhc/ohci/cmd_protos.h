/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
*/

#ifndef CMD_PROTOS_H
#define CMD_PROTOS_H

#include "dev.h"

struct Unit *Open_Unit(struct IOUsbHWReq *ioreq, LONG unitnr,
    struct PCIDevice *base);
void Close_Unit(struct PCIDevice *base, struct PCIUnit *unit,
    struct IOUsbHWReq *ioreq);

void DelayMS(ULONG milli, struct PCIUnit *unit);
void CheckSpecialCtrlTransfers(struct PCIController *hc,
    struct IOUsbHWReq *ioreq);

WORD cmdReset(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);
WORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);
WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);
WORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);

WORD cmdXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);

WORD cmdFlush(struct IOUsbHWReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq, struct PCIUnit *unit,
    struct PCIDevice *base);

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

void TermIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

AROS_INTP(NakTimeoutInt);

#endif /* CMD_PROTOS_H */
