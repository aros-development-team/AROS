#ifndef HIDD_VESAGFXCLASS_H
#define HIDD_VESAGFXCLASS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VesaGfx useful data.
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "bitmap.h"
#include "hardware.h"

#define IID_Hidd_VesaGfx  "hidd.gfx.vesa"
#define CLID_Hidd_VesaGfx "hidd.gfx.vesa"

struct VesaGfx_data
{
    struct Interrupt ResetInterrupt;
};

#define ATTRBASES_NUM 9

struct VesaGfx_staticdata
{
    OOP_Class 	    	    *vesagfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Class 	    	    *compositingclass;
    OOP_Object      	    *vesagfxhidd;
    OOP_Object      	    *compositing;
    OOP_Object      	    *gc;
    struct HWData   	    data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
    ULONG                   *cursor_pixels;
    OOP_Object              *cursor_scratch_bm;
    WORD                    cursor_x;
    WORD                    cursor_y;
    UWORD                   cursor_width;
    UWORD                   cursor_height;
    BOOL                    cursor_visible;

    OOP_MethodID    mid_BitMapStackChanged;
    OOP_MethodID    mid_BitMapPositionChanged;
    OOP_MethodID    mid_BitMapRectChanged;
    OOP_MethodID    mid_ValidateBitMapPositionChange;
    OOP_MethodID    mid_DisplayRectChanged;
};

struct VesaGfxBase
{
    struct Library library;
    struct VesaGfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct VesaGfxBase *)cl->UserData)->vsd)

#undef HiddVesaGfxBitMapAttrBase
#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddCompositingAttrBase
#undef HiddGCAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in vesagfx_init.c */
#define HiddVesaGfxBitMapAttrBase XSD(cl)->attrBases[0]
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[1]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[2]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[3]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[4]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[5]
#define HiddCompositingAttrBase	  XSD(cl)->attrBases[6]
#define HiddGCAttrBase		  XSD(cl)->attrBases[7]
#define HiddAttrBase		  XSD(cl)->attrBases[8]

#endif /* HIDD_VESAGFXCLASS_H */
