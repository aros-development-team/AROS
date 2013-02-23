/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include "mouse.h"

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	CLID_Hidd_Mouse

    NOTES
	This class represents a "hub" for collecting input from various
	pointing devices (mice, tablets, touchscreens, etc) in the
	system. Events from all pointing devices are merged into a
        single stream and propagated to all clients.

	In order to get an access to pointing input subsystem you need to
	create an object of CLID_Hidd_Mouse class. The actual returned
        object is a singletone, you do not have to dispose it, and every
        call will return the same object pointer. After getting this object
        you can, for example, register your driver using moHW_AddDriver
        method, or enumerate drivers using moHW_EnumDrivers.

        If you wish to receive keyboard events, use objects of CLID_Hidd_Mouse
        class. This class implements the same interface as driver class, but
        represents receiver's side and is responsible for registering user's
        interrupt handler in the listeners chain. These objects are not real
        drivers and do not need to me registered within the subsystem.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        --hardware_drivers--

    LOCATION
        CLID_HW_Mouse

    NOTES
        A hardware driver should be a subclass of CLID_Hidd and implement IID_Hidd_Mouse
        interface according to the following rules:

        1. A single object of driver class represents a single hardware unit.
        2. A single driver object maintains a single callback address (passed to it
           using aoHidd_Mouse_IrqHandler). Under normal conditions this callback is
           supplied by CLID_HW_Mouse class.

*****************************************************************************************/

OOP_Object *MouseHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct mouse_staticdata *csd = CSD(cl);

    if (!csd->hwObject)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"Pointing devices"},
            {TAG_DONE     , 0                       }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        csd->hwObject = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }
    return csd->hwObject;
}

VOID MouseHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{

}

OOP_Object *MouseHW__HW__AddDriver(OOP_Class *cl, OOP_Object *o, struct pHW_AddDriver *Msg)
{
    /*
     * Instantiate a proxy object instead of driver. The proxy object
     * will create driver itself.
     * Since driver object creation/disposition is handled by proxy,
     * we do not have to use Setup/Cleanup methods in this class.
     */
    struct TagItem tags[] =
    {
        {aHidd_DriverData_ClassPtr, (IPTR)Msg->driverClass},
        {TAG_MORE                 , (IPTR)Msg->tags       }
    };
    struct pHW_AddDriver add_msg =
    {
        .mID         = Msg->mID,
        .driverClass = CSD(cl)->dataClass,
        .tags        = tags
    };

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &add_msg.mID);
}

/*
 * For simplicity and speed our hooks access fields of DriverNode class
 * directly. OOP_Object * is directly cast to struct driver Node *.
 * This is perfectly OK becuase:
 * a) This is our private class, nobody is going to mess with it.
 * b) It does not have any base classes, and is derived directly from rootclass.
 * In C++ terminology it's our friend class.
 */
AROS_UFH3(static BOOL, searchFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(struct driverNode *, dn, A2),
    AROS_UFHA(OOP_Object *, wanted, A1))
{
    AROS_USERFUNC_INIT

    if (dn->drv == wanted)
    {
        h->h_Data = dn;
        return TRUE;
    }

    return FALSE;
    
    AROS_USERFUNC_EXIT
}

BOOL MouseHW__HW__RemoveDriver(OOP_Class *cl, OOP_Object *o, struct pHW_RemoveDriver *Msg)
{
    /*
     * We are given driver object, and now we need to look up
     * its proxy. We use moHW_EnumDrivers method for this purpose.
     * Our hook's h_Data will be filled in with proxy pointer.
     * This double traversing objects list is what i really dislike
     * in this design. An alternative would be to add some means
     * to extend struct DriverNode in hidd/hwclass.c, however this
     * would be also ugly, since it's private data.
     * Looks like we are not going to have this problem anywhere else.
     * Other subsystems (PCI, I2C) have base class for its drivers, and
     * the needed data can be simply encapsulated there.
     */

    struct Hook searchHook =
    {
        .h_Entry = (HOOKFUNC)searchFunc,
        .h_Data  = NULL
    };

    HW_EnumDrivers(o, &searchHook, Msg->driverObject);
    
    if (searchHook.h_Data)
    {
        struct pHW_RemoveDriver rem_msg =
        {
            .mID = Msg->mID,
            .driverObject = searchHook.h_Data
        };
        
        return OOP_DoSuperMethod(cl, o, &rem_msg.mID);
    }
    return FALSE;
}

AROS_UFH3(static void, enumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(struct driverNode *, dn, A2),
    AROS_UFHA(OOP_Object *, hookMsg, A1))
{
    AROS_USERFUNC_INIT

    struct Hook *user_hook = h->h_Data;

    D(bug("[Mouse] Enum: node 0x%p driver 0x%p\n", dn, dn->drv));
    CALLHOOKPKT(user_hook, dn->drv, hookMsg);
    
    AROS_USERFUNC_EXIT
}

void MouseHW__HW__EnumDrivers(OOP_Class *cl, OOP_Object *o, struct pHW_EnumDrivers *Msg)
{
    struct Hook enumHook =
    {
        .h_Entry = (HOOKFUNC)enumFunc,
        .h_Data  = Msg->callback
    };
    struct pHW_EnumDrivers enum_msg =
    {
        .mID      = Msg->mID,
        .callback = &enumHook,
        .hookMsg  = Msg->hookMsg
    };
    
    OOP_DoSuperMethod(cl, o, &enum_msg.mID);
}
