/*
    Copyright (C) 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

#undef HiddKbdAB
#define	HiddKbdAB		(CSD(cl)->hiddKbdAB)

static void GlobalCallback(struct kbd_staticdata *csd, UWORD code)
{
    struct kbd_data *data;
    
    for (data = (struct kbd_data *)csd->callbacks.mlh_Head; data->node.mln_Succ;
         data = (struct kbd_data *)data->node.mln_Succ)
	data->callback(data->callbackdata, code);
}

OOP_Object *KBD__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct kbd_data *data;
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

	if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Kbd hidd tag\n"));
	    switch (idx)
	    {
		case aoHidd_Kbd_IrqHandler:
		    data->callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
		    break;

		case aoHidd_Kbd_IrqHandlerData:
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

VOID KBD__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);

    if (data->callback) {
        Disable();
	Remove((struct Node *)data);
	Enable();
    }
    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * The following two methods are small stubs providing means for future expansion.
 * In future we could support enumeration of devices and specifying
 * which device we wish to read events from (in case if we want to implement
 * amigainput.library or something like it)
 */

OOP_Object *KBD__Hidd_Kbd__AddHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_AddHardwareDriver *Msg)
{
    struct TagItem tags[] = {
	{ aHidd_Kbd_IrqHandler	  , (IPTR)GlobalCallback	},
	{ aHidd_Kbd_IrqHandlerData, (IPTR)CSD(cl) 		},
	{ TAG_MORE		  , (IPTR)Msg->tags		}
    };

    return OOP_NewObject(Msg->driverClass, Msg->driverId, tags);
}

void KBD__Hidd_Kbd__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_RemHardwareDriver *Msg)
{
    OOP_DisposeObject(Msg->driverObject);
}

/* Class initialization and destruction */

static int KBD_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[KBD] Base Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_Kbd);

    return TRUE;
}

static int KBD_InitClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[KBD] base class initialization\n"));

    LIBBASE->csd.hiddKbdAB = OOP_ObtainAttrBase(IID_Hidd_Kbd);

    if (LIBBASE->csd.hiddKbdAB)
    {
	NewList((struct List *)&LIBBASE->csd.callbacks);
	D(bug("[KBD] Everything OK\n"));
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(KBD_InitClass, 0)
ADD2EXPUNGELIB(KBD_ExpungeClass, 0)
