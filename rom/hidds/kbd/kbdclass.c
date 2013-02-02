/*
    Copyright (C) 2004-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "kbd.h"

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_Hidd_Kbd

    NOTES
        Instances of this class are virtual devices being clients of the
        keyboard input subsystem. In order to receive keyboard events, you
        have to create an object of this class and supply a callback using
        aoHidd_Kbd_IrqHandler attribute. After this your callback will be
        called every time the event arrives until you dispose your object.

        Every client receives events from all keyboard devices merged into
        a single stream.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Kbd_IrqHandler

    SYNOPSIS
        [I..], APTR

    LOCATION
        CLID_Hidd_Kbd

    FUNCTION
        Specifies a keyboard event handler. The handler will called be every time a
        keyboard event happens. A "C" calling convention is used, declare the handler
        functions as follows:

        void KeyboardIRQ(APTR data, UWORD keyCode)

        Handler parameters are:
            data    - Anything you specify using aoHidd_Kbd_IrqHandlerData
            keyCode - A raw key code as specified in devices/rawkeycodes.h.
                      Key release event is indicated by ORing this value
                      with IECODE_UP_PREFIX (defined in devices/inputevent.h)

        The handler is called inside interrupts, so usual restrictions apply to it.

    NOTES

    EXAMPLE

    BUGS
        Not all hosted drivers provide this attribute.

    SEE ALSO
        aoHidd_Kbd_IrqHandlerData

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Kbd_IrqHandlerData

    SYNOPSIS
        [I..], APTR

    LOCATION
        CLID_Hidd_Kbd

    FUNCTION
        Specifies a user-defined value that will be passed to IRQ handler as a first
        parameter. The purpose of this is to pass some static data to the handler.
        The system will not assume anything about this value.

        Defaults to NULL if not specified.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Kbd_IrqHandler

    INTERNALS

******************************************************************************************/

OOP_Object *KBD__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct kbd_data *data;
    struct TagItem *tag, *tstate;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

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

        if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
        {
            D(bug("Kbd hidd tag\n"));
            switch (idx)
            {
                case aoHidd_Kbd_IrqHandler:
                    data->callback = (APTR)tag->ti_Data;
                    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
                    break;

                case aoHidd_Kbd_IrqHandlerData:
                    data->callbackdata = (APTR)tag->ti_Data;
                    D(bug("Got data %p\n", (APTR)tag->ti_Data));
                    break;
            }
        }
    } /* while (tags to process) */

    /*
     * Add to interrupts list if we have a callback.
     * Creating an object without callback is a legacy way of
     * installing hardware drivers. It should be removed.
     */
    if (data->callback)
    {
        Disable();
        ADDTAIL(&CSD(cl)->callbacks, data);
        Enable();
    }

    return o;
}

VOID KBD__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);

    /* This condition is a legacy and should be removed */
    if (data->callback)
    {
        Disable();
        REMOVE(data);
        Enable();
    }
    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * The following two methods are legacy and strongly deprecated.
 * They are present only for backward compatibility with the old code.
 */

/*****************************************************************************************

    NAME
        moHidd_Kbd_AddHardwareDriver

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Kbd_AddHardwareDriver *Msg);

        OOP_Object *HIDD_Kbd_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass,
                                               struct TagItem *tags);

    LOCATION
        CLID_Hidd_Kbd

    FUNCTION
        Creates a hardware driver object and registers it in the system.

        It does not matter on which instance of CLID_Hidd_Kbd class this method is
        used. Hardware driver objects are shared between all of them.

        Since V2 this interface is obsolete and deprecated. Use moHW_AddDriver
        method on CLID_HW_Kbd class in order to install the driver.

    INPUTS
        obj         - Any object of CLID_Hidd_Kbd class.
        driverClass - A pointer to OOP class of the driver. In order to create an object
                      of some previously registered public class, use
                      oop.library/OOP_FindClass().
        tags        - An optional taglist which will be passed to driver class' New() method.

    RESULT
        A pointer to driver object.

    NOTES
        Do not dispose the returned object yourself, use HIDD_Kbd_RemHardwareDriver() for it.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Kbd_RemHardwareDriver

    INTERNALS
        This method supplies own interrupt handler to the driver, do not override this.

*****************************************************************************************/

OOP_Object *KBD__Hidd_Kbd__AddHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_AddHardwareDriver *Msg)
{
    return HW_AddDriver(CSD(cl)->hwObj, Msg->driverClass, Msg->tags);
}

/*****************************************************************************************

    NAME
        moHidd_Kbd_RemHardwareDriver

    SYNOPSIS
        void OOP_DoMethod(OOP_Object *obj, struct pHidd_Kbd_RemHardwareDriver *Msg);

        void HIDD_Kbd_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver);

    LOCATION
        CLID_Hidd_Kbd

    FUNCTION
        Unregisters and disposes keyboard hardware driver object.

        It does not matter on which instance of CLID_Hidd_Kbd class this method is
        used. Hardware driver objects are shared between all of them.

        Since V2 this interface is obsolete and deprecated. Use moHW_RemoveDriver
        method on CLID_HW_Kbd class in order to remove the driver.

    INPUTS
        obj    - Any object of CLID_Hidd_Kbd class.
        driver - A pointer to a driver object, returned by HIDD_Kbd_AddHardwareDriver().

    RESULT
        None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Kbd_AddHardwareDriver

    INTERNALS

*****************************************************************************************/

void KBD__Hidd_Kbd__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_RemHardwareDriver *Msg)
{
    HW_RemoveDriver(CSD(cl)->hwObj, Msg->driverObject);
}
