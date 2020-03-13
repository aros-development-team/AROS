#ifndef SAGAGFX_HIDD_H
#define SAGAGFX_HIDD_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAGAGfx header.
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <oop/oop.h>
#include <hidd/gfx.h>

#define ATTRBASES_NUM 7

#define CLID_Hidd_Gfx_SAGA "hidd.gfx.saga"

struct SAGAGfx_data
{
    int __dummy__;
};

struct SAGAGfx_staticdata
{
    OOP_Class *     basebm;
    OOP_Class *     sagagfxclass;
    OOP_Class *     bmclass;
    OOP_Object *    sagagfxhidd;
    OOP_Object *    visible;        /* Currently visible bitmap */
    OOP_AttrBase    attrBases[ATTRBASES_NUM];

    UBYTE           cursor_clut[16*16];
    UWORD           cursor_pal[4];
    UBYTE           cursor_visible;

    WORD            cursorX;
    WORD            cursorY;
    WORD           hotX;
    WORD           hotY;
    APTR            mempool;

    BOOL            useHWSprite;
//    struct SignalSemaphore  framebufferlock;
};

struct SAGAGfxBase
{
    struct Library library;
    struct SAGAGfx_staticdata vsd;
};

#define XSD(cl)	(&((struct SAGAGfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase
#undef HiddColorMapAttrBase

/* These must stay in the same order as interfaces[] array in sm502gfx_init.c */
#define HiddChunkyBMAttrBase  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase    XSD(cl)->attrBases[1]
#define HiddGfxAttrBase       XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase    XSD(cl)->attrBases[3]
#define HiddSyncAttrBase      XSD(cl)->attrBases[4]
#define HiddAttrBase          XSD(cl)->attrBases[5]
#define HiddColorMapAttrBase  XSD(cl)->attrBases[6]

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define METHOD_NAME(base, id, name) \
  base ## __ ## id ## __ ## name

#define METHOD_NAME_S(base, id, name) \
  # base "__" # id "__" # name


#endif /* SAGAGFX_HIDD_H */
