/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <proto/utility.h>

#include "controller.h"

/*
 * This file contains private DriverData class.
 * The problem is that together with the actual driver object we have
 * to store some information about the driver, together with the callback.
 * Unfortunately our drivers do not have any base class, so we cannot just
 * add fields to its structure. In order to circumvent this limitation,
 * we use proxy objects instead of real driver objects. Proxy object
 * encapsulates driver object together with all the needed information.
 * (De)masquerading drivers is done in subsystem class.
 */

static void ControllerInput__CallBackFunc(struct ControllerInput_DriverData *drv, struct pHidd_Controller_ExtEvent *ev)
{
    struct pHidd_Controller_ExtEvent xev;

    D(bug("[ControllerHidd:DD] %s(0x%p, 0x%p)\n", __func__, drv, ev));
    if (drv->inputcb) {
        drv->inputcb(drv->inputcbd, ev);
    }
}

OOP_Object *DriverData__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    OOP_Class *driverClass;

    D(bug("[ControllerHidd:DD] %s(0x%p, 0x%p, 0x%p)\n", __func__, cl, o, msg));

    driverClass = (OOP_Class *)GetTagData(aHidd_DriverData_ClassPtr, 0, msg->attrList);
    D(bug("[ControllerHidd:DD] %s: DriverClass @ 0x%p\n", __func__, driverClass));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o) {
        struct ControllerInput_DriverData *drvnode = OOP_INST_DATA(cl, o);
        struct TagItem tags[] =
        {
            { aHidd_Input_IrqHandler,       (IPTR)ControllerInput__CallBackFunc  },
            { aHidd_Input_IrqHandlerData,   (IPTR)drvnode                   },
            { TAG_MORE,                     (IPTR)msg->attrList             }
        };
        struct TagItem *handlerTag = FindTagItem(aHidd_Input_IrqHandler, msg->attrList);
        if (handlerTag) {
            drvnode->inputcb = (APTR)handlerTag->ti_Data;
            drvnode->inputcbd = (APTR)GetTagData(aHW_Input_ConsumerList, 0, msg->attrList);
            handlerTag->ti_Tag = TAG_IGNORE;
        }

        drvnode->drv = OOP_NewObject(driverClass, NULL, tags);
        D(bug("[ControllerHidd:DD] Driver node 0x%p, driver 0x%p\n", drvnode, drvnode->drv));

        if (drvnode->drv) {
            struct controller_staticdata *csd = CSD(cl);
            IPTR val = FALSE;

            OOP_GetAttr(drvnode->drv, aHidd_Controller_Extended, &val);
            if (val) {
                D(bug("[ControllerHidd:DD] Extended event reporting\n"));
                drvnode->flags = vHidd_Controller_Extended;
            } else {
                OOP_GetAttr(drvnode->drv, aHidd_Controller_RelativeCoords, &val);
                D(bug("[ControllerHidd:DD] Relative coordinates: %d\n", val));
            }
        } else {
            /*
             * One more trick saving us from headache with OOP_GetMethodID():
             * Given a method ID, we can obtain MethodBase of the same
             * interface by simply subtracting method offset.
             */
            OOP_MethodID rootMethodBase = msg->mID - moRoot_New;
            OOP_MethodID dispose_msg = rootMethodBase + moRoot_Dispose;

            OOP_DoSuperMethod(cl, o, &dispose_msg);
            o = NULL;
        }
    }
    return o;
}

void DriverData__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct ControllerInput_DriverData *drvnode = OOP_INST_DATA(cl, o);

    OOP_DisposeObject(drvnode->drv);
    OOP_DoSuperMethod(cl, o, msg);
}
