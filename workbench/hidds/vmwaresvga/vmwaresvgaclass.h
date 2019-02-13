#ifndef _VMWARESVGA_CLASS_H
#define _VMWARESVGA_CLASS_H

/*
    Copyright � 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VMWareSVGA useful data.
    Lang: English.
*/

#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/semaphores.h>

#include "vmwaresvgahardware.h"
#include "vmwaresvgabitmap.h"

#define IID_Hidd_VMWareSVGA  "hidd.gfx.vmwaresvga"
#define CLID_Hidd_VMWareSVGA "hidd.gfx.vmwaresvga"

struct VMWareSVGA_staticdata {
	struct MemHeader mh;
    OOP_Class *basebm;            /* baseclass for CreateObject */
	OOP_Class *vmwaresvgaclass;
	OOP_Class *vmwaresvgaonbmclass;
	OOP_Class *vmwaresvgaoffbmclass;
	OOP_Object *vmwaresvgahidd;
	OOP_Object *card;
	OOP_Object *pcihidd;
	struct BitmapData *visible;
	VOID (*activecallback)(APTR, OOP_Object *, BOOL);
	APTR callbackdata;
	struct MouseData mouse;
	struct HWData data;
};

struct VMWareSVGABase
{
    struct Library library;
    
    struct VMWareSVGA_staticdata vsd;    
};

#define XSD(cl) (&((struct VMWareSVGABase *)cl->UserData)->vsd)

#endif /* _VMWARESVGA_CLASS_H */
