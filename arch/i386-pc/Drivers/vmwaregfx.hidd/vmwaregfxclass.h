#ifndef HIDD_VMWAREGFXCLASS_H
#define HIDD_VMWAREGFXCLASS_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VMWareGfx useful data.
    Lang: English.
*/

#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include "hardware.h"
#include "bitmap.h"

#define IID_Hidd_VMWareGfx  "hidd.gfx.vmware"
#define CLID_Hidd_VMWareGfx "hidd.gfx.vmware"

struct VMWareGfx_staticdata {
	struct MemHeader mh;
	OOP_Class *vmwaregfxclass;
	OOP_Class *onbmclass;
	OOP_Class *offbmclass;
	OOP_Object *vmwaregfxhidd;
	OOP_Object *card;
	OOP_Object *pcihidd;
	struct bitmap_data *visible;
	VOID	(*activecallback)(APTR, OOP_Object *, BOOL);
	APTR	callbackdata;
	struct MouseData mouse;
	struct HWData data;
};

struct VMWareGfxBase
{
    struct Library library;
    struct ExecBase *sysBase;
    BPTR	SegList;
    
    struct VMWareGfx_staticdata vsd;    
};

#define XSD(cl) (&((struct VMWareGfxBase *)cl->UserData)->vsd)

#endif /* HIDD_VMWAREGFXCLASS_H */
