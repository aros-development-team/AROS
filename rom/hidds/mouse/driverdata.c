/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <hidd/mouse.h>
#include <oop/oop.h>
#include <proto/utility.h>

#include "mouse.h"

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

static void GlobalCallback(struct driverNode *drv, struct pHidd_Mouse_ExtEvent *ev)
{
    struct pHidd_Mouse_ExtEvent xev;
    struct mouse_data *data;

    /* Ignore the event if the driver is not activated yet */
    if (!drv->callbacks)
        return;

    /*
     * The event passed in may be pHidd_Mouse_Event instead of pHidd_Mouse_ExtEvent,
     * according to flags. In this case we add own flags
     */
    if (drv->flags != vHidd_Mouse_Extended)
    {
        xev.button = ev->button;
        xev.x      = ev->x;
        xev.y      = ev->y;
        xev.type   = ev->type;
        xev.flags  = drv->flags;
        
        ev = &xev;
    }

    for (data = (struct mouse_data *)drv->callbacks->mlh_Head; data->node.mln_Succ;
         data = (struct mouse_data *)data->node.mln_Succ)
    {
        data->callback(data->callbackdata, ev);
    }
}

OOP_Object *DriverData__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    OOP_Class *driverClass;

    driverClass = (OOP_Class *)GetTagData(aHidd_DriverData_ClassPtr, 0, msg->attrList);
    D(bug("[Mouse] AddHardwareDriver(0x%p)\n", driverClass));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct driverNode *drvnode = OOP_INST_DATA(cl, o);
        struct TagItem tags[] =
        {
            { aHidd_Mouse_IrqHandler    , (IPTR)GlobalCallback},
            { aHidd_Mouse_IrqHandlerData, (IPTR)drvnode       },
            { TAG_MORE                  , (IPTR)msg->attrList }
        };

        drvnode->drv = OOP_NewObject(driverClass, NULL, tags);
        D(bug("[Mouse] Driver node 0x%p, driver 0x%p\n", drvnode, drvnode->drv));

        if (drvnode->drv)
        {
            struct mouse_staticdata *csd = CSD(cl);
            IPTR val = FALSE;

            OOP_GetAttr(drvnode->drv, aHidd_Mouse_Extended, &val);
            D(bug("[Mouse] Extended event: %d\n", val));
            if (val)
            {
                drvnode->flags = vHidd_Mouse_Extended;
            }
            else
            {
                OOP_GetAttr(drvnode->drv, aHidd_Mouse_RelativeCoords, &val);
                D(bug("[Mouse] Relative coordinates: %d\n", val));
                drvnode->flags = val ? vHidd_Mouse_Relative : 0;
            }
            /* This enables sending interrupts to clients */
            drvnode->callbacks = &csd->callbacks;

            return o;
        }
        else
        {
            /*
             * One more trick saving us from headache with OOP_GetMethodID():
             * Given a method ID, we can obtain MethodBase of the same
             * interface by simply subtracting method offset.
             */
            OOP_MethodID rootMethodBase = msg->mID - moRoot_New;
            OOP_MethodID dispose_msg = rootMethodBase + moRoot_Dispose;

            OOP_DoSuperMethod(cl, o, &dispose_msg);
        }
    }
    return NULL;
}

void DriverData__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct driverNode *drvnode = OOP_INST_DATA(cl, o);

    OOP_DisposeObject(drvnode->drv);
    OOP_DoSuperMethod(cl, o, msg);
}
