/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "storage_intern.h"

OOP_Object *StorageController__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug ("[Storage:Controller] Root__New()\n");)
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HIDDStorageControllerData *data = OOP_INST_DATA(cl, o);
        NEWLIST(&data->scd_Buses);
        InitSemaphore(&data->scd_BusLock);
    }
    D(bug ("[Storage:Controller] Root__New: Instance @ 0x%p\n", o);)
    return o;
}

VOID StorageController__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[Storage:Controller] Root__Dispose(0x%p)\n", o);)
    OOP_DoSuperMethod(cl, o, msg);
}


/*****************************************************************************************

    NAME
        moHidd_StorageController_AddBus

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_StorageController_AddBus *Msg);

        OOP_Object *Hidd_StorageController_AddBus(OOP_Object *obj, OOP_Class *busClass,
                                 struct TagItem *tags);

    LOCATION
        CLID_HW

    FUNCTION
        Creates a bus driver object and registers it in the controller.

    INPUTS
        obj         - An ATA Controller object to operate on.
        busClass - A pointer to OOP class of the bus. In order to create an object
                      of some previously registered public class, use
                      oop.library/OOP_FindClass().
        tags        - An optional taglist which will be passed to bus class' New() method.

    RESULT
        A pointer to bus object or NULL in case of failure.

    NOTES
        Do not dispose the returned bus yourself, use Hidd_StorageController_RemoveBus() for it.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_StorageController_RemoveBus

    INTERNALS

*****************************************************************************************/

OOP_Object *StorageController__Hidd_StorageController__AddBus(OOP_Class *cl, OOP_Object *o,
            struct pHidd_StorageController_AddBus *msg)
{
    struct HIDDStorageControllerData *data = OOP_INST_DATA(cl, o);
    OOP_Object *bus = NULL;
    struct BusNode *bn;

    D(bug("[Storage:Controller] Hidd_StorageController__AddBus: Adding Bus class 0x%p\n", msg->busClass));

    if (msg->busClass != NULL)
    {
        // Get some extra memory for bus node
        bn = AllocPooled( CSD(cl)->cs_MemPool, sizeof(struct BusNode));
        if (bn)
        {
            D(bug("[Storage:Controller] Hidd_StorageController__AddBus: Bus Node @ 0x%p\n", bn));
            bus = OOP_NewObject(msg->busClass, NULL, msg->tags);

            if (!bus)
            {
                FreePooled( CSD(cl)->cs_MemPool, bn, sizeof(struct BusNode));
                D(bug("[Storage:Controller] Hidd_StorageController__AddBus: Failed to instantiate Bus\n"));
                return NULL;
            }

           D(bug("[Storage:Controller] Hidd_StorageController__AddBus: Bus Instance @ 0x%p\n", bus));

            if (HIDD_StorageController_SetUpBus(o, bus))
            {
                D(bug("[Storage:Controller] Hidd_StorageController__AddBus: Bus Initialized\n"));
                /* Add the driver to the end of drivers list */
                bn->busObject = bus;
                ObtainSemaphore(&data->scd_BusLock);
                ADDTAIL(&data->scd_Buses, bn);
                ReleaseSemaphore(&data->scd_BusLock);
                
                return bus;
            }

            D(bug("[Storage:Controller] Hidd_StorageController__AddBus: Bus failed to Initialize\n"));

            FreePooled( CSD(cl)->cs_MemPool, bn, sizeof(struct BusNode));
            OOP_DisposeObject(bus);
        }
    }
    return NULL;
}

/*****************************************************************************************

    NAME
	moHidd_StorageController_RemoveBus

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHidd_StorageController_RemoveBus *Msg);

	void Hidd_StorageController_RemoveBus(OOP_Object *obj, OOP_Object *driver);

    LOCATION
	CLID_HW

    FUNCTION
	Unregisters and disposes hardware bus object.

    INPUTS
	obj    - An ATA Controller object from which the bus should be removed.
	driver - A pointer to a bus object, returned by Hidd_StorageController_AddBus().

    RESULT
	TRUE if removal successful or FALSE upon failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_StorageController_AddBus

    INTERNALS

*****************************************************************************************/

BOOL StorageController__Hidd_StorageController__RemoveBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_RemoveBus *Msg)
{
    /*
     * Currently we don't support unloading bus drivers.
     * This is a very-very big TODO.
     */
    D(bug ("[Storage:Controller] Hidd_StorageController__RemoveBus(0x%p)\n", o);)
    return FALSE;
}

/*****************************************************************************************

    NAME
	moHidd_StorageController_EnumBuses

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHidd_StorageController_EnumBuses *Msg);

	void Hidd_StorageController_EnumBuses(OOP_Object *obj, struct Hook *callback, APTR hookMsg);

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
            AROS_UFHA(OOP_Object * , busObject, A2)
                - A bus object
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
        it is illegal to attempt to add or remove buses within the hook.

*****************************************************************************************/

void StorageController__Hidd_StorageController__EnumBuses(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_EnumBuses *msg)
{
    struct HIDDStorageControllerData *data = OOP_INST_DATA(cl, o);
    struct BusNode *bn;

    /* Lock Bus list for shared use */
    ObtainSemaphoreShared(&data->scd_BusLock);

    /* For every Bus on the controller... */
    ForeachNode(&data->scd_Buses, bn)
    {
        BOOL stop = CALLHOOKPKT(msg->callback, bn->busObject, msg->hookMsg);

        if (stop)
            break;
    }

    ReleaseSemaphore(&data->scd_BusLock);
}


/*****************************************************************************************

    NAME
	moHidd_StorageController_SetUpBus

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHidd_StorageController_SetUpBus *Msg);

	void Hidd_StorageController_SetUpBus(OOP_Object *obj, OOP_Object *busObject);

    LOCATION
	CLID_HW

    FUNCTION
	Performs subsystem-specific setup after driver object creation.
        This method is intended to be used only by subclasses of CLID_HW.

    INPUTS
	obj          - A subsystem object.
        busObject - Device driver object.

    RESULT
	TRUE if setup completed successfully and FALSE in case of error.
        If this method returns error, the driver object will be disposed
        and moHidd_StorageController_AddBus method will fail.

    NOTES
        In base class this method does nothing and always returns TRUE.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_StorageController_CleanUpBus

    INTERNALS

*****************************************************************************************/

BOOL StorageController__Hidd_StorageController__SetUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_SetUpBus *Msg)
{
    D(bug ("[Storage:Controller] Hidd_StorageController__SetUpBus(0x%p)\n", o);)

    /* By default we have nothing to do here */

    return TRUE;
}

/*****************************************************************************************

    NAME
	moHidd_StorageController_CleanUpBus

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHidd_StorageController_CleanUpBus *Msg);

	void Hidd_StorageController_CleanUpBus(OOP_Object *obj, OOP_Object *busObject);

    LOCATION
	CLID_HW

    FUNCTION
	Performs subsystem-specific cleanup before driver object disposal.
        This method is intended to be used only by subclasses of CLID_HW.

    INPUTS
	obj          - A subsystem object.
        busObject - Device driver object.

    RESULT
	None.

    NOTES
        In base class this method does nothing.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_StorageController_SetUpBus

    INTERNALS

*****************************************************************************************/

void StorageController__Hidd_StorageController__CleanUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_CleanUpBus *msg)
{
    D(bug ("[Storage:Controller] Hidd_StorageController__CleanUpBus(0x%p)\n", o);)
    /* By default we have nothing to do here */
}
