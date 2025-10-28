/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "input.h"

/*****************************************************************************************

    NAME
        --background_inputclass--

    LOCATION
        CLID_Hidd_Input

    NOTES
        This class is the abstract base for all input subsystem classes.
        It provides the common interface and event distribution mechanism
        used by subsystem-specific input classes (e.g., keyboard, mouse, 
        controller). 

        The Input class itself is not intended for direct use. Instead, 
        you create an object of a subsystem-specific subclass and supply
        a callback via the aoHidd_Input_IrqHandler attribute. Your 
        callback will then be invoked whenever an event arrives until 
        you dispose of the object.

        All clients receive events from all subsystems input devices merged 
        into a single stream.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_Input_IrqHandler

    SYNOPSIS
        [I..], APTR

    LOCATION
        CLID_Hidd_Input

    FUNCTION
        Specifies an input event handler callback. The handler will be called every time
        an input event occurs.

        This attribute is used both by input event consumers (e.g., applications or higher-
        level input clients) and input event providers (e.g., device drivers or subsystems)
        to pass the necessary hooks for interacting with the input subsystem.

        Handlers should be declared using 'C' calling conventions, for example:

            void DeviceInputIRQ(APTR data, InputIrqData_t inputData)

        Handler parameters are:
            data      - The handler will be called with this set to the value
                        defined using the aoHidd_Input_IrqHandlerData attribute.
            inputData - This is an opaque pointer containing subsystem-specific 
                        input data. Pointing devices will use struct pHidd_Mouse_Event,
                        while keyboards will provide struct pHidd_Kbd_Event. See the
                        subsystem-specific documentation for more details.

        The handler is called inside interrupt context, so normal interrupt restrictions
        apply (e.g., avoid blocking operations).

    NOTES
        Every registered handler receives the merged event stream from all input devices.

    EXAMPLE

    BUGS
        Not all hosted drivers provide this attribute.

    SEE ALSO
        aoHidd_Input_IrqHandlerData

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Input_IrqHandlerData

    SYNOPSIS
        [I..], APTR

    LOCATION
        CLID_Hidd_Input

    FUNCTION
        Specifies a user-defined value that will be passed to the IRQ handler as its
        first parameter. This allows both input consumers and providers to associate
        static context or private data with the callback. The system will not assume
        anything about this value.

        Defaults to NULL if not specified.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Input_IrqHandler

    INTERNALS

******************************************************************************************/

OOP_Object *Input__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library          *UtilityBase = __ICSD(cl)->icsd_UtilityBase;
    struct InputHWInstData  *data;
    struct TagItem          *tag, *tstate;
    APTR                    *inputConsumers = NULL;

    D(bug("[Input:Hidd] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o)
        return NULL;

    data = OOP_INST_DATA(cl, o);
    data->ihid_callback = NULL;
    data->ihid_private = NULL;

    tstate = msg->attrList;
    D(bug("[Input:Hidd] %s: tstate = 0x%p\n", __func__, tstate));

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        if (IS_HIDDINPUT_ATTR(tag->ti_Tag, idx)) {
            switch (idx) {
                case aoHidd_Input_Subsystem:
                    data->ihid_hwObj = (OOP_Object *)tag->ti_Data;
                    break;

                case aoHidd_Input_IrqHandler:
                    data->ihid_callback = (APTR)tag->ti_Data;
                    break;

                case aoHidd_Input_IrqHandlerData:
                    data->ihid_private = (APTR)tag->ti_Data;
                    break;
            }
        }
        else if (IS_HWINPUT_ATTR(tag->ti_Tag, idx)) {
            switch (idx) {
                case aoHW_Input_ConsumerList:
                    inputConsumers = (APTR)tag->ti_Data;
                    break;
            }
        }
    } /* while (tags to process) */

    if (inputConsumers) {
        D(bug("[Input:Hidd] %s: Input event generator\n", __func__);)
        if (!data->ihid_private)
            data->ihid_private = inputConsumers;
        D(
            bug("[Input:Hidd] %s:     for Input HW subsystem @ 0x%p\n", __func__, data->ihid_hwObj);
            bug("[Input:Hidd] %s:     Callback @ 0x%p\n", __func__, data->ihid_callback);
            bug("[Input:Hidd] %s:     Callback Private = %p\n", __func__, data->ihid_private);
        )
        Disable();
        ADDTAIL(&__ICSD(cl)->icsd_producers, &data->ihid_node);
        Enable();
    } else {
        D(bug("[Input:Hidd] %s: Input event consumer\n", __func__);)
        if (data->ihid_hwObj) {
            if (data->ihid_callback) {
                struct Library *OOPBase = __ICSD(cl)->icsd_OOPBase;
                struct List *cbList = NULL;
                D(
                    bug("[Input:Hidd] %s:     for Input HW subsystem @ 0x%p\n", __func__, data->ihid_hwObj);
                    bug("[Input:Hidd] %s:     Callback @ 0x%p\n", __func__, data->ihid_callback);
                    bug("[Input:Hidd] %s:     Callback Private = %p\n", __func__, data->ihid_private);
                )
                OOP_GetAttr(data->ihid_hwObj, aHW_Input_ConsumerList, (IPTR *)&cbList);
                if (cbList) {
                    Disable();
                    ADDTAIL(cbList, &data->ihid_node);
                    Enable();
                }
            }
        }
    }
    return o;
}

