/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "hiddclass_intern.h"

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_HW

    NOTES
        This class is a base class for all hardware subsystems in AROS. Its
        main purpose is to manage hardware drivers.
        
        A "subsystem" is a kind of devices, e. g. keyboards, mice, etc. This
        class stores information about existing device instances and can provide
        this information to user's software.

        A typical subsystem class should be a singletone. This greatly simplifies
        handling it.

*****************************************************************************************/

#define IS_HW_ATTR(attr, idx) ((idx = attr - HWAttrBase) < num_HW_Attrs)

/*****************************************************************************************

    NAME
        aoHW_ClassName

    SYNOPSIS
        [I.G], CONST_STRPTR

    LOCATION
        CLID_HW

    FUNCTION
        Query human-readable description of the class. E. g. "Keyboards".

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHW_InUse

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_HW

    FUNCTION
        Return TRUE if the subsystem is currently in use by some driver(s)
        and FALSE otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *HW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);    
    if (o)
    {
        struct HWData *data = OOP_INST_DATA(cl, o);

        NEWLIST(&data->drivers);
        InitSemaphore(&data->driver_lock);

        data->name = (const char *)GetTagData(aHW_ClassName,
                                              (IPTR)"Unknown hardware",
                                              msg->attrList);
    }
    return o;
}

void HW__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HWData *data = OOP_INST_DATA(cl, o);
    IPTR idx;

    if (IS_HW_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHW_ClassName:
            *msg->storage = (IPTR)data->name;
            return;

        case aoHW_InUse:
            *msg->storage = IsListEmpty(&data->drivers) ? FALSE : TRUE;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*****************************************************************************************

    NAME
        moHW_AddDriver

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHW_AddHardwareDriver *Msg);

        OOP_Object *HW_AddDriver(OOP_Object *obj, OOP_Class *driverClass,
                                 struct TagItem *tags);

    LOCATION
        CLID_HW

    FUNCTION
        Creates a hardware driver object and registers it in the subsystem.

    INPUTS
        obj         - A subsystem object to operate on.
        driverClass - A pointer to OOP class of the driver. In order to create an object
                      of some previously registered public class, use
                      oop.library/OOP_FindClass().
        tags        - An optional taglist which will be passed to driver class' New() method.

    RESULT
        A pointer to driver object or NULL in case of failure.

    NOTES
        Do not dispose the returned object yourself, use HW_RemoveDriver() for it.

    EXAMPLE

    BUGS

    SEE ALSO
        moHW_RemHardwareDriver

    INTERNALS

*****************************************************************************************/

OOP_Object *HW__HW__AddDriver(OOP_Class *cl, OOP_Object *o,
            struct pHW_AddDriver *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HWData *data = OOP_INST_DATA(cl, o);
    OOP_Object *drv = NULL;
    struct DriverNode *dn;

    D(bug("[HW] Adding Driver class 0x%p\n", msg->driverClass));

    if (msg->driverClass != NULL)
    {
        // Get some extra memory for driver node
        dn = AllocPooled(CSD(cl)->MemPool, sizeof(struct DriverNode));
        if (dn)
        {
            drv = OOP_NewObject(msg->driverClass, NULL, msg->tags);

            if (!drv)
            {
                FreePooled(CSD(cl)->MemPool, dn, sizeof(struct DriverNode));
                D(bug("[HW] Driver did not initialize\n"));
                return NULL;
            }

            if (HW_SetupDriver(o, drv))
            {
                /* Add the driver to the end of drivers list */
                dn->driverObject = drv;
                ObtainSemaphore(&data->driver_lock);
                ADDTAIL(&data->drivers, dn);
                ReleaseSemaphore(&data->driver_lock);
                
                return drv;
            }

            OOP_DisposeObject(drv);
        }
    }
    return NULL;
}

/*****************************************************************************************

    NAME
	moHW_RemoveDriver

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHW_RemoveDriver *Msg);

	void HW_RemoveDriver(OOP_Object *obj, OOP_Object *driver);

    LOCATION
	CLID_HW

    FUNCTION
	Unregisters and disposes hardware driver object.

    INPUTS
	obj    - A subsystem object from which the driver should be removed.
	driver - A pointer to a driver object, returned by HW_AddDriver().

    RESULT
	TRUE if removal succesful or FALSE upon failure.

    NOTES
        The base class returns FALSE only if the driver is not registered
        in the subsystem. However, subclasses implementing actual subsystems
        can add their own checks, for example whether the driver currently
        owns some objects which are in use, and thus cannot be disposed.

    EXAMPLE

    BUGS

    SEE ALSO
	moHW_AddDriver

    INTERNALS

*****************************************************************************************/

