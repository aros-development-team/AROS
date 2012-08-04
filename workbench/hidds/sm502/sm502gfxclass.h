#ifndef HIDD_SM502GFXCLASS_H
#define HIDD_SM502GFXCLASS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some SM502Gfx useful data.
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "bitmap.h"
#include "hardware.h"

#define CLID_Hidd_SM502Gfx "hidd.gfx.sm502"

struct SM502Gfx_data
{
    struct Interrupt ResetInterrupt;
};

#define ATTRBASES_NUM 6

struct SM502Gfx_staticdata
{
    OOP_Class 	    	    *sm502gfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *sm502gfxhidd;
    OOP_Object       	    *visible;		/* Currently visible bitmap */
    struct SM502_HWData      data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
};

struct SM502GfxBase
{
    struct Library library;
    struct SM502Gfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct SM502GfxBase *)cl->UserData)->vsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in sm502gfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]

#endif /* HIDD_SM502GFXCLASS_H */
