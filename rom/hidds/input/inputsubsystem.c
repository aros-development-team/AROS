/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <proto/utility.h>

#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "input.h"

/*****************************************************************************************

    NAME
        --background_inputsubsystem--

    LOCATION
        CLID_HW_Input

    NOTES
        This class represents an input subsystem in AROS.
        Additionally, it serves as a "hub" for collecting input from the subsystems
        devices in the system. Events from all subsystem input devices are merged
        into a single stream and propagated to all clients.

        In order to get an access to input subsystem you need to create an object of
        CLID_HW_Input class. The actual returned object is a singletone, you do
        not have to dispose it, and every call will return the same object pointer.

        After getting this object, you can, for example, enumerate drivers
        using moHW_EnumDrivers.

        Hardware drivers should register with the subsystems HW Driver, using
        moHW_AddDriver method.

        If you wish to receive input events, use objects of CLID_Hidd_Input
        class.

        This class implements the same interface as driver class, but
        represents receiver's side and is responsible for registering user's
        interrupt handler in the listeners chain. These objects are not real
        drivers and do not need to be registered within the subsystem.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        --hardware_drivers--

    LOCATION
        CLID_HW_Input

    NOTES
        A hardware driver should be a subclass of CLID_Hidd_Input, and implement IID_Hidd_Input
        interface according to the following rules:

        1. A single object of driver class represents a single hardware unit.
        2. A single driver object maintains a single callback address (passed to it
           using aoHidd_Input_IrqHandler). Under normal conditions this callback is supplied
           by CLID_Hidd_Input class.

*****************************************************************************************/

static void InputHW__CallBackFunc(struct MinList *cbList, InputIrqData_t cbData)
{
    struct InputHWInstData *data;

    D(bug("[Input:HW] %s(0x%p)\n", __func__, cbList));

    for (data = (struct InputHWInstData *)cbList->mlh_Head; data->ihid_node.mln_Succ;
         data = (struct InputHWInstData *)data->ihid_node.mln_Succ)
    {
        data->ihid_callback(data->ihid_private, cbData);
    }
}

OOP_Object *InputHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct InputClassStaticData *icsd = __ICSD(cl);
    struct Library              *UtilityBase = icsd->icsd_UtilityBase;
    struct TagItem              *tag, *tstate;
    char                        *inputSSName = NULL;
    OOP_Object                  *inputSSObj = NULL;

    D(bug("[Input:HW] %s(0x%p, 0x%p, 0x%p)\n", __func__, cl, o, msg));

    tstate = msg->attrList;
    D(bug("[Input:HW] %s: tstate: %p\n", __func__, tstate));
    while ((tag = NextTagItem(&tstate))) {
        ULONG idx;
        if (IS_HW_ATTR(tag->ti_Tag, idx)) {
            switch (idx) {
                case aoHW_ClassName:
                    inputSSName = (char *)tag->ti_Data;
                    break;
            }
        }
    }

    if (inputSSName) {
        D(bug("[Input:HW] %s: '%s' Input HW Subsystem class\n", __func__, inputSSName));
        inputSSObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        if (inputSSObj) {
            struct InputHWData *hd = OOP_INST_DATA(cl, inputSSObj);
            D(bug("[Input:HW] %s: Consumer List @ 0x%p\n", __func__, &hd->ihd_consumers));
            NEWLIST(&hd->ihd_consumers);
        }
    }
    D(bug("[Input:HW] %s: Returning 0x%p\n", __func__, inputSSObj));
    return inputSSObj;
}

VOID InputHW__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct InputHWData *hd = OOP_INST_DATA(cl, o);
    ULONG idx;
    
    D(bug("[Input:HW] %s()\n", __func__));

    if (IS_HWINPUT_ATTR(msg->attrID, idx)) {
        switch (idx) {
            case aoHW_Input_ConsumerList:
                *msg->storage = (IPTR)&hd->ihd_consumers;
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

VOID InputHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[Input:HW] %s()\n", __func__));
}

OOP_Object *InputHW__HW__AddDriver(OOP_Class *cl, OOP_Object *o, struct pHW_AddDriver *Msg)
{
    struct Library          *UtilityBase = __ICSD(cl)->icsd_UtilityBase;
    struct InputHWData      *hd = OOP_INST_DATA(cl, o);
    struct TagItem          tags[] =
    {
        { aHidd_Input_IrqHandler,       0               },
        { aHW_Input_ConsumerList,       0               },
        { TAG_MORE,                     (IPTR)Msg->tags }
    };
    struct pHW_AddDriver    add_msg =
    {
        .mID         = Msg->mID,
        .driverClass = Msg->driverClass,
        .tags        = tags
    };

    D(bug("[Input:HW] %s: Registering driver '%s', tags 0x%p\n",
            __func__,
            Msg->driverClass->ClassNode.ln_Name, Msg->tags));

    tags[0].ti_Data = GetTagData(aHidd_Input_IrqHandler, 0, Msg->tags);
    if (tags[0].ti_Data != 0) {
        D(bug("[Input:HW] %s: - using hw subsystem class input processing hook\n", __func__));
    } else {
        D(bug("[Input:HW] %s: - using default input processing hook\n", __func__));
        tags[0].ti_Data = (IPTR)InputHW__CallBackFunc;
    }

    if (tags[1].ti_Data == 0) {
        tags[1].ti_Data = (IPTR)&hd->ihd_consumers;
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &add_msg.mID);
}

void InputHW__HW_Input__PushEvent(OOP_Class *cl, OOP_Object *o, struct pHW_Input_PushEvent *Msg)
{
    struct InputHWData      *hd = OOP_INST_DATA(cl, o);
    struct InputHWInstData *data;

    D(bug("[Input:HW] %s()\n", __func__));

    ForeachNode(&hd->ihd_consumers, data) {
        data->ihid_callback(data->ihid_private, Msg->iedata);
    }
}
