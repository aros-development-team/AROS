/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "kbd.h"

/*****************************************************************************************

    NAME
        --background--

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
        A hardware driver should be a subclass of CLID_Hidd and implement IID_Hidd_Kbd
        interface according to the following rules:

        1. A single object of driver class represents a single hardware unit.
        2. A single driver object maintains a single callback address (passed to it
           using aoHidd_Kbd_IrqHandler). Under normal conditions this callback is supplied
           by CLID_Hidd_Kbd class.

*****************************************************************************************/

static void GlobalCallback(struct kbd_staticdata *csd, UWORD code)
{
    struct kbd_data *data;
    
    for (data = (struct kbd_data *)csd->callbacks.mlh_Head; data->node.mln_Succ;
         data = (struct kbd_data *)data->node.mln_Succ)
    {
        data->callback(data->callbackdata, code);
    }
}

OOP_Object *KBDHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct kbd_staticdata *csd = CSD(cl);

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

VOID KBDHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{

}

OOP_Object *KBDHW__HW__AddDriver(OOP_Class *cl, OOP_Object *o, struct pHW_AddDriver *Msg)
{
    struct TagItem tags[] =
    {
        { aHidd_Kbd_IrqHandler    , (IPTR)GlobalCallback        },
        { aHidd_Kbd_IrqHandlerData, (IPTR)CSD(cl)               },
        { TAG_MORE                , (IPTR)Msg->tags             }
    };
    struct pHW_AddDriver add_msg =
    {
        .mID         = Msg->mID,
        .driverClass = Msg->driverClass,
        .tags        = tags
    };

    D(bug("[KBD] Adding driver %s, tags 0x%p\n",
          Msg->driverClass->ClassNode.ln_Name, Msg->tags));

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &add_msg.mID);
}
