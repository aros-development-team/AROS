#ifndef HIDD_VESAGFXCLASS_H
#define HIDD_VESAGFXCLASS_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VesaGfx useful data.
    Lang: English.
*/

#define BUFFERED_VRAM	    1

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "bitmap.h"
#include "hardware.h"
#include "mouse.h"

#define IID_Hidd_VesaGfx  "hidd.gfx.vesa"
#define CLID_Hidd_VesaGfx "hidd.gfx.vesa"

struct VesaGfx_staticdata
{
    struct MemHeader 	    mh;
    OOP_Class 	    	    *vesagfxclass;
    OOP_Class 	    	    *onbmclass;
    OOP_Class 	    	    *offbmclass;
    OOP_Object      	    *vesagfxhidd;
    OOP_Object      	    *pcihidd;
    struct BitmapData       *visible;
    VOID	    	    (*activecallback)(APTR, OOP_Object *, BOOL);
    APTR	    	    callbackdata;
    struct MouseData 	    mouse;
    struct HWData   	    data;
#if BUFFERED_VRAM
    struct SignalSemaphore  framebufferlock;
#endif
};

struct VesaGfxBase
{
    struct Library library;
    struct ExecBase *sysBase;
    BPTR	SegList;
    
    struct VesaGfx_staticdata vsd;
};

#if BUFFERED_VRAM
#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)
#endif

#define XSD(cl)     	    	(&((struct VesaGfxBase *)cl->UserData)->vsd)

#endif /* HIDD_VESAGFXCLASS_H */
