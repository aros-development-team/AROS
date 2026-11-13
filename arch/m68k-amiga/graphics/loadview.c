/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <graphics/view.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include "graphics_intern.h"
#include "graphics_driver.h"
#include "graphics_display.h"
#include "gfxfuncsupport.h"

/* See rom/graphics/loadview.c for the public API documentation. */
AROS_LH1(void, LoadView,
    AROS_LHA(struct View *, view, A1),
    struct GfxBase *, GfxBase, 37, Graphics)
{
    AROS_LIBFUNC_INIT

    volatile struct Custom *custom = (APTR)0xdff000;

    ObtainSemaphore(GfxBase->ActiViewCprSemaphore);
    if (GfxBase->ActiView != view || view != NULL)
    {
        GfxBase->ActiView = view;
        DoViewFunction(view, display_LoadViewPorts, GfxBase);
    }

    /* AmigaOS restores vertical blank even for repeated LoadView(NULL).
     * Hardware takeover programs can have disabled it since the last call;
     * leaving it off makes their following WaitTOF sleep forever. Preserve
     * their Copper list and DMA state when the view is already NULL.
     * Enable() restores the master bit only when Disable nesting allows it.
     */
    Disable();
    custom->intena = INTF_SETCLR | INTF_VERTB;
    Enable();
    ReleaseSemaphore(GfxBase->ActiViewCprSemaphore);

    AROS_LIBFUNC_EXIT
}
