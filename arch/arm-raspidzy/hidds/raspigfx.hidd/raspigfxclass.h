#ifndef HIDD_RASPIGFXCLASS_H
#define HIDD_RASPIGFXCLASS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some RasPiGfx useful data.
    Lang: English.
*/

#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "bitmap.h"

#define IID_Hidd_RasPiGfx  "hidd.gfx.raspi"
#define CLID_Hidd_RasPiGfx "hidd.gfx.raspi"

struct RasPiGfx_data
{
    struct Interrupt ResetInterrupt;
};

#define ATTRBASES_NUM 6

struct RasPiGfx_staticdata
{
    OOP_Class 	    	    *raspigfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *raspigfxhidd;
    OOP_Object       	    *visible;       /* Currently visible bitmap */

    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
    OOP_AttrBase	    attrBases[ATTRBASES_NUM];
};

struct RasPiGfxBase
{
    struct Library library;
    struct RasPiGfx_staticdata rpisd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)	(&((struct RasPiGfxBase *)cl->UserData)->rpisd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in raspigfx_init.c */
#define HiddChunkyBMAttrBase	  XSD(cl)->attrBases[0]
#define HiddBitMapAttrBase	  XSD(cl)->attrBases[1]
#define HiddGfxAttrBase		  XSD(cl)->attrBases[2]
#define HiddPixFmtAttrBase	  XSD(cl)->attrBases[3]
#define HiddSyncAttrBase	  XSD(cl)->attrBases[4]
#define HiddAttrBase		  XSD(cl)->attrBases[5]

#endif /* HIDD_RASPIGFXCLASS_H */
