/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <graphics/gfxnodes.h>
#include <graphics/monitor.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include "graphics_intern.h"

/* Native palette updates may look up a raw ViewPort from a VBlank server.
 * They cannot wait for the graphics hash semaphore. On this single-CPU
 * target, disabling interrupts keeps the forward walk stable. GfxAssociate
 * initializes a new link before publishing it, and GfxFree unlinks a node
 * before freeing it, so an interrupted writer also leaves a valid walk. */
AROS_LH1(struct ExtendedNode *, GfxLookUp,
    AROS_LHA(void *, pointer, A0),
    struct GfxBase *, GfxBase, 117, Graphics)
{
    AROS_LIBFUNC_INIT

    IPTR *hash = GfxBase->hash_table;
    ULONG index = CalcHashIndex((IPTR)pointer, GFXASSOCIATE_HASHSIZE);
    struct ExtendedNode *node;

    Disable();
    node = (struct ExtendedNode *)hash[index];
    while (node && ((struct ViewExtra *)node)->View != pointer)
        node = (struct ExtendedNode *)node->xln_Succ;
    Enable();
    return node;

    AROS_LIBFUNC_EXIT
}
