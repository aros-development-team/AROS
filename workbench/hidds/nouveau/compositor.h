#ifndef HIDD_COMPOSITOR_H
#define HIDD_COMPOSITOR_H
/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef HIDD_GRAPHICS
#   include <hidd/graphics.h>
#endif

/* Compositor interface */
#define CLID_Hidd_Compositor   "hidd.graphics.compositor"
#define IID_Hidd_Compositor    "hidd.graphics.compositor"

#define HiddCompositorAttrBase __IHidd_Compositor

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddCompositorAttrBase;
#endif

/* Compositor class methods */

enum
{
    moHidd_Compositor_BitMapStackChanged = 0,
    moHidd_Compositor_BitMapRectChanged,
    moHidd_Compositor_BitMapPositionChanged,
    moHidd_Compositor_ValidateBitMapPositionChange,

    NUM_COMPOSITOR_METHODS
};

enum
{
    aoHidd_Compositor_GfxHidd = 0, /* [I..] Gfx driver object connected with this compositor object */
    
    num_Hidd_Compositor_Attrs
};

#define aHidd_Compositor_GfxHidd  (HiddCompositorAttrBase + aoHidd_Compositor_GfxHidd)

#define IS_COMPOSITOR_ATTR(attr, idx) \
    (((idx) = (attr) - HiddCompositorAttrBase) < num_Hidd_Compositor_Attrs)

struct pHidd_Compositor_BitMapStackChanged
{
    OOP_MethodID                mID;
    struct HIDD_ViewPortData    *data;
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

struct pHidd_Compositor_BitMapPositionChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
};

struct pHidd_Compositor_ValidateBitMapPositionChange
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    LONG            *newxoffset;
    LONG            *newyoffset;
};

#endif /* HIDD_COMPOSITOR_H */
