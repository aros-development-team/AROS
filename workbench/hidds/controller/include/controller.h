#ifndef HIDD_CONTROLLER_H
#define HIDD_CONTROLLER_H

/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.
    $Id$

    Desc: Include for the controller hidd.
    Lang: English.
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef PROTO_OOP_H
#   include <proto/oop.h>
#endif

#define CLID_Hidd_Controller "hidd.input.controller"
#define CLID_HW_Controller   "hw.input.controller"

#define IID_Hidd_Controller "hidd.input.controller"

#define HiddControllerAB __abHidd_Controller

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddControllerAB;
#endif

/*****************************************************************************************

    NAME
        Controller class

    LOCATION
        CLID_Hidd_Controller

    NOTES
        Instances of this class represent game controllers, joysticks, gamepads,
        wheels and similar devices. To receive controller input events you need to
        create an object of this class and supply a callback using
        aoHidd_Input_IrqHandler attribute. The callback will be invoked whenever
        controller input changes. Each event contains both the specific change
        (button/axis/hat) and a full snapshot of the device state.

*****************************************************************************************/

/* Attributes */
enum {
    aoHidd_Controller_State,          /* [..G], struct pHidd_Controller_State* */
    aoHidd_Controller_RelativeCoords, /* [..G], BOOL */
    aoHidd_Controller_Extended,       /* [..G], BOOL */
    aoHidd_Controller_ButtonCount,    /* [..G], UWORD */
    aoHidd_Controller_AxisCount,      /* [..G], UWORD */
    aoHidd_Controller_HatCount,       /* [..G], UWORD */
    aoHidd_Controller_ForceFeedback,  /* [..G], BOOL */
    aoHidd_Controller_Capabilities,   /* [..G], ULONG (bitmask, see below) */

    num_Hidd_Controller_Attrs
};

#define aHidd_Controller_State          (aoHidd_Controller_State          + HiddControllerAB)
#define aHidd_Controller_RelativeCoords (aoHidd_Controller_RelativeCoords + HiddControllerAB)
#define aHidd_Controller_Extended       (aoHidd_Controller_Extended       + HiddControllerAB)
#define aHidd_Controller_ButtonCount    (aoHidd_Controller_ButtonCount    + HiddControllerAB)
#define aHidd_Controller_AxisCount      (aoHidd_Controller_AxisCount      + HiddControllerAB)
#define aHidd_Controller_HatCount       (aoHidd_Controller_HatCount       + HiddControllerAB)
#define aHidd_Controller_ForceFeedback  (aoHidd_Controller_ForceFeedback  + HiddControllerAB)
#define aHidd_Controller_Capabilities   (aoHidd_Controller_Capabilities   + HiddControllerAB)

#define IS_HIDDCONTROLLER_ATTR(attr, idx) \
    IS_IF_ATTR(attr, idx, HiddControllerAB, num_Hidd_Controller_Attrs)

/* Capability bits for aoHidd_Controller_Capabilities */
#define vHidd_Controller_Cap_ButtonPressure  (1 << 0)
#define vHidd_Controller_Cap_AnalogAxes      (1 << 1)
#define vHidd_Controller_Cap_Hats            (1 << 2)
#define vHidd_Controller_Cap_TriggersAsAxes  (1 << 3)
#define vHidd_Controller_Cap_ForceFeedback   (1 << 4)
#define vHidd_Controller_Cap_Rumble          (1 << 5)

/* Event types */
enum {
    vHidd_Controller_Press,        /* button pressed */
    vHidd_Controller_Release,      /* button released */
    vHidd_Controller_AxisMotion,   /* axis moved */
    vHidd_Controller_HatMotion,    /* hat changed */
    vHidd_Controller_TriggerMotion /* trigger analog motion */
};

