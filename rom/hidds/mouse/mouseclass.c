/*
    Copyright (C) 2004-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "mouse.h"

#define SysBase     (CSD(cl)->cs_SysBase)
#define UtilityBase (CSD(cl)->cs_UtilityBase)

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_Hidd_Mouse

    NOTES
        Instances of this class are virtual devices being clients of the
        pointing input subsystem. In order to receive input events, you
        have to create an object of this class and supply a callback using
        aoHidd_Mouse_IrqHandler attribute. After this your callback will be
        called every time the event arrives until you dispose your object.

        Every client receives events from all pointing devices merged into
        a single stream.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Mouse_IrqHandler

    SYNOPSIS
        [I..], APTR

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Specifies a pointing device interrupt handler. The handler will called be every time a
        keyboard event happens. A "C" calling convention is used, declare the handler
        functions as follows:

        void MouseIRQ(APTR data, struct pHidd_Mouse_Event *event);

        Handler parameters are:
            data  - Anything you specify using aoHidd_Mouse_IrqHandlerData
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

    NOTES
        CLID_Hidd_Mouse class always provides extended form of event structure
        (struct pHidd_Mouse_ExtEvent). Drivers will not always provide it, depending
        on their aoHidd_Mouse_Extended attribute value.

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Mouse_IrqHandlerData, aoHidd_Mouse_Extended

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Mouse_IrqHandlerData

    SYNOPSIS
        [I..], APTR

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Specifies a user-defined value that will be passed to interrupt handler as a first
        parameter. The purpose of this is to pass some static data to the handler.
        The system will not assume anything about this value.

        Defaults to NULL if not specified.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Mouse_IrqHandler

    INTERNALS

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
        aoHidd_Mouse_IrqHandler, aoHidd_Mouse_Extended

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
        aoHidd_Mouse_IrqHandler

    INTERNALS

******************************************************************************************/

OOP_Object *Mouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct mouse_data *data;
    struct TagItem *tag, *tstate;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o)
        return NULL;

    data = OOP_INST_DATA(cl, o);
    data->callback = NULL;
    data->callbackdata = NULL;

    tstate = msg->attrList;
    D(bug("tstate: %p\n", tstate));

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        D(bug("Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));

        if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
        {
            D(bug("Mouse hidd tag\n"));
            switch (idx)
            {
                case aoHidd_Mouse_IrqHandler:
                    data->callback = (APTR)tag->ti_Data;
                    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
                    break;

                case aoHidd_Mouse_IrqHandlerData:
                    data->callbackdata = (APTR)tag->ti_Data;
                    D(bug("Got data %p\n", (APTR)tag->ti_Data));
                    break;
            }
        }
    } /* while (tags to process) */

    /* Add to interrupts list if we have a callback */
    if (data->callback)
    {
        Disable();
        ADDTAIL(&CSD(cl)->callbacks, data);
        Enable();
    }

    return o;
}

VOID Mouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);

    if (data->callback)
    {
        Disable();
        REMOVE((struct Node *)data);
        Enable();
    }
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

/*
 * In future we could support enumeration of devices and specifying
 * which device we wish to read events from (in case if we want to implement
 * amigainput.library or something like it)
 */

/*****************************************************************************************

    NAME
        moHidd_Mouse_AddHardwareDriver

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Mouse_AddHardwareDriver *Msg);

        OOP_Object *HIDD_Mouse_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Creates a hardware driver object and registers it in the system.

        It does not matter on which instance of CLID_Hidd_Mouse class this method is
        used. Hardware driver objects are shared between all of them.

        Since V2 this interface is obsolete and deprecated. Use moHW_AddDriver
        method on CLID_HW_Mouse class in order to install the driver.

    INPUTS
        obj         - Any object of CLID_Hidd_Mouse class.
        driverClass - A pointer to OOP class of the driver. In order to create an object
                      of some previously registered public class, use
                      oop.library/OOP_FindClass().
        tags        - An optional taglist which will be passed to driver class' New() method.

    RESULT
        A pointer to driver object.

    NOTES
        Do not dispose the returned object yourself, use HIDD_Mouse_RemHardwareDriver() for it.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Mouse_RemHardwareDriver

    INTERNALS
        This method supplies own interrupt handler to the driver, do not override this.

*****************************************************************************************/

OOP_Object *Mouse__Hidd_Mouse__AddHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_AddHardwareDriver *Msg)
{
    return HW_AddDriver(CSD(cl)->hwObject, Msg->driverClass, Msg->tags);
}

/*****************************************************************************************

    NAME
        moHidd_Mouse_RemHardwareDriver

    SYNOPSIS
        void OOP_DoMethod(OOP_Object *obj, struct pHidd_Mouse_RemHardwareDriver *Msg);

        void HIDD_Mouse_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver);

    LOCATION
        CLID_Hidd_Mouse

    FUNCTION
        Unregisters and disposes pointing device hardware driver object.

        It does not matter on which instance of CLID_Hidd_Mouse class this method is
        used. Hardware driver objects are shared between all of them.

        Since V2 this interface is obsolete and deprecated. Use moHW_RemoveDriver
        method on CLID_HW_Kbd class in order to remove the driver.

    INPUTS
        obj    - Any object of CLID_Hidd_Mouse class.
        driver - A pointer to a driver object, returned by HIDD_Mouse_AddHardwareDriver().

    RESULT
        None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Mouse_AddHardwareDriver

    INTERNALS

*****************************************************************************************/

void Mouse__Hidd_Mouse__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_RemHardwareDriver *Msg)
{
    HW_RemoveDriver(CSD(cl)->hwObject, Msg->driverObject);
}
