/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/oop.h>

#include "hiddclass_intern.h"

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_HW_Root

    NOTES
        This class represents a root of HIDD subsystem tree. In other words, it
        represents your computer. Calling HW_EnumDrivers() on it will enumerate
        installed subsystem classes.

        By design this class is a singletone. In order to get access to it, just
        call OOP_NewObject() on it. Every call will return the same pointer to
        the same object. You do not need to call OOP_Dispose object on it. Such
        calls will simply do nothing.

        Subsystem classes need to register themselves in the tree by calling
        HW_AddDriver() on this class. The class keeps an eye on the subsystem
        usage and will allow to remove it using HW_RemoveDriver() only if the
        subsystem being removed is not in use by any other components.

*****************************************************************************************/

OOP_Object *HWRoot__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct class_static_data *csd = CSD(cl);

    /*
     * This singletone lacks semaphore protection. It is OK since
     * our instance is created during initialization procedure.
     */
    if (!csd->hwroot)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"Computer"},
            {TAG_DONE     , 0               }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        csd->hwroot = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }

    return csd->hwroot;
}

void HWRoot__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* Do nothing here */
}

BOOL HWRoot__HW__RemoveDriver(OOP_Class *cl, OOP_Object *o,
                         struct pHW_RemoveDriver *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    IPTR used = TRUE;

    OOP_GetAttr(msg->driverObject, aHW_InUse, &used);
    if (used)
        return FALSE;
    
    return OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*
 * TODO: Computer can have some OEM strings, like and vendor name.
 * In future it would be nice to make use of them by implementing
 * respective attributes of I_Hidd interface.
 * Perhaps, in order to be able to retrieve this properties,
 * this class will become hardware-specific, will be separated
 * from hiddclass.hidd, and moved to BSP.
 */
