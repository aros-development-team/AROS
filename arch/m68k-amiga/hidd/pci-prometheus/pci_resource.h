/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef PCI_RESOURCE_H
#define PCI_RESOURCE_H

#include <exec/types.h>

struct PCIResource {
    struct MinNode pr_Node;
    IPTR pr_Start, pr_End;
};

struct MinList *CreatePCIResourceList(IPTR start, IPTR size);

/* Dispose of a resource list */
VOID DeletePCIResourceList(struct MinList *reslist);

/* Add resources to a list */
VOID AddPCIResource(struct MinList *reslist, IPTR start, IPTR size);

/* Allocates some size aligned space from the resource
 * list. Modifies the list (by removing the allocated chunk),
 * and returns the new start.
 *
 * 'size' must be a power of 2!
 *
 * Returns ~0 if no allocation was available.
 */
IPTR AllocPCIResource(struct MinList *reslist, IPTR size);

#endif /* PCI_RESOURCE_H */
