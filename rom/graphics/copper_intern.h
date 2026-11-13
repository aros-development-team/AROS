#ifndef GRAPHICS_COPPER_INTERN_H
#define GRAPHICS_COPPER_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <graphics/copper.h>
#include <proto/graphics.h>

#define COPPER_INITIAL_COUNT 16

/* Classic callers may start with CWait/CMove on a zeroed UCopList.
 * Allocate its intermediate instructions before writing the first one. */
static inline struct CopIns *GetUserCopperInstruction(struct UCopList *ucl,
                                                     struct GfxBase *GfxBase)
{
    if (!ucl)
        return NULL;

    if (!ucl->FirstCopList && !UCopperListInit(ucl, COPPER_INITIAL_COUNT))
        return NULL;

    if (!ucl->CopList)
        return NULL;

    return ucl->CopList->CopPtr;
}

#endif
