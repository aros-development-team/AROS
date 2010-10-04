/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GDI hidd handling mouse events.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdi.h"

/****************************************************************************************/

static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB  },
    { NULL  	    , NULL  	    }
};

static VOID MouseIntHandler(struct gdimouse_data *data, struct GDI_Control *ctl);

/****************************************************************************************/

OOP_Object * GDIMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
    
    D(EnterFunc("[GDIMouse] hidd.mouse.gdi::New()\n"));
    
    ObtainSemaphoreShared( &XSD(cl)->sema);
    
    if (XSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
	
    ReleaseSemaphore( &XSD(cl)->sema);
    
    if (has_mouse_hidd) { /* Cannot open twice */
    	D(bug("[GDIMouse] Attempt to create a second instance\n"));
    	return NULL; /* Should have some error code here */
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    D(bug("[GDIMouse] Object created by superclass: 0x%p\n"));
    if (o)
    {
	struct gdimouse_data *data = OOP_INST_DATA(cl, o);
	struct GDI_Control   *ctl = XSD(cl)->ctl;
	struct TagItem       *tag, *tstate;
	
	data->buttons = 0;
	data->interrupt = KrnAddIRQHandler(ctl->MouseIrq, MouseIntHandler, data, ctl);
	D(bug("[GDIMouse] Mouse interrupt object: 0x%p\n", data->interrupt));
    	if (data->interrupt) {
	    tstate = msg->attrList;
	    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
	    {
		ULONG idx;
	    
		if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx)) {
		    switch (idx) {
		    case aoHidd_Mouse_IrqHandler:
		        D(bug("[GDIMouse] Callback address 0x%p\n", tag->ti_Data));
		        data->mouse_callback = (VOID (*)())tag->ti_Data;
			break;
		    case aoHidd_Mouse_IrqHandlerData:
		        D(bug("[GDIMouse] Callback data 0x%p\n", tag->ti_Data));
		        data->callbackdata = (APTR)tag->ti_Data;
			break;
		    }
		}
	    } /* while (tags to process) */
	
	    /* Install the mouse hidd */
	    ObtainSemaphore( &XSD(cl)->sema);
	    XSD(cl)->mousehidd = o;
	    ReleaseSemaphore( &XSD(cl)->sema);
	    return o;
	}
    	OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
    	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
        return NULL;
    }
    return o;
}


/****************************************************************************************/

VOID GDIMouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gdimouse_data *data = OOP_INST_DATA(cl, o);
    
    KrnRemIRQHandler(data->interrupt);
    ObtainSemaphore( &XSD(cl)->sema);
    XSD(cl)->mousehidd = NULL;
    ReleaseSemaphore( &XSD(cl)->sema);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

static BOOL check_button(UWORD new, UWORD mask, UWORD arosbutton, struct pHidd_Mouse_Event *e, struct gdimouse_data *data)
{
    UWORD old = data->buttons & mask;

    new &= mask;
    if (old != new) {
        e->button = arosbutton;
        e->type = new ? vHidd_Mouse_Press : vHidd_Mouse_Release;
        data->mouse_callback(data->callbackdata, e);
        return TRUE;
    } else
        return FALSE;
}

static VOID MouseIntHandler(struct gdimouse_data *data, struct GDI_Control *MOUSEDATA)
{
    struct pHidd_Mouse_Event e;
    UWORD new_buttons;
    BOOL button;
    /* Due to asynchronous nature of host-side window service thread GDI_Control structure acts like hardware registers.
       There can be many pending events before we get here and the structure will hold a summary of all states, however
       MouseEvent will contain only last event. Because of this we read it ASAP and pay as little attention to MouseEvent
       as possible.
    */

    D(bug("[GDIMouse] Interrupt\n"));
    switch(MOUSEDATA->MouseEvent) {
    case WM_MOUSEWHEEL:
        /* Wheel delta comes only with WM_MOUSEWHEEL, otherwise it's zero */
        e.y = -MOUSEDATA->WheelDelta; /* Windows gives us inverted data */
        e.x = 0;
    	e.type   = vHidd_Mouse_WheelMotion;
    	e.button = vHidd_Mouse_NoButton;
    	data->mouse_callback(data->callbackdata, &e);
    	break;
    default:
    	e.x = MOUSEDATA->MouseX;
    	e.y = MOUSEDATA->MouseY;
    	new_buttons = MOUSEDATA->Buttons;

    	button = check_button(new_buttons, MK_LBUTTON, vHidd_Mouse_Button1, &e, data);
    	button |= check_button(new_buttons, MK_RBUTTON, vHidd_Mouse_Button2, &e, data);
    	button |= check_button(new_buttons, MK_MBUTTON, vHidd_Mouse_Button3, &e, data);
    	if (button)
    	    data->buttons = new_buttons;
    	else {
    	    e.button = vHidd_Mouse_NoButton;
	    e.type = vHidd_Mouse_Motion;
	    data->mouse_callback(data->callbackdata, &e);
	}
    }
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

/****************************************************************************************/

static int GDIMouse_Init(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int GDIMouse_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GDIMouse_Init, 0)
ADD2EXPUNGELIB(GDIMouse_Expunge, 0)