BOOL HW__HW__RemoveDriver(OOP_Class *cl, OOP_Object *o,
                         struct pHW_RemoveDriver *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HWData *data = OOP_INST_DATA(cl, o);
    struct DriverNode *dn = NULL, *next = NULL, *rem = NULL;

    D(bug("[HW] Removing hardware driver %p\n", msg->driverObject));
 
    /* Get exclusive lock on driver list */
    ObtainSemaphore(&data->driver_lock);
    ForeachNodeSafe(&data->drivers, dn, next)
    {
        if (dn->driverObject == msg->driverObject)
        {
            Remove((struct Node *)dn);
            rem = dn;
        }
    }
    ReleaseSemaphore(&data->driver_lock);

    /* If driver removed, rem contains pointer to removed DriverNode */
    if (rem)
    {
        HW_CleanupDriver(o, rem->driverObject);

        /* Dispose driver */
        OOP_DisposeObject(rem->driverObject);
        /* And free memory for DriverNode */
        FreePooled(CSD(cl)->MemPool, rem, sizeof(struct DriverNode));
    }

    D(bug("[HW] Mgr::RemHardwareDriver() %s\n", rem ? "succeeded" : "failed"));

    return rem ? TRUE : FALSE;
}

/*****************************************************************************************

    NAME
	moHW_EnumDrivers

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHW_EnumDrivers *Msg);

	void HW_EnumDrivers(OOP_Object *obj, struct Hook *callback, APTR hookMsg);

    LOCATION
	CLID_HW

    FUNCTION
	Enumerates all installed driver in the subsystem.

    INPUTS
	obj      - A subsystem object to query.
	callback - A user-supplied hook which will be called for every driver.
        hookMsg  - A user-defined data to be passed to the hook.

        The hook will be called with the following parameters:
            AROS_UFHA(struct Hook *, hook        , A0)
                - A pointer to hook structure itself
            AROS_UFHA(OOP_Object * , driverObject, A2)
                - A device driver object
            AROS_UFHA(APTR         , message     , A1)
                - User-defined data

        The hook should return FALSE in order to continue enumeration
        or TRUE in order to stop it.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        The function uses internal semaphore locking. Because of this,
        it is illegal to attempt to add or remove drivers within the hook.

*****************************************************************************************/

void HW__HW__EnumDrivers(OOP_Class *cl, OOP_Object *o, struct pHW_EnumDrivers *msg)
{
    struct HWData *data = OOP_INST_DATA(cl, o);
    struct DriverNode *dn;

    /* Lock driver list for shared use */
    ObtainSemaphoreShared(&data->driver_lock);

    /* For every driver in the system... */
    ForeachNode(&data->drivers, dn)
    {
        /*
         * Explicit cast to BOOL makes it possible to declare the hook as
         * returning BOOL, not IPTR. BOOL is 16-bit wide, so higher bits
         * of result may carry random trash.
         */
        BOOL stop = CALLHOOKPKT(msg->callback, dn->driverObject, msg->hookMsg);

        if (stop)
            break;
    }

    ReleaseSemaphore(&data->driver_lock);
}

/*****************************************************************************************

    NAME
	moHW_SetupDriver

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHW_SetupDriver *Msg);

	void HW_SetupDriver(OOP_Object *obj, OOP_Object *driverObject);

    LOCATION
	CLID_HW

    FUNCTION
	Performs subsystem-specific setup after driver object creation.
        This method is intended to be used only by subclasses of CLID_HW.

    INPUTS
	obj          - A subsystem object.
        driverObject - Device driver object.

    RESULT
	TRUE if setup completed succesfully and FALSE in case of error.
        If this method returns error, the driver object will be disposed
        and moHW_AddDriver method will fail.

    NOTES
        In base class this method does nothing and always returns TRUE.

    EXAMPLE

    BUGS

    SEE ALSO
        moHW_CleanupDriver

    INTERNALS

*****************************************************************************************/

BOOL HW__HW__SetupDriver(OOP_Class *cl, OOP_Object *o, struct pHW_SetupDriver *msg)
{
    /* By default we have nothing to do here */
    return TRUE;
}

/*****************************************************************************************

    NAME
	moHW_CleanupDriver

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHW_CleanupDriver *Msg);

	void HW_CleanupDriver(OOP_Object *obj, OOP_Object *driverObject);

    LOCATION
	CLID_HW

    FUNCTION
	Performs subsystem-specific cleanup before driver object disposal.
        This method is intended to be used only by subclasses of CLID_HW.

    INPUTS
	obj          - A subsystem object.
        driverObject - Device driver object.

    RESULT
	None.

    NOTES
        In base class this method does nothing.

    EXAMPLE

    BUGS

    SEE ALSO
        moHW_SetupDriver

    INTERNALS

*****************************************************************************************/

void HW__HW__CleanupDriver(OOP_Class *cl, OOP_Object *o, struct pHW_CleanupDriver *msg)
{
    /* By default we have nothing to do here */
}
