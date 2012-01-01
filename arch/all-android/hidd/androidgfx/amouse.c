/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android mouse input hidd class.
    Lang: English.
*/

#include <aros/debug.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <hidd/mouse.h>

#include <android/input.h>

#include "agfx.h"
#include "agfx_mouse.h"
#include "server.h"

OOP_Object *AMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(EnterFunc("[AMouse] hidd.mouse.gdi::New()\n"));

    /*
     * We can't be instantiated twice. Just in case.
     * We don't want to cope with semaphore protection here because actually nobody
     * will try to instantiate us.
     */
    if (XSD(cl)->mousehidd)
    	return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    D(bug("[AMouse] Object created by superclass: 0x%p\n", o));

    if (o)
    {
	struct mouse_data *data = OOP_INST_DATA(cl, o);
	struct TagItem *tstate = msg->attrList;
	struct TagItem *tag;

	while ((tag = NextTagItem(&tstate)))
	{
	    ULONG idx;

	    if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
	    {
		switch (idx)
		{
		case aoHidd_Mouse_IrqHandler:
		    D(bug("[AMouse] Callback address 0x%p\n", tag->ti_Data));
		    data->mouse_callback = (VOID (*)())tag->ti_Data;
		    break;

		case aoHidd_Mouse_IrqHandlerData:
		    D(bug("[AMouse] Callback data 0x%p\n", tag->ti_Data));
		    data->callbackdata = (APTR)tag->ti_Data;
		    break;
		}
	    }
	} /* while (tags to process) */
	
	/*
	 * Install the mouse hidd.
	 * Our class is final, it's not meant to be subclassed. Additionally, we
	 * report mouse events from within an interrupt, and omitting oop.library calls
	 * speeds things up.
	 */
	XSD(cl)->mousehidd = data;
    }

    return o;
}

/****************************************************************************************/

VOID AMouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    XSD(cl)->mousehidd = NULL;

    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID AMouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    ULONG  idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Mouse_IrqHandler:
		*msg->storage = (IPTR)data->mouse_callback;
		return;

	    case aoHidd_Mouse_IrqHandlerData:
		*msg->storage = (IPTR)data->callbackdata;
		return;

/*	    case aoHidd_Mouse_State:

		TODO: Implement this

		return;*/

	    /* We can report both absolute and relative coordinates */
    	    case aoHidd_Mouse_Extended:
	    	*msg->storage = TRUE;
	    	return;
	}
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

void AMouse_ReportEvent(struct mouse_data *data, struct PointerEvent *pkt)
{
    struct pHidd_Mouse_ExtEvent e;

    DB2(bug("[AMouse] Mouse event 0x%08X at (%d, %d)\n", e.action, e.x, e.y));

    switch (pkt->action)
    {
    case AMOTION_EVENT_ACTION_DOWN:
    	e.button = vHidd_Mouse_Button1;
    	e.type	 = vHidd_Mouse_Press;
    	break;
    	
    case AMOTION_EVENT_ACTION_UP:
    	e.button = vHidd_Mouse_Button1;
    	e.type	 = vHidd_Mouse_Release;
    	break;
    	
    case AMOTION_EVENT_ACTION_MOVE:
    	e.button = vHidd_Mouse_NoButton;
    	e.type	 = vHidd_Mouse_Motion;
    	break;

    default:
    	/* Ignore something we don't know about */
    	return;
    }

    e.x	    = pkt->x;
    e.y	    = pkt->y;
    e.flags = vHidd_Mouse_Relative;

    data->mouse_callback(data->callbackdata, &e);
}

/****************************************************************************************/

void AMouse_ReportTouch(struct mouse_data *data, struct PointerEvent *pkt)
{
    struct pHidd_Mouse_ExtEvent e;

    DB2(bug("[AMouse] Touch event 0x%08X at (%d, %d)\n", e.action, e.x, e.y));

    /*
     * Intuition input handler doesn't recognize mouse button events
     * together with movement. Instead it catches actions but misses
     * the actual movement.
     * In order to work around this, we send two actions instead of one.
     * First we report movement to given coordinates, then action
     * (if press or release happened).
     *
     * TODO: Perhaps we should feed touchscreen events to input.device instead.
     *       Well, this is very experimental anyway.
     */

    e.button = vHidd_Mouse_NoButton;
    e.type   = vHidd_Mouse_Motion;
    e.x	     = pkt->x;
    e.y	     = pkt->y;
    e.flags  = 0;

    data->mouse_callback(data->callbackdata, &e);

    switch (pkt->action)
    {
    case AMOTION_EVENT_ACTION_DOWN:
    	e.button = vHidd_Mouse_Button1;
    	e.type	 = vHidd_Mouse_Press;
    	break;
    	
    case AMOTION_EVENT_ACTION_UP:
    	e.button = vHidd_Mouse_Button1;
    	e.type	 = vHidd_Mouse_Release;
    	break;

    default:
    	/* Ignore something we don't know about */
    	return;
    }

    /* Report the second action */
    data->mouse_callback(data->callbackdata, &e);
}

/****************************************************************************************/

void AMouse_ReportButton(struct mouse_data *data, UWORD button, UWORD action)
{
    struct pHidd_Mouse_ExtEvent e;

    e.button = button;
    e.type   = action;
    e.x      = 0;	/* Relative motion of (0, 0) = no motion */
    e.y      = 0;
    e.flags  = vHidd_Mouse_Relative;

    data->mouse_callback(data->callbackdata, &e);
}
