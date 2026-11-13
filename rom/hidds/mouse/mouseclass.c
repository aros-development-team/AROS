/*
    Copyright (C) 2004-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/mouse.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "mouse.h"

#define SysBase     ((struct ExecBase *)(CSD(cl)->cs_SysBase))
#define UtilityBase (CSD(cl)->cs_UtilityBase)

/*****************************************************************************************

    NAME
        --background_mouseclass--

    LOCATION
        CLID_Hidd_Mouse

    NOTES
        Instances of this class are virtual devices being clients of the
        pointing input subsystem. In order to receive input events, you
        have to create an object of this class and supply a callback using
        aoHidd_Input_IrqHandler attribute. After this your callback will be
        called every time the event arrives until you dispose your object.

        Every client receives events from all pointing devices merged into
        a single stream.

        Mouse event handlers are specified by passing the aHidd_Input_IrqHandler attrib,
        The handler will be called every time a mouse event occurs.  Handlers
        should be declared using 'C' calling conventions,
        e.g.:

        void MouseIRQ(APTR data, struct pHidd_Mouse_ExtEvent *event);

        Handler parameters are:
            data  - Anything you specify using aoHidd_Input_IrqHandlerData
            event - A pointer to a read-only event descriptor structure with the following
                    contents:
                button - button code, or vHidd_Mouse_NoButton of the event describes a simple
                         motion.
                x, y   - event coordinates. Need to be always valid, even if the event describes
                         a button pressed without actual motion.
                         In case of mouse wheel event these fields specify horizontal and vertical
                         wheel delta respectively.
                type   - type of event (button press, button release, wheel or motion).
                flags  - event flags. Currently only one value of vHidd_Mouse_Relative is defined.
                         If this flag is not set, coordinates are assumed to be absolute.
                         This member is actually present in the structure only if the driver
                         supplies TRUE value for aoHidd_Mouse_Extended attribute.

        The handler is called inside interrupts, so usual restrictions apply to it.


    SEE ALSO
        aoHidd_Input_IrqHandler, aoHidd_Input_IrqHandlerData

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Mouse_State

    SYNOPSIS
        [..G], struct pHidd_Mouse_Event

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Obtains current pointing devices state.

        This attribute was historically implemented only in PS/2 mouse driver, but the
        implementation was broken and incomplete. At the moment this attribute is considered
        reserved. Do not use it, the specification may change in future.

    NOTES

    EXAMPLE

    BUGS
        Not implemented, considered reserved.

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Mouse_RelativeCoords

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Asks the driver it the device provides relative (like mouse) or absolute (like
        touchscreen or tabled) coordinates.

        Drivers which provide extended event structure may not implement this attribute
        because they may provide mixed set of events. In this case coordinates type
        is determined by flags member of struct pHidd_Mouse_ExtEvent.

        CLID_Hidd_Mouse class does not implement this attribute since it provides mixed
        stream of events.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Mouse_Extended

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Mouse_Extended

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Asks the driver if it provides extended event descriptor structure
        (struct pHidd_Mouse_ExtEvent).

        If value of this attribute is FALSE, the flags member is actually missing from
        the structure, not just zeroed out! So do not use it at all in this case.

        CLID_Hidd_Mouse class always return TRUE for this attribute.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************************/

OOP_Object *Mouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mouseNewMsg;
    struct TagItem mouseTags[] =
    {
        {aHidd_Input_Subsystem, (IPTR)CSD(cl)->hwObject },
        {TAG_MORE,              (IPTR)msg->attrList     },
        {TAG_DONE                                       }
    };
    mouseNewMsg.mID = msg->mID;
    mouseNewMsg.attrList = mouseTags;

    D(bug("[MouseHidd] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mouseNewMsg);

    D(bug("[MouseHidd] %s: returning 0x%p\n", __func__, o));

    return o;
}

VOID Mouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct MouseInput_Data *data = OOP_INST_DATA(cl, o);

    D(bug("[MouseHidd] %s()\n", __func__));
    
    OOP_DoSuperMethod(cl, o, msg);
}

VOID Mouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
/*          case aoHidd_Mouse_State:

                TODO: Implement this, by ORing buttons from all registered mice (?)

                return;*/

            case aoHidd_Mouse_Extended:
                *msg->storage = TRUE;
                return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
