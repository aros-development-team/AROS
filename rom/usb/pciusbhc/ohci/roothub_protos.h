/*
   Copyright (C) 2002-2009, Chris Hodges. All rights reserved.
   Copyright (C) 2009-2012, The AROS Development Team. All rights reserved.
*/

#ifndef ROOTHUB_H
#define ROOTHUB_H

#include "dev.h"

void CheckRootHubChanges(struct PCIUnit *unit);
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq,
    struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq,
    struct PCIUnit *unit, struct PCIDevice *base);

#endif /* ROOTHUB_H */
