#ifndef HIDD_VESAGFXCLASS_H
#define HIDD_VESAGFXCLASS_H

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VesaGfx useful data.
    Lang: English.
*/

#define BUFFERED_VRAM	    1

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "hardware.h"
#include "bitmap.h"
#include "mouse.h"

#define IID_Hidd_VesaGfx  "hidd.gfx.vesa"
#define CLID_Hidd_VesaGfx "hidd.gfx.vesa"

struct VesaGfx_staticdata
{
    struct ExecBase 	    *sysBase;
    struct Library  	    *oopBase;
    struct Library  	    *utilityBase;
    struct MemHeader 	    mh;
    OOP_Class 	    	    *vesagfxclass;
    OOP_Class 	    	    *onbmclass;
    OOP_Class 	    	    *offbmclass;
    OOP_Object      	    *vesagfxhidd;
    OOP_Object      	    *pcihidd;
    struct bitmap_data      *visible;
    VOID	    	    (*activecallback)(APTR, OOP_Object *, BOOL);
    APTR	    	    callbackdata;
    struct MouseData 	    mouse;
    struct HWData   	    data;
#if BUFFERED_VRAM
    struct SignalSemaphore  framebufferlock;
#endif
};

#if BUFFERED_VRAM
#define LOCK_FRAMEBUFFER(xsd)	ObtainSemaphore(&xsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(xsd) ReleaseSemaphore(&xsd->framebufferlock)
#endif

#define XSD(cl)     	    	((struct VesaGfx_staticdata *)cl->UserData)
#define UtilityBase 	    	((struct Library *)XSD(cl)->utilityBase)
#define OOPBase     	    	((struct Library *)XSD(cl)->oopBase)
#define SysBase     	    	(XSD(cl)->sysBase)

OOP_Class *init_vesagfxclass(struct VesaGfx_staticdata *);
VOID free_vesagfxclass(struct VesaGfx_staticdata *);

#endif /* HIDD_VESAGFXCLASS_H */
