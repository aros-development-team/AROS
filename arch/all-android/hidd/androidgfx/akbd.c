/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android keyboard hidd class.
    Lang: English.
*/

#include <aros/debug.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <hidd/keyboard.h>
#include <devices/rawkeycodes.h>

#include "agfx.h"
#include "agfx_keyboard.h"

/****************************************************************************************/

OOP_Object *AKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("AKbd::New()\n"));

    if (XSD(cl)->kbdhidd)
    	return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct kbd_data *data = OOP_INST_DATA(cl, o);
	const struct TagItem *tstate = msg->attrList;
	struct TagItem *tag;

	while ((tag = NextTagItem(&tstate)))
    	{
	    ULONG idx;
	    
	    if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
	    {
	    	switch (idx)
	    	{
	    	case aoHidd_Kbd_IrqHandler:
		    data->callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", tag->ti_Data));
		    break;
			
		case aoHidd_Kbd_IrqHandlerData:
		    data->callbackdata = (APTR)tag->ti_Data;
		    D(bug("Got data %p\n", tag->ti_Data));
		    break;
		}
	    }
	}
	XSD(cl)->kbdhidd = o;
    }

    ReturnPtr("AKbd::New", OOP_Object *, o);
}

/****************************************************************************************/

VOID AKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("[AKbd] Dispose()\n"));

    XSD(cl)->kbdhidd = NULL;
    OOP_DoSuperMethod(cl, o, msg);
}
