/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: X11 gfx HIDD for AROS.
*/

#include "x11_debug.h"


#include <proto/oop.h>

#include "x11_types.h"
#include "x11.h"

/****************************************************************************************/

VOID Hidd_Mouse_X11_HandleEvent(OOP_Object *o, XEvent *event)
{
    struct pHidd_Mouse_X11_HandleEvent msg;
    static OOP_MethodID mid;
    
    if (!mid)
        mid = OOP_GetMethodID(IID_Hidd_Mouse_X11, moHidd_Mouse_X11_HandleEvent);
        
    msg.mID = mid;
    msg.event = event;
    
    OOP_DoMethod(o, (OOP_Msg) &msg);
}

/****************************************************************************************/

VOID Hidd_Kbd_X11_HandleEvent(OOP_Object *o, XEvent *event)
{
    struct pHidd_Kbd_X11_HandleEvent msg;
    static OOP_MethodID mid;
        
    if (!mid)
        mid = OOP_GetMethodID(IID_Hidd_Kbd_X11, moHidd_Kbd_X11_HandleEvent);

    msg.mID = mid;
    msg.event = event;
    
    OOP_DoMethod(o, (OOP_Msg) &msg);
}

/****************************************************************************************/


