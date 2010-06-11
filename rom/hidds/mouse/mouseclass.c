/*
    Copyright (C) 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/mouse.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

#undef HiddMouseAB
#define	HiddMouseAB		(CSD(cl)->hiddMouseAB)

static void GlobalCallback(struct driverNode *drv, struct pHidd_Mouse_ExtEvent *ev)
{
    struct pHidd_Mouse_ExtEvent xev;
    struct mouse_data *data;

/* The event passed in may be pHidd_Mouse_Event instead of pHidd_Mouse_ExtEvent,
   according to flags. In this case we add own flags */
    if (drv->flags != vHidd_Mouse_Extended) {
        xev.button = ev->button;
	xev.x      = ev->x;
	xev.y      = ev->y;
	xev.type   = ev->type;
        xev.flags  = drv->flags;
	
	ev = &xev;
    }

    for (data = (struct mouse_data *)drv->callbacks->mlh_Head; data->node.mln_Succ;
         data = (struct mouse_data *)data->node.mln_Succ)
	data->callback(data->callbackdata, ev);
}

OOP_Object *Mouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct mouse_data *data;
    struct TagItem *tag, *tstate;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o)
        return NULL;

    data = OOP_INST_DATA(cl, o);
    data->callback = NULL;

    tstate = msg->attrList;
    D(bug("tstate: %p\n", tstate));

    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
	ULONG idx;

	D(bug("Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));

	if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Mouse hidd tag\n"));
	    switch (idx)
	    {
		case aoHidd_Mouse_IrqHandler:
		    data->callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
		    break;

		case aoHidd_Mouse_IrqHandlerData:
		    data->callbackdata = (APTR)tag->ti_Data;
		    D(bug("Got data %p\n", (APTR)tag->ti_Data));
		    break;
	    }
	}
    } /* while (tags to process) */

    /* Add to interrupts list if we have a callback */
    if (data->callback) {
        Disable();
        AddTail((struct List *)&CSD(cl)->callbacks, (struct Node *)data);
	Enable();
    }

    return o;
}

VOID Mouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);

    if (data->callback) {
        Disable();
	Remove((struct Node *)data);
	Enable();
    }
    OOP_DoSuperMethod(cl, o, msg);
}

VOID Mouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    ULONG   	       idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Mouse_IrqHandler:
		*msg->storage = (IPTR)data->callback;
		return;

	    case aoHidd_Mouse_IrqHandlerData:
		*msg->storage = (IPTR)data->callbackdata;
		return;

/*	    case aoHidd_Mouse_State:

		TODO: Implement this, by ORing buttons from all registered mice (?)

		return;*/

    	    case aoHidd_Mouse_Extended:
	    	*msg->storage = TRUE;
	    	return;
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * In future we could support enumeration of devices and specifying
 * which device we wish to read events from (in case if we want to implement
 * amigainput.library or something like it)
 */

OOP_Object *Mouse__Hidd_Mouse__AddHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_AddHardwareDriver *Msg)
{
    struct TagItem tags[] = {
	{ aHidd_Mouse_IrqHandler    , (IPTR)GlobalCallback},
	{ aHidd_Mouse_IrqHandlerData, 0 		  },
	{ TAG_MORE		    , (IPTR)Msg->tags	  }
    };
    struct driverNode *drvnode;

    D(bug("[Mouse] AddHardwareDriver(0x%p)\n", Msg->driverClass));

    drvnode = AllocMem(sizeof(struct driverNode), MEMF_PUBLIC);  
    if (!drvnode)
	return NULL;

    tags[1].ti_Data = (IPTR)drvnode;
    drvnode->drv = OOP_NewObject(Msg->driverClass, NULL, tags);
    D(bug("[Mouse] Driver node 0x%p, driver 0x%p\n", drvnode, drvnode->drv));

    if (drvnode->drv) {
        struct mouse_staticdata *csd = CSD(cl);
        IPTR val = FALSE;

	drvnode->callbacks = &csd->callbacks;

	OOP_GetAttr(drvnode->drv, aHidd_Mouse_Extended, &val);
	D(bug("[Mouse] Extended event: %d\n", val));
	if (val)
	    drvnode->flags = vHidd_Mouse_Extended;
	else {
            OOP_GetAttr(drvnode->drv, aHidd_Mouse_RelativeCoords, &val);
	    D(bug("[Mouse] Relative coordinates: %d\n", val));
            drvnode->flags = val ? vHidd_Mouse_Relative : 0;
	}

	ObtainSemaphore(&csd->drivers_sem);
	AddTail((struct List *)&csd->drivers, (struct Node *)drvnode);
	ReleaseSemaphore(&csd->drivers_sem);

	return drvnode->drv;
    }

    FreeMem(drvnode, sizeof(struct driverNode));
    return NULL;
}

void Mouse__Hidd_Mouse__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_RemHardwareDriver *Msg)
{
    struct mouse_staticdata *csd = CSD(cl);
    struct driverNode *node;

    ObtainSemaphore(&csd->drivers_sem);

    for (node = (struct driverNode *)csd->drivers.mlh_Head; node->node.mln_Succ;
         node = (struct driverNode *)node->node.mln_Succ) {
	if (node->drv == Msg->driverObject) {
	    Remove((struct Node *)node);
	    break;
	}
    }

    ReleaseSemaphore(&csd->drivers_sem);

    OOP_DisposeObject(Msg->driverObject);

    if (node->drv == Msg->driverObject)
        FreeMem(node, sizeof(struct driverNode));
}

/* Class initialization and destruction */

static int Mouse_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Mouse] Base Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_Mouse);

    return TRUE;
}

static int Mouse_InitClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Mouse] base class initialization\n"));

    LIBBASE->csd.hiddMouseAB = OOP_ObtainAttrBase(IID_Hidd_Mouse);

    if (LIBBASE->csd.hiddMouseAB)
    {
	NewList((struct List *)&LIBBASE->csd.callbacks);
	NewList((struct List *)&LIBBASE->csd.drivers);
	InitSemaphore(&LIBBASE->csd.drivers_sem);
	D(bug("[Mouse] Everything OK\n"));
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(Mouse_InitClass, 0)
ADD2EXPUNGELIB(Mouse_ExpungeClass, 0)
