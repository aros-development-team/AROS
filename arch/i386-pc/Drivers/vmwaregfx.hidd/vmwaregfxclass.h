#ifndef HIDD_VMWAREGFXCLASS_H
#define HIDD_VMWAREGFXCLASS_H

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Some VMWareGfx useful data.
    Lang: English.
*/

#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include "hardware.h"
#include "bitmap.h"
#include "mouse.h"

#define IID_Hidd_VMWareGfx  "hidd.gfx.vmware"
#define CLID_Hidd_VMWareGfx "hidd.gfx.vmware"

struct VMWareGfx_staticdata {
	struct ExecBase *sysBase;
	struct Library *oopBase;
	struct Library *utilityBase;
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

#define XSD(cl) ((struct VMWareGfx_staticdata *)cl->UserData)
#define UtilityBase ((struct Library *)XSD(cl)->utilityBase)
#define OOPBase ((struct Library *)XSD(cl)->oopBase)
#define SysBase (XSD(cl)->sysBase)

OOP_Class *init_vmwaregfxclass(struct VMWareGfx_staticdata *);
VOID free_vmwaregfxclass(struct VMWareGfx_staticdata *);

#endif /* HIDD_VGACLASS_H */
