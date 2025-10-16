/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "controller.h"

#define SysBase     ((struct ExecBase *)(CSD(cl)->cs_SysBase))
#define UtilityBase (CSD(cl)->cs_UtilityBase)

/*****************************************************************************************

    NAME
        Controller class

    LOCATION
        CLID_Hidd_Controller

    NOTES
        Instances of this class are virtual devices representing game controllers,
        joysticks, pads, wheels, or other generic input controllers. To receive
        controller input events, create an object of this class and supply a callback
        using aoHidd_Input_IrqHandler. After that, your callback will be called
        whenever controller input is received until you dispose of the object.

        Every client receives events from all controllers merged into a single stream.
        If you need to distinguish between physical devices, check the device_id field
        of the extended event structure (struct pHidd_Controller_ExtEvent).

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Controller_State

    SYNOPSIS
        [..G], struct pHidd_Controller_State

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Obtains the current controller state. This is a snapshot structure containing
        button bitmasks, axis values, and hat values.

    NOTES
        Reading state directly is optional; normally you will track state using the
        event stream instead.

    SEE ALSO
        pHidd_Controller_Event, pHidd_Controller_ExtEvent

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Controller_RelativeCoords

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Queries whether the device reports relative (like a controller or steering wheel)
        or absolute (like a touchscreen or analog stick with fixed range) coordinates.

        Drivers which provide extended event structures may not implement this attribute
        if they generate a mixed set of events. In that case, the coordinate type must
        be determined from the flags member of struct pHidd_Controller_Event or
        pHidd_Controller_ExtEvent.

        CLID_Hidd_Controller class does not implement this attribute itself since it
        merges a mixed stream of events.

    SEE ALSO
        aoHidd_Input_IrqHandler, aoHidd_Controller_Extended

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Controller_Extended

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Queries whether the driver provides the extended event descriptor structure
        (struct pHidd_Controller_ExtEvent).

        If the value of this attribute is FALSE, the event passed to the handler is
        a struct pHidd_Controller_Event and does not contain timestamp or device_id.
        If TRUE, handlers receive a struct pHidd_Controller_ExtEvent instead.

        CLID_Hidd_Controller class always returns TRUE for this attribute.

    SEE ALSO
        aoHidd_Input_IrqHandler

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_Controller_ButtonCount

    SYNOPSIS
        [..G], ULONG

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Returns the number of buttons physically supported by this controller.  
        Button indices in events range from 0 to (ButtonCount - 1).

    NOTES
        Button codes are device-specific; higher-level code should not assume
        semantic meaning (e.g. "A button") from index values.

    SEE ALSO
        pHidd_Controller_State

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Controller_AxisCount

    SYNOPSIS
        [..G], ULONG

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Returns the number of analog axes exposed by this controller.  
        Axis indices in events range from 0 to (AxisCount - 1).

    NOTES
        Axis values are signed 16-bit integers, typically in the range -32768..32767.  
        The interpretation (X/Y stick, throttle, trigger, wheel) depends on the driver
        or higher-level mapping.

    SEE ALSO
        pHidd_Controller_State

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Controller_HatCount

    SYNOPSIS
        [..G], ULONG

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Returns the number of directional hat switches (POV hats) exposed by this
        controller.  
        Hat indices in events range from 0 to (HatCount - 1).

    NOTES
        Hat values are usually encoded as 0–7 for the eight cardinal/diagonal
        directions, with 0xFFFF meaning "centered".

    SEE ALSO
        pHidd_Controller_State

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Controller_Capabilities

    SYNOPSIS
        [..G], ULONG (bitmask)

    LOCATION
        CLID_Hidd_Controller

    FUNCTION
        Returns a bitmask describing what types of controls this device provides.  
        Capability flags may include:
            vHidd_Controller_Cap_Buttons  – one or more buttons are present  
            vHidd_Controller_Cap_Axes     – one or more analog axes are present  
            vHidd_Controller_Cap_Hats     – one or more hat switches are present  

    NOTES
        This attribute can be used to quickly filter devices before querying their
        detailed counts.

    SEE ALSO
        aoHidd_Controller_ButtonCount, aoHidd_Controller_AxisCount,
        aoHidd_Controller_HatCount

*****************************************************************************************/

OOP_Object *Controller__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New controllerNewMsg;
    struct TagItem controllerTags[] =
    {
        {aHidd_Input_Subsystem, (IPTR)CSD(cl)->hwObject },
        {TAG_MORE,              (IPTR)msg->attrList     },
        {TAG_DONE                                       }
    };
    controllerNewMsg.mID = msg->mID;
    controllerNewMsg.attrList = controllerTags;

    D(bug("[ControllerHidd] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&controllerNewMsg);

    D(bug("[ControllerHidd] %s: returning 0x%p\n", __func__, o));

    return o;
}

VOID Controller__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ControllerInput_Data *data = OOP_INST_DATA(cl, o);

    D(bug("[ControllerHidd] %s()\n", __func__));
    
    OOP_DoSuperMethod(cl, o, msg);
}

VOID Controller__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_HIDDCONTROLLER_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Controller_Extended:
                *msg->storage = TRUE;
                return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
