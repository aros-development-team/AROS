/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd handling mouse events.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <hidd/mouse.h>

#include <aros/symbolsets.h>

#include "linuxinput_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

OOP_Object *LinuxMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;

    ObtainSemaphore(&LSD(cl)->sema);
    if (LSD(cl)->mousehidd)
        has_mouse_hidd = TRUE;
    ReleaseSemaphore(&LSD(cl)->sema);

    if (has_mouse_hidd) /* Cannot open twice */
        return NULL; /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct LinuxMouse_data *data = OOP_INST_DATA(cl, o);
        struct TagItem *tag, *tstate;
        
        tstate = msg->attrList;
        while ((tag = NextTagItem(&tstate)))
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
        ObtainSemaphore(&LSD(cl)->sema);
        LSD(cl)->mousehidd = o;
        Update_EventHandlers(LSD(cl));
        ReleaseSemaphore(&LSD(cl)->sema);
    }
    return o;
}

VOID LinuxMouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&LSD(cl)->sema);
    LSD(cl)->mousehidd = NULL;
    Update_EventHandlers(LSD(cl));
    ReleaseSemaphore(&LSD(cl)->sema);

    OOP_DoSuperMethod(cl, o, msg);

}


VOID LinuxMouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Mouse_RelativeCoords:
            *msg->storage = TRUE;
            return;
        }
    }
    
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


VOID LinuxMouse__Hidd_LinuxMouse__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_LinuxMouse_HandleEvent *msg)
{

    struct LinuxMouse_data *data = OOP_INST_DATA(cl, o);
    
    data->mouse_callback(data->callbackdata, msg->mouseEvent);
    
    return;
}

VOID HIDD_LinuxMouse_HandleEvent(OOP_Object *o, struct pHidd_Mouse_Event *mouseEvent)
{
    static OOP_MethodID mid = 0;
    struct pHidd_LinuxMouse_HandleEvent p;
    
    if (!mid)
        mid = OOP_GetMethodID(IID_Hidd_LinuxMouse, moHidd_LinuxMouse_HandleEvent);
    
    p.mID        = mid;
    p.mouseEvent    = mouseEvent;
    
    OOP_DoMethod(o, (OOP_Msg)&p);
}
