/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include "x11_debug.h"


#include <proto/oop.h>

#include "x11_types.h"
#include "x11.h"

/****************************************************************************************/

VOID Hidd_X11Mouse_HandleEvent(OOP_Object *o, XEvent *event)
{
    struct pHidd_X11Mouse_HandleEvent msg;
    static OOP_MethodID mid;
    
    if (!mid)
    	mid = OOP_GetMethodID(IID_Hidd_X11Mouse, moHidd_X11Mouse_HandleEvent);
	
    msg.mID = mid;
    msg.event = event;
    
    OOP_DoMethod(o, (OOP_Msg) &msg);
}

/****************************************************************************************/

VOID Hidd_X11Kbd_HandleEvent(OOP_Object *o, XEvent *event)
{
    struct pHidd_X11Kbd_HandleEvent msg;
    static OOP_MethodID mid;
        
    if (!mid)
    	mid = OOP_GetMethodID(IID_Hidd_X11Kbd, moHidd_X11Kbd_HandleEvent);

    msg.mID = mid;
    msg.event = event;
    
    OOP_DoMethod(o, (OOP_Msg) &msg);
}

/****************************************************************************************/


