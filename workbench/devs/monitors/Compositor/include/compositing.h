#ifndef HIDD_COMPOSITING_H
#define HIDD_COMPOSITING_H
/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: compositing.h 35441 2010-11-13 22:17:39Z deadwood $
*/

#include <oop/oop.h>
#include <hidd/graphics.h>

/*
 * Things described here are actually system-internal.
 * This class has no other use except inside graphics.library.
 * This include file is even not a part of AROS SDK.
 */

/*
 * Compositing interface.
 * Changing this breaks binary compatibility. graphics.library recognizes
 * this class by its name.
 */
#define CLID_Hidd_Compositing   "hidd.graphics.compositing"
#define IID_Hidd_Compositing    "hidd.graphics.compositing"

/* Compositing class methods */

enum
{
    moHidd_Compositing_BitMapStackChanged = 0,
    moHidd_Compositing_BitMapRectChanged,
    moHidd_Compositing_BitMapPositionChange,
    moHidd_Compositing_DisplayRectChanged,

    NUM_COMPOSITING_METHODS
};

enum
{
    aoHidd_Compositing_GfxHidd = 0, 	/* [I..] Gfx driver object connected with this compositing object */
    aoHidd_Compositing_Capabilities,	/* [ISG] Composition capabilities of this implementation	  */
    aoHidd_Compositing_BackFillHook,	/* [ISG] Rendering hook for void space                            */
    aoHidd_Compositing_FrameBuffer,	/* [I..] Driver's framebuffer bitmap				  */

    num_Hidd_Compositing_Attrs
};

#define aHidd_Compositing_GfxHidd  	(HiddCompositingAttrBase + aoHidd_Compositing_GfxHidd)
#define aHidd_Compositing_Capabilities  (HiddCompositingAttrBase + aoHidd_Compositing_Capabilities)
#define aHidd_Compositing_BackFillHook  (HiddCompositingAttrBase + aoHidd_Compositing_BackFillHook)
#define aHidd_Compositing_FrameBuffer   (HiddCompositingAttrBase + aoHidd_Compositing_FrameBuffer)

#define IS_COMPOSITING_ATTR(attr, idx) \
    (((idx) = (attr) - HiddCompositingAttrBase) < num_Hidd_Compositing_Attrs)

struct pHidd_Compositing_BitMapStackChanged
{
    OOP_MethodID                mID;
    struct HIDD_ViewPortData    *data;
    BOOL			*active;
};

struct pHidd_Compositing_BitMapRectChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    WORD            x;
    WORD            y;
    WORD            width;
    WORD            height;
};

struct pHidd_Compositing_BitMapPositionChange
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

#endif /* HIDD_COMPOSITING_H */
