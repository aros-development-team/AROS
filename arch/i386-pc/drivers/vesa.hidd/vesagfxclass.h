#ifndef HIDD_VESAGFXCLASS_H
#define HIDD_VESAGFXCLASS_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

struct VesaGfx_staticdata
{
    OOP_Class 	    	    *vesagfxclass;
    OOP_Class 	    	    *bmclass;
    OOP_Object      	    *vesagfxhidd;
    OOP_Object       	    *visible;		/* Currently visible bitmap */
    struct HWData   	    data;
    struct SignalSemaphore  framebufferlock;
    struct SignalSemaphore  HW_acc;
};

struct VesaGfxBase
{
    struct Library library;
    struct VesaGfx_staticdata vsd;
};

#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)

#define XSD(cl)     	    	(&((struct VesaGfxBase *)cl->UserData)->vsd)

#endif /* HIDD_VESAGFXCLASS_H */
