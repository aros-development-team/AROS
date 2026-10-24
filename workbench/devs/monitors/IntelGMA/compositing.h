#ifndef HIDD_COMPOSITING_H
#define HIDD_COMPOSITING_H
/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
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
#   include <hidd/gfx.h>
#endif

/* Compositor interface */
#define CLID_Hidd_GMACompositor   "hidd.gfx.compositor.intelgma"
#define IID_Hidd_GMACompositor    "hidd.gfx.compositor.intelgma"

#define HiddGMACompositorAttrBase __IHidd_GMACompositor

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGMACompositorAttrBase;
#endif

/* Compositor class methods */

enum
{
    moHidd_GMACompositor_BitMapStackChanged = 0,
    moHidd_GMACompositor_BitMapRectChanged,
    moHidd_GMACompositor_BitMapPositionChanged,
    moHidd_GMACompositor_ValidateBitMapPositionChange,

    NUM_COMPOSITING_METHODS
};

enum
{
    aoHidd_GMACompositor_DisplayHidd = 0, /* [I..] Display driver object connected with this compositor object */
    
    num_Hidd_GMACompositor_Attrs
};

#define aHidd_GMACompositor_DisplayHidd  (HiddGMACompositorAttrBase + aoHidd_GMACompositor_DisplayHidd)

#define IS_COMPOSITING_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGMACompositorAttrBase) < num_Hidd_GMACompositor_Attrs)

struct pHidd_GMACompositor_BitMapStackChanged
{
    OOP_MethodID                mID;
    struct HIDD_ViewPortData    *data;
};

struct pHidd_GMACompositor_BitMapRectChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    WORD            x;
    WORD            y;
    WORD            width;
    WORD            height;
};

struct pHidd_GMACompositor_BitMapPositionChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
};

struct pHidd_GMACompositor_ValidateBitMapPositionChange
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    LONG            *newxoffset;
    LONG            *newyoffset;
};

#endif /* HIDD_COMPOSITING_H */
