/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include <X11/Xlib.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include "x11gfx_intern.h"
#include "x11.h"

#undef OOPBase

/* Support functions */
ULONG map_x11_to_hidd(long *penarray, ULONG x11pixel)
{
    ULONG hidd_pen = 0;
    BOOL pix_found = FALSE;
    for (hidd_pen = 0; hidd_pen < 256; hidd_pen ++)
    {
    	if (x11pixel == penarray[hidd_pen])
	{
	    pix_found = TRUE;
	    break;
	}
    }
    if (!pix_found)
    	hidd_pen = 0UL;
	
    return hidd_pen;	
    
}

VOID releaseattrbases(struct abdescr *abd, struct Library * OOPBase)
{
    for (; abd->interfaceid; abd ++)
    {
        if ( *(abd->attrbase) != 0 )
	{
	    ReleaseAttrBase(abd->interfaceid);
	    *(abd->attrbase) = 0;
	}
    }
    return;
}

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase)
{
    struct abdescr *d;
    for (d = abd; d->interfaceid; d ++)
    {
        *(d->attrbase) = ObtainAttrBase(abd->interfaceid);
	if ( *(d->attrbase) == 0 )
	{
	    releaseattrbases(abd, OOPBase);
	    return FALSE;
	}   
    }
    return TRUE;
    
}


#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(o))->UserData)


VOID Hidd_X11Mouse_HandleEvent(Object *o, XEvent *event)
{
    struct pHidd_X11Mouse_HandleEvent msg;
    static MethodID mid = 0;
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_X11Mouse, moHidd_X11Mouse_HandleEvent);
	
    msg.mID = mid;
    msg.event = event;
    
    DoMethod(o, (Msg) &msg);
}


VOID Hidd_X11Kbd_HandleEvent(Object *o, XEvent *event)
{
    struct pHidd_X11Kbd_HandleEvent msg;
    static MethodID mid = 0;
    
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_X11Kbd, moHidd_X11Kbd_HandleEvent);

    msg.mID = mid;
    msg.event = event;
    
    DoMethod(o, (Msg) &msg);
}


