#ifndef HIDD_COMPOSITOR_H
#define HIDD_COMPOSITOR_H
/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <oop/oop.h>
#include <hidd/graphics.h>

/*
 * Things described here are actually system-internal.
 * This class has no other use except inside graphics.library.
 * This include file is even not a part of AROS SDK.
 */

/*
 * Compositor interface.
 * Changing this breaks binary compatibility. graphics.library recognizes
 * this class by its name.
 */
#define CLID_Hidd_Compositor   "hidd.graphics.compositor"
#define IID_Hidd_Compositor    "hidd.graphics.compositor"

/* Compositor class methods */

enum
{
    moHidd_Compositor_BitMapStackChanged = 0,
    moHidd_Compositor_BitMapRectChanged,
    moHidd_Compositor_BitMapPositionChange,
    moHidd_Compositor_DisplayRectChanged,

    NUM_COMPOSITOR_METHODS
};

enum
{
    aoHidd_Compositor_GfxHidd = 0, 	/* [I..] Gfx driver object connected with this compositor object */
    aoHidd_Compositor_Capabilities,	/* [ISG] Composition capabilities of this implementation	  */
    aoHidd_Compositor_BackFillHook,	/* [ISG] Rendering hook for void space                            */
    aoHidd_Compositor_FrameBuffer,	/* [I..] Driver's framebuffer bitmap				  */

    num_Hidd_Compositor_Attrs
};

#define aHidd_Compositor_GfxHidd  	(HiddCompositorAttrBase + aoHidd_Compositor_GfxHidd)
#define aHidd_Compositor_Capabilities  (HiddCompositorAttrBase + aoHidd_Compositor_Capabilities)
#define aHidd_Compositor_BackFillHook  (HiddCompositorAttrBase + aoHidd_Compositor_BackFillHook)
#define aHidd_Compositor_FrameBuffer   (HiddCompositorAttrBase + aoHidd_Compositor_FrameBuffer)

#define IS_COMPOSITOR_ATTR(attr, idx) \
    (((idx) = (attr) - HiddCompositorAttrBase) < num_Hidd_Compositor_Attrs)

struct pHidd_Compositor_BitMapStackChanged
{
    OOP_MethodID                mID;
    struct HIDD_ViewPortData    *data;
    BOOL			*active;
};

struct pHidd_Compositor_BitMapRectChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    WORD            x;
    WORD            y;
    WORD            width;
    WORD            height;
};

struct pHidd_Compositor_BitMapPositionChange
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    SIPTR           *newxoffset;
    SIPTR           *newyoffset;
};

struct HIDD_BackFillHookMsg
{
    void                        *dummy;
    struct Rectangle            *bounds;
    LONG                        offsetx;
    LONG                        offsety;
};

#endif /* HIDD_COMPOSITOR_H */
