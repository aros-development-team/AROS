/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
*/

#ifndef CHIP_PROTOS_H
#define CHIP_PROTOS_H

#include "dev.h"

void UpdateFrameCounter(struct PCIController *hc);
void AbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq);
BOOL InitController(struct PCIController *hc, struct PCIUnit *hu);
void FreeController(struct PCIController *hc, struct PCIUnit *hu);
UWORD TranslatePortFlags(ULONG flags, ULONG mask);

#endif /* CHIP_PROTOS_H */
