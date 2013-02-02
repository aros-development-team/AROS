/*
    Copyright (C) 2004-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

#undef HiddMouseAB
#define HiddMouseAB             (CSD(cl)->hiddMouseAB)

#include <hidd/mouse.h>

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_Hidd_Mouse

    NOTES
        This class represents a "hub" for collecting input from various
        pointing devices (mice, tablets, touchscreens, etc) in the
        system and sending them to clients.

        In order to get an access to pointing input subsystem you need to
        create an object of CLID_Hidd_Mouse class. There can be two use
        scenarios: driver mode and client mode.

        If you wish to run in client mode (receive pointing events), you
        have to supply a callback using aoHidd_Mouse_IrqHandler attribute.
        After this your callback will be called every time the event arrives
        until you dispose your object.

        Events from all pointing devices are merged into a single stream
        and propagated to all clients.

        In driver mode you don't need to supply a callback (however it's not
        forbidden). Instead you use the master object for registering your
        hardware driver using HIDD_Mouse_AddHardwareDriver(). It is safe to
        dispose the master object after adding a driver, the driver will
        be internally kept in place.
*****************************************************************************************/

/*****************************************************************************************

    NAME
        --hardware_drivers--

    LOCATION
        CLID_Hidd_Mouse

    NOTES
        A hardware driver should implement the same interface according to the following
        rules:

        1. A single object of driver class represents a single hardware unit.
        2. A single driver object maintains a single callback address (passed to it
           using aoHidd_Mouse_IrqHandler). Under normal conditions this callback is supplied
           by CLID_Hidd_Mouse class.
        3. HIDD_Mouse_AddHardwareDriver() and HIDD_Mouse_RemHardwareDriver() on a driver object
           itself do not make sense, so there's no need to implement them.

        A hardware driver class should be a subclass of CLID_Hidd in order to ensure
        compatibility in future.

*****************************************************************************************/


static void GlobalCallback(struct driverNode *drv, struct pHidd_Mouse_ExtEvent *ev)
{
    struct pHidd_Mouse_ExtEvent xev;
    struct mouse_data *data;

/* The event passed in may be pHidd_Mouse_Event instead of pHidd_Mouse_ExtEvent,
   according to flags. In this case we add own flags */
    if (drv->flags != vHidd_Mouse_Extended) {
        xev.button = ev->button;
        xev.x      = ev->x;
        xev.y      = ev->y;
        xev.type   = ev->type;
        xev.flags  = drv->flags;
        
        ev = &xev;
    }

    for (data = (struct mouse_data *)drv->callbacks->mlh_Head; data->node.mln_Succ;
         data = (struct mouse_data *)data->node.mln_Succ)
        data->callback(data->callbackdata, ev);
}

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
        CLID_Hidd_Mouse and some hardware driver classes allow to get value of this attribute,
        however there is currently no use for it. The attribute is considered non-getable.

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
        CLID_Hidd_Mouse and some hardware driver classes allow to get value of this attribute,
        however there is currently no use for it. The attribute is considered non-getable.

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
    struct Library *UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);

    if (!UtilityBase)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o) {
        CloseLibrary(UtilityBase);
        return NULL;
    }

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
    if (data->callback) {
        Disable();
        AddTail((struct List *)&CSD(cl)->callbacks, (struct Node *)data);
        Enable();
    }

    CloseLibrary(UtilityBase);

    return o;
}

VOID Mouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);

    if (data->callback) {
        Disable();
        Remove((struct Node *)data);
        Enable();
    }
    OOP_DoSuperMethod(cl, o, msg);
}

VOID Mouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    ULONG              idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Mouse_IrqHandler:
                *msg->storage = (IPTR)data->callback;
                return;

            case aoHidd_Mouse_IrqHandlerData:
                *msg->storage = (IPTR)data->callbackdata;
                return;

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
    struct TagItem tags[] = {
        { aHidd_Mouse_IrqHandler    , (IPTR)GlobalCallback},
        { aHidd_Mouse_IrqHandlerData, 0                   },
        { TAG_MORE                  , (IPTR)Msg->tags     }
    };
    struct driverNode *drvnode;

    D(bug("[Mouse] AddHardwareDriver(0x%p)\n", Msg->driverClass));

    drvnode = AllocMem(sizeof(struct driverNode), MEMF_PUBLIC);  
    if (!drvnode)
        return NULL;

    tags[1].ti_Data = (IPTR)drvnode;
    drvnode->drv = OOP_NewObject(Msg->driverClass, NULL, tags);
    D(bug("[Mouse] Driver node 0x%p, driver 0x%p\n", drvnode, drvnode->drv));

    if (drvnode->drv) {
        struct mouse_staticdata *csd = CSD(cl);
        IPTR val = FALSE;

        drvnode->callbacks = &csd->callbacks;

        OOP_GetAttr(drvnode->drv, aHidd_Mouse_Extended, &val);
        D(bug("[Mouse] Extended event: %d\n", val));
        if (val)
            drvnode->flags = vHidd_Mouse_Extended;
        else
        {
            OOP_GetAttr(drvnode->drv, aHidd_Mouse_RelativeCoords, &val);
            D(bug("[Mouse] Relative coordinates: %d\n", val));
            drvnode->flags = val ? vHidd_Mouse_Relative : 0;
        }

        ObtainSemaphore(&csd->drivers_sem);
        AddTail((struct List *)&csd->drivers, (struct Node *)drvnode);
        ReleaseSemaphore(&csd->drivers_sem);

        return drvnode->drv;
    }

    FreeMem(drvnode, sizeof(struct driverNode));
    return NULL;
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
    struct mouse_staticdata *csd = CSD(cl);
    struct driverNode *node;

    ObtainSemaphore(&csd->drivers_sem);

    for (node = (struct driverNode *)csd->drivers.mlh_Head; node->node.mln_Succ;
         node = (struct driverNode *)node->node.mln_Succ) {
        if (node->drv == Msg->driverObject) {
            Remove((struct Node *)node);
            break;
        }
    }

    ReleaseSemaphore(&csd->drivers_sem);

    OOP_DisposeObject(Msg->driverObject);

    if (node->drv == Msg->driverObject)
        FreeMem(node, sizeof(struct driverNode));
}

/* Class initialization and destruction */
#undef  SysBase
#define SysBase LIBBASE->csd.cs_SysBase
#undef  OOPBase
#define OOPBase LIBBASE->csd.cs_OOPBase

static int Mouse_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Mouse] Base Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_Mouse);

    return TRUE;
}

static int Mouse_InitClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Mouse] base class initialization\n"));

    LIBBASE->csd.hiddMouseAB = OOP_ObtainAttrBase(IID_Hidd_Mouse);

    if (LIBBASE->csd.hiddMouseAB)
    {
        NewList((struct List *)&LIBBASE->csd.callbacks);
        NewList((struct List *)&LIBBASE->csd.drivers);
        InitSemaphore(&LIBBASE->csd.drivers_sem);
        D(bug("[Mouse] Everything OK\n"));
        return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(Mouse_InitClass, 0)
ADD2EXPUNGELIB(Mouse_ExpungeClass, 0)
