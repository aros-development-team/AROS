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

#define ATTRBASES_NUM 6

struct VesaGfx_staticdata
{
    OOP_Class 	    	    *vesagfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *vesagfxhidd;
    OOP_Object       	    *visible;		/* Currently visible bitmap */
    struct HWData   	    data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
};

struct VesaGfxBase
{
    struct Library library;
    struct VesaGfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct VesaGfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in vesagfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]

#endif /* HIDD_VESAGFXCLASS_H */
