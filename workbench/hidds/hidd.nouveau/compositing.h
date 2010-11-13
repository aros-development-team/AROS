#ifndef HIDD_COMPOSITING_H
#define HIDD_COMPOSITING_H
/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
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

/* Compositing interface */
#define CLID_Hidd_Compositing   "hidd.graphics.compositing"
#define IID_Hidd_Compositing    "hidd.graphics.compositing"

#define HiddCompositingAttrBase __IHidd_Compositing

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddCompositingAttrBase;
#endif

/* Compositing class methods */

enum
{
    moHidd_Compositing_BitMapStackChanged = 0,
    moHidd_Compositing_BitMapRectChanged,
    moHidd_Compositing_BitMapPositionChanged,
    moHidd_Compositing_ValidateBitMapPositionChange,

    NUM_COMPOSITING_METHODS
};

enum
{
    aoHidd_Compositing_GfxHidd = 0, /* [I..] Gfx driver object connected with this compositing object */
    
    num_Hidd_Compositing_Attrs
};

#define aHidd_Compositing_GfxHidd  (HiddCompositingAttrBase + aoHidd_Compositing_GfxHidd)

#define IS_COMPOSITING_ATTR(attr, idx) \
    (((idx) = (attr) - HiddCompositingAttrBase) < num_Hidd_Compositing_Attrs)

struct pHidd_Compositing_BitMapStackChanged
{
    OOP_MethodID                mID;
    struct HIDD_ViewPortData    *data;
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

struct pHidd_Compositing_BitMapPositionChanged
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
};

struct pHidd_Compositing_ValidateBitMapPositionChange
{
    OOP_MethodID    mID;
    OOP_Object      *bm;
    LONG            *newxoffset;
    LONG            *newyoffset;
};

#endif /* HIDD_COMPOSITING_H */
