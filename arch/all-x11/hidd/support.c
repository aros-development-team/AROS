/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include <X11/Xlib.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include <hidd/graphics.h>
#include <aros/debug.h>

#include "x11gfx_intern.h"
#include "x11.h"

#include <X11/Xlib.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(o))

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


