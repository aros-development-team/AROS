#ifndef HIDD_COMPOSITOR_H
#define HIDD_COMPOSITOR_H
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
#define CLID_Hidd_AmigaVideoCompositor   "hidd.gfx.compositor.amigavideo"
#define IID_Hidd_AmigaVideoCompositor    "hidd.gfx.compositor.amigavideo"

#define HiddAmigaVideoCompositorAttrBase __IHidd_AmigaVideoCompositor

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddAmigaVideoCompositorAttrBase;
#endif

/* Compositor class methods */

enum
{
    moHidd_AmigaVideoCompositor_BitMapStackChanged = 0,
    moHidd_AmigaVideoCompositor_BitMapRectChanged,
    moHidd_AmigaVideoCompositor_BitMapPositionChanged,
    moHidd_AmigaVideoCompositor_ValidateBitMapPositionChange,

    NUM_AMIGAVIDEOCOMPOSITOR_METHODS
};

enum
{
    aoHidd_AmigaVideoCompositor_DisplayHidd = 0, /* [I..] Display driver object connected with this compositor object */
    
    num_Hidd_AmigaVideoCompositor_Attrs
};

#define aHidd_AmigaVideoCompositor_DisplayHidd  (HiddAmigaVideoCompositorAttrBase + aoHidd_AmigaVideoCompositor_DisplayHidd)

#define IS_AMIGAVIDEOCOMPOSITOR_ATTR(attr, idx) \
    (((idx) = (attr) - HiddAmigaVideoCompositorAttrBase) < num_Hidd_AmigaVideoCompositor_Attrs)

struct pHidd_AmigaVideoCompositor_BitMapStackChanged
{
    OOP_MethodID                mID;
    struct HIDD_ViewPortData    *data;
};

struct pHidd_AmigaVideoCompositor_BitMapRectChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    WORD            x;
    WORD            y;
    WORD            width;
    WORD            height;
};

struct pHidd_AmigaVideoCompositor_BitMapPositionChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
};

struct pHidd_AmigaVideoCompositor_ValidateBitMapPositionChange
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    LONG            *newxoffset;
    LONG            *newyoffset;
};

#endif /* HIDD_COMPOSITOR_H */