/* Common button identifiers (drivers may extend >= 0x100) */
enum {
    vHidd_Controller_NoButton = 0,
    vHidd_Controller_ButtonA,
    vHidd_Controller_ButtonB,
    vHidd_Controller_ButtonX,
    vHidd_Controller_ButtonY,
    vHidd_Controller_ButtonLB,
    vHidd_Controller_ButtonRB,
    vHidd_Controller_ButtonLStick,
    vHidd_Controller_ButtonRStick,
    vHidd_Controller_ButtonBack,
    vHidd_Controller_ButtonStart,
    vHidd_Controller_ButtonHome
};

/* Axis identifiers (drivers may extend by index) */
enum {
    vHidd_Controller_Axis_X = 0,
    vHidd_Controller_Axis_Y,
    vHidd_Controller_Axis_Z,
    vHidd_Controller_Axis_RX,
    vHidd_Controller_Axis_RY,
    vHidd_Controller_Axis_RZ,
    vHidd_Controller_Axis_Slider0,
    vHidd_Controller_Axis_Slider1
};

/* Hat directions (bitmask — multiple bits may be set for diagonals) */
#define vHidd_Controller_Hat_Up     (1 << 0)
#define vHidd_Controller_Hat_Right  (1 << 1)
#define vHidd_Controller_Hat_Down   (1 << 2)
#define vHidd_Controller_Hat_Left   (1 << 3)
#define vHidd_Controller_Hat_Center 0

/* Event flags */
#define vHidd_Controller_Flag_Relative 0x0001
#define vHidd_Controller_Flag_Analog   0x0002

/*****************************************************************************************

    NAME
        pHidd_Controller_State

    FUNCTION
        Represents the current complete state of a controller.

*****************************************************************************************/
struct pHidd_Controller_State
{
    UWORD button_count;
    ULONG *buttons_mask;    /* array of ULONGs for button bitmask */
    UWORD axis_count;
    WORD *axis_values;      /* signed 16-bit axis values */
    UWORD hat_count;
    UWORD *hat_values;      /* hat directions bitmask */
};

/*****************************************************************************************

    NAME
        pHidd_Controller_Event

    FUNCTION
        Event descriptor passed to the client IRQ handler. Contains the type of change,
        code identifying which control changed, and a snapshot of the entire controller
        state. This structure is always passed unless aoHidd_Controller_Extended is TRUE,
        in which case the extended form is passed instead.

*****************************************************************************************/
struct pHidd_Controller_Event
{
    UWORD type;         /* one of vHidd_Controller_* event types */
    UWORD code;         /* button/axis/hat index depending on type */
    UWORD flags;        /* vHidd_Controller_Flag_* */
    UWORD reserved;

    struct pHidd_Controller_State state;
};

/*****************************************************************************************

    NAME
        pHidd_Controller_ExtEvent

    FUNCTION
        Extended event descriptor. Used only when aoHidd_Controller_Extended == TRUE.
        Contains the base event structure plus timestamp and device instance id.

*****************************************************************************************/
struct pHidd_Controller_ExtEvent
{
    struct pHidd_Controller_Event ev;
    ULONG timestamp;    /* driver/system ticks */
    UWORD device_id;    /* instance id */
    UWORD padding;
};

/*****************************************************************************************

    NAME
        ControllerIRQ

    SYNOPSIS
        void ControllerIRQ(APTR data, struct pHidd_Controller_Event *event);

    FUNCTION
        Prototype for the interrupt handler specified with aoHidd_Input_IrqHandler.
        If aoHidd_Controller_Extended == TRUE, the pointer passed will actually
        reference a struct pHidd_Controller_ExtEvent.

*****************************************************************************************/

#if !defined(HiddControllerBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddControllerBase HIDD_Controller_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_Controller_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID ControllerMethodBase;

    if (!ControllerMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OOPBASE(obj);
        ControllerMethodBase = OOP_GetMethodID(IID_Hidd_Controller, 0);
    }

    return ControllerMethodBase;
}
#endif

#endif /* HIDD_CONTROLLER_H */
