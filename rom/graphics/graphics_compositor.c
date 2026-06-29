/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/oop.h>

#include <graphics/driver.h>

#include "graphics_intern.h"
#include "graphics_compositor.h"
#include "dispinfo.h"

ULONG compositor_Install(OOP_Class *cl, struct GfxBase *GfxBase)
{
    struct monitor_displaydata *mdd;

    D(bug("[graphics.library/composit] Installing class 0x%p\n", cl));

    /*
     * Completely lazy initialization.
     * It even can't be done in other way because OOP_GetMethodID() on an unregistered
     * interface will fail (unlike OOP_ObtainAttrBase).
     */
    if (!HiddCompositorAttrBase)
    	HiddCompositorAttrBase = OOP_ObtainAttrBase(IID_Hidd_Compositor);

    if (!HiddCompositorAttrBase)
    	return DD_NO_MEM;

    if (!PrivGBase(GfxBase)->HiddCompositorMethodBase)
    {
    	PrivGBase(GfxBase)->HiddCompositorMethodBase = OOP_GetMethodID(IID_Hidd_Compositor, 0);

	if (!PrivGBase(GfxBase)->HiddCompositorMethodBase)
	{
    	    OOP_ReleaseAttrBase(IID_Hidd_Compositor);

    	    return DD_NO_MEM;
    	}
    }

    CDD(GfxBase)->compositorClass = cl;

    for (mdd = GFXPRIVATE_MONITORFIRST; mdd; mdd = (struct monitor_displaydata *)mdd->mdisplay.display_next)
    {
	/* If the display needs software composition, instantiate it. */
    	if (((mdd->mdisplay.display_flags & (DF_Enabled|DF_SoftComposit)) == (DF_Enabled|DF_SoftComposit)) &&
            (!mdd->mdisplay.display_compositor))
    	{
    	    compositor_Setup(mdd, GfxBase);
    	}
    }

    return DD_OK;
}

void compositor_Setup(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    mdd->mdisplay.display_compositor = OOP_NewObjectTags(CDD(GfxBase)->compositorClass, NULL,
				      aHidd_Compositor_DisplayHidd, mdd->mdisplay.display_obj,
				      aHidd_Compositor_DisplayID, mdd->mdisplay.display_idbase,
				      aHidd_Compositor_FrameBuffer, mdd->framebuffer, TAG_DONE);

    /* ... but print name of the original driver, to be informative */
    D(bug("[graphics.library/composit] compositor object @ 0x%p attached to display @ 0x%p (%s)\n",
          mdd->mdisplay.display_compositor, mdd->mdisplay.display_obj,
     	  OOP_OCLASS(mdd->mdisplay.display_obj)->ClassNode.ln_Name));
    D(bug("[graphics.library/composit] FrameBuffer @ 0x%p\n", mdd->framebuffer));
}

BOOL compositor_IsBMCompositable(struct BitMap *bitmap, DisplayInfoHandle handle, struct GfxBase *GfxBase)
{
    if (GFXPRIVATE_DISPLAYDATA(DIH(handle)->drv)->mdisplay.display_compositor)
    {
        struct pHidd_Compositor_BitMapValidate msg =
        {
            mID    : PrivGBase(GfxBase)->HiddCompositorMethodBase + moHidd_Compositor_BitMapValidate,
            bm     : bitmap,
        };

        return (BOOL)OOP_DoMethod(GFXPRIVATE_DISPLAYDATA(DIH(handle)->drv)->mdisplay.display_compositor, &msg.mID);
    }
    return FALSE;
}

BOOL compositor_SetBMCompositable(struct BitMap *bitmap, DisplayInfoHandle handle, struct GfxBase *GfxBase)
{
    if (GFXPRIVATE_DISPLAYDATA(DIH(handle)->drv)->mdisplay.display_compositor)
    {
        struct pHidd_Compositor_BitMapEnable msg =
        {
            mID    : PrivGBase(GfxBase)->HiddCompositorMethodBase + moHidd_Compositor_BitMapEnable,
            bm     : bitmap,
        };

        return (BOOL)OOP_DoMethod(GFXPRIVATE_DISPLAYDATA(DIH(handle)->drv)->mdisplay.display_compositor, &msg.mID);
    }
    return FALSE;
}
