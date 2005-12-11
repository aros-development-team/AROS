/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd handling mouse events.
    Lang: English.
*/

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <X11/Xlib.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "x11.h"

/****************************************************************************************/

static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB  },
    { NULL  	    , NULL  	    }
};

/****************************************************************************************/

static ULONG xbutton2hidd(XButtonEvent *xb)
{
    ULONG button;
    
    switch (xb->button)
    {
	case Button1:
	    button = vHidd_Mouse_Button1;
	    break;
	
	case Button2:
	    button = vHidd_Mouse_Button3;
	    break;
	    
	case Button3:
	    button = vHidd_Mouse_Button2;
	    break;
	
    }
    
    return button;
}

/****************************************************************************************/

OOP_Object * X11Mouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
    
    ObtainSemaphoreShared( &XSD(cl)->sema);
    
    if (XSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
	
    ReleaseSemaphore( &XSD(cl)->sema);
    
    if (has_mouse_hidd) /* Cannot open twice */
    	return NULL; /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct x11mouse_data *data = OOP_INST_DATA(cl, o);
	struct TagItem       *tag, *tstate;
	
	tstate = msg->attrList;
	while ((tag = NextTagItem((const struct TagItem **)&tstate)))
	{
	    ULONG idx;
	    
	    if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
	    {
	    	switch (idx)
		{
		    case aoHidd_Mouse_IrqHandler:
		    	data->mouse_callback = (VOID (*)())tag->ti_Data;
			break;
			
		    case aoHidd_Mouse_IrqHandlerData:
		    	data->callbackdata = (APTR)tag->ti_Data;
			break;
		}
	    }
	    
	} /* while (tags to process) */
	
	/* Install the mouse hidd */
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->mousehidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
	
    }
    
    return o;
}

/****************************************************************************************/

VOID X11Mouse__Hidd_X11Mouse__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_X11Mouse_HandleEvent *msg)
{

    struct x11mouse_data    	*data = OOP_INST_DATA(cl, o);    
    struct pHidd_Mouse_Event 	 e;
    
    XButtonEvent *xb = &(msg->event->xbutton);
    
    e.x = xb->x;
    e.y = xb->y;
   
    if (msg->event->type == ButtonRelease)
    {
    	switch(xb->button)
	{
	    case Button1:
	    case Button2:
	    case Button3:
    	    	e.button = xbutton2hidd(xb);
	    	e.type   = vHidd_Mouse_Release;
	    	data->mouse_callback(data->callbackdata, &e);
		break;
	}
    }
    else if (msg->event->type == ButtonPress)
    {
    	switch(xb->button)
	{
	    case Button1:
	    case Button2:
	    case Button3:	    	
    		e.button = xbutton2hidd(xb);
		e.type   = vHidd_Mouse_Press;
        	data->mouse_callback(data->callbackdata, &e);
		break;
		
	    case Button4:
	    	e.type   = vHidd_Mouse_WheelMotion;
		e.button = vHidd_Mouse_NoButton;
		e.x      = 0;
		e.y      = -1;
        	data->mouse_callback(data->callbackdata, &e);
		break;
		
	    case Button5:
	    	e.type   = vHidd_Mouse_WheelMotion;
		e.button = vHidd_Mouse_NoButton;
		e.x 	 = 0;
		e.y 	 = 1;
        	data->mouse_callback(data->callbackdata, &e);
		break;
		
	}
    }
    else if (msg->event->type == MotionNotify)
    {
    	e.button = vHidd_Mouse_NoButton;
	e.type = vHidd_Mouse_Motion;
	
        data->mouse_callback(data->callbackdata, &e);
    }
    
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

/****************************************************************************************/

AROS_SET_LIBFUNC(X11Mouse_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    return OOP_ObtainAttrBases(attrbases);

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(X11Mouse_Expunge, LIBBASETYPE, LIBBASE)
{

    AROS_SET_LIBFUNC_INIT
    
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(X11Mouse_Init, 0)
ADD2EXPUNGELIB(X11Mouse_Expunge, 0)