VOID Input__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct InputHWInstData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[Input:Hidd] %s()\n", __func__));

    if (IS_HIDDINPUT_ATTR(msg->attrID, idx)) {
        switch (idx) {
            case aoHidd_Input_Subsystem:
                *msg->storage = (IPTR)data->ihid_hwObj;
                break;

            case aoHidd_Input_IrqHandlerData:
                *msg->storage = (IPTR)data->ihid_private;
                break;

            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
                break;
        }
    } else {
        OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    }

    ReturnVoid("HIDD::Get");
}

VOID Input__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct InputHWInstData *data = OOP_INST_DATA(cl, o);

    D(bug("[Input:Hidd] %s(0x%p, 0x%p, 0x%p)\n", __func__, cl, o, msg));

    if (data->ihid_hwObj && data->ihid_callback) {
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
        moHidd_Input_AddHardwareDriver

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Input_AddHardwareDriver *Msg);

        OOP_Object *HIDD_Input_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass,
                                               struct TagItem *tags);

    LOCATION
        CLID_Hidd_Input

    FUNCTION
        Creates a hardware driver object and registers it in the system.

        It does not matter on which instance of CLID_Hidd_Input class this method is
        used. Hardware driver objects are shared between all of them.

        Since V2 this interface is obsolete and deprecated. Use moHW_AddDriver
        method on CLID_HW_Input class in order to install the driver.

    INPUTS
        obj         - Any object of CLID_Hidd_Input class.
        driverClass - A pointer to OOP class of the driver. In order to create an object
                      of some previously registered public class, use
                      oop.library/OOP_FindClass().
        tags        - An optional taglist which will be passed to driver class' New() method.

    RESULT
        A pointer to driver object.

    NOTES
        Do not dispose the returned object yourself, use HIDD_Input_RemHardwareDriver() for it.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Input_RemHardwareDriver

    INTERNALS
        This method supplies own interrupt handler to the driver, do not override this.

*****************************************************************************************/

OOP_Object *Input__Hidd_Input__AddHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Input_AddHardwareDriver *Msg)
{
    struct InputHWInstData *data = OOP_INST_DATA(cl, o);

    D(bug("[Input:Hidd] %s()\n", __func__));

    return HW_AddDriver(data->ihid_hwObj, Msg->driverClass, Msg->tags);
}

/*****************************************************************************************

    NAME
        moHidd_Input_RemHardwareDriver

    SYNOPSIS
        void OOP_DoMethod(OOP_Object *obj, struct pHidd_Input_RemHardwareDriver *Msg);

        void HIDD_Input_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver);

    LOCATION
        CLID_Hidd_Input

    FUNCTION
        Unregisters and disposes input hardware driver object.

        It does not matter on which instance of CLID_Hidd_Input class this method is
        used. Hardware driver objects are shared between all of them.

        Since V2 this interface is obsolete and deprecated. Use moHW_RemoveDriver
        method on CLID_HW_Input class in order to remove the driver.

    INPUTS
        obj    - Any object of CLID_Hidd_Input class.
        driver - A pointer to a driver object, returned by HIDD_Input_AddHardwareDriver().

    RESULT
        None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Input_AddHardwareDriver

    INTERNALS

*****************************************************************************************/

void Input__Hidd_Input__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Input_RemHardwareDriver *Msg)
{
    struct InputHWInstData *data = OOP_INST_DATA(cl, o);

    D(bug("[Input:Hidd] %s()\n", __func__));

    HW_RemoveDriver(data->ihid_hwObj, Msg->driverObject);
}
