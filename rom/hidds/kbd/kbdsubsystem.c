/*
    Copyright (C) 2013-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "kbd.h"

/*****************************************************************************************

    NAME
        --background_kbdsubsystem--

    LOCATION
        CLID_HW_Kbd

    NOTES
        This class represents a keyboard input subsystem in AROS. Additionally
        it serves as a "hub" for collecting input from various keyboard devices
        in the system. Events from all keyboard devices are merged into a single
        stream and propagated to all clients.

        In order to get an access to keyboard input subsystem you need to
        create an object of CLID_HW_Kbd class. The actual returned object is a
        singletone, you do not have to dispose it, and every call will return
        the same object pointer. After getting this object, you can, for example,
        register your driver using moHW_AddDriver method, or enumerate drivers
        using moHW_EnumDrivers.

        If you wish to receive keyboard events, use objects of CLID_Hidd_Kbd
        class. This class implements the same interface as driver class, but
        represents receiver's side and is responsible for registering user's
        interrupt handler in the listeners chain. These objects are not real
        drivers and do not need to me registered within the subsystem.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        --hardware_drivers--

    LOCATION
        CLID_HW_Kbd

    NOTES
        A hardware driver should be a subclass of CLID_Hidd_Kbd and implement IID_Hidd_Kbd
        interface according to the following rules:

        1. A single object of driver class represents a single hardware unit.
        2. A single driver object maintains a single callback address (passed to it
           using aoHidd_Input_IrqHandler). Under normal conditions this callback is supplied
           by CLID_Hidd_Input class.

*****************************************************************************************/

OOP_Object *KBDHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct kbd_staticdata *csd = CSD(cl);

    D(bug("[KbdHW] %s()\n", __func__));

    if (!csd->hwObj)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"Keyboards"},
            {TAG_DONE     , 0                }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        csd->hwObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }
    return csd->hwObj;
}
