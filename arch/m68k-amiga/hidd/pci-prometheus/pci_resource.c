/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>

#include "pci_resource.h"

struct MinList *CreatePCIResourceList(IPTR start, IPTR size)
{
    struct MinList *ml;
    struct PCIResource *node;

    if ((node = AllocMem(sizeof(*node), MEMF_ANY))) {
        if ((ml = AllocMem(sizeof(*ml), MEMF_ANY))) {
            NEWLIST((struct List *)ml);

            node->pr_Start = start;
            node->pr_End = start + size - 1;
            AddTail((struct List *)ml, (struct Node *)node);
            return ml;
        }
        FreeMem(node, sizeof(*node));
    }

    return NULL;
}

/* Dispose of a resource list */
VOID DeletePCIResourceList(struct MinList *reslist)
{
    struct PCIResource *node;
    struct Node *tmp;

    ForeachNodeSafe((struct List *)reslist, node, tmp) {
        FreeMem(node, sizeof(*node));
    }

    FreeMem(reslist, sizeof(*reslist));
}

/* Allocates some size aligned space from the resource
 * list. Modifies the list (by removing the allocated chunk),
 * and returns the new start.
 *
 * 'size' must be a power of 2!
 *
 * Returns ~0 if no allocation was available.
 */
IPTR AllocPCIResource(struct MinList *reslist, IPTR size)
{
    struct PCIResource *node;
    IPTR alloc = ~0;
    IPTR start, end;

    ForeachNode((struct List *)reslist, node) {
        /* Aligned start to size */
        start = (node->pr_Start + size - 1) & ~(size - 1);
        end   = start + size - 1;
        if (end <= node->pr_End) {
            alloc = start;
            break;
        }
    }

    if (alloc == ~0)
        return alloc;

    if (node->pr_Start == start && end == node->pr_End) {
        /* Delete the node */
        Remove((struct Node *)node);
        FreeMem(node, sizeof(*node));
    } else if (node->pr_Start < start && end < node->pr_End) {
        struct PCIResource *tmp;

        /* Split the node? */
        tmp = AllocMem(sizeof(*tmp), MEMF_ANY);
        if (tmp == NULL)
            return ~0;

        tmp->pr_Start = end + 1;
        tmp->pr_End   = node->pr_End;

        node->pr_End = start - 1;
        Insert((struct List *)reslist, (struct Node *)tmp, (struct Node *)node);
    } else if (start == node->pr_Start) {
        /* Trim from front? */
        node->pr_Start = end + 1;
    } else if (end == node->pr_End) {
        /* Trim from end? */
        node->pr_End = start - 1;
    }

    return alloc;
}
