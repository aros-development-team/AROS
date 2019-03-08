/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/ahci.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "ahci.h"

#define DIRQ(x)

#if (0)
static AROS_INTH1(ahciBus_Reset, struct ahci_Bus *, bus)
{
    AROS_INTFUNC_INIT

    struct AHCIBase *AHCIBase = bus->ab_Base;
    OOP_Object *obj = (void *)bus - AHCIBase->busClass->InstOffset;

 	D(bug ("[AHCI:Bus] ahciBus_Reset(%p)\n", bus);)

    HIDD_AHCIBus_Shutdown(obj);

    return FALSE;

    AROS_INTFUNC_EXIT
}
#endif

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_Hidd_AHCIBus

    NOTES
        This class serves as a base class for implementing AHCI SATA bus drivers.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_AHCIBus_Unit

    SYNOPSIS
        [..G], OOP_Object *

    LOCATION
        CLID_Hidd_AHCIBus

    FUNCTION
        Returns a pointer to OOP object of private unit class, representing
        a drive on the bus, or NULL if there's no device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *AHCIBus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct AHCIBase *AHCIBase = cl->UserData;
    struct ahci_port *ap = (struct ahci_port *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    D(bug ("[AHCI:Bus] Root__New()\n");)

    if (!ap)
            return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct ahci_Bus *data = OOP_INST_DATA(cl, o);
#if (0)
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_AHCIBus_Switch(tag->ti_Tag, idx)
            {
            }
        }
#endif
        /* Cache device base pointer. Useful. */
        data->ab_Base = AHCIBase;
        if ((data->ab_Port = ap) != NULL)
        {
            ap->ap_Object = o;
        }
        D(
          bug ("[AHCI:Bus] Root__New: AHCIBase @ %p\n", data->ab_Base);
          bug ("[AHCI:Bus] Root__New: ahci_port @ %p\n", data->ab_Port);
        )
    }
    return o;
}

void AHCIBus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
#if (0)
    struct ahci_Bus *data = OOP_INST_DATA(cl, o);
#endif
    D(bug ("[AHCI:Bus] Root__Dispose(%p)\n", o);)

    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * Here we take into account that the table can be either
 * terminated early, or have NULL entries.
 */
void AHCIBus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct AHCIBase *AHCIBase = cl->UserData;
#if (0)
    struct ahci_Bus *data = OOP_INST_DATA(cl, o);
#endif
    ULONG idx;

    Hidd_Bus_Switch (msg->attrID, idx)
    {
    case aoHidd_Bus_MaxUnits:
        *msg->storage = 1;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void AHCIBus__Hidd_StorageBus__EnumUnits(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageBus_EnumUnits *msg)
{
    struct ahci_Bus *data = OOP_INST_DATA(cl, o);

    D(bug ("[AHCI:Bus] Hidd_StorageBus__EnumUnits()\n");)

    if (data->ab_Unit)
    {
        CALLHOOKPKT(msg->callback, data->ab_Unit, msg->hookMsg);
    }
}

/*****************************************************************************************

    NAME
        moHidd_AHCIBus_Shutdown

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_AHCIBus_Shutdown *Msg);

        APTR HIDD_AHCIBus_Shutdown(void);

    LOCATION
        CLID_Hidd_AHCIBus

    FUNCTION
        Instantly shutdown all activity on the bus.

    INPUTS
        None

    RESULT
        None

    NOTES
        This method is called by ahci.device during system reset handler execution.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Default implementation disables interrupt using AltControl register.

*****************************************************************************************/

void AHCIBus__Hidd_AHCIBus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg *msg)
{
#if (0)
    struct ahci_Bus *data = OOP_INST_DATA(cl, o);
#endif
    D(bug ("[AHCI:Bus] AHCIBus__Shutdown(%p)\n", o);)
}

/***************** Private nonvirtual methods follow *****************/

BOOL Hidd_AHCIBus_Start(OOP_Object *o, struct AHCIBase *AHCIBase)
{
    D(bug ("[AHCI:Bus] AHCIBus_Start(%p)\n", o);)
#if (0)
    struct ahci_Bus *ab = OOP_INST_DATA(AHCIBase->busClass, o);
	struct ahci_port *ap = ab->ab_Port;

	while (ap->ap_signal & AP_SIGF_INIT)
		ahci_os_sleep(100);
	ahci_os_lock_port(ap);
	if (ahci_cam_attach(ap) == 0) {
		ahci_cam_changed(ap, NULL, -1);
		ahci_os_unlock_port(ap);
		while ((ap->ap_flags & AP_F_SCAN_COMPLETED) == 0) {
			ahci_os_sleep(100);
		}
	} else {
		ahci_os_unlock_port(ap);
	}
#endif

	return TRUE;
}


AROS_UFH3(BOOL, Hidd_AHCIBus_Open,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(OOP_Object *, obj, A2),
          AROS_UFHA(IPTR, reqUnit, A1))
{
    AROS_USERFUNC_INIT

    D(bug ("[AHCI:Bus] Hidd_AHCIBus_Open(%p)\n", obj);)
#if (0)
    struct IORequest *req = h->h_Data;
    struct AHCIBase *AHCIBase = (struct AHCIBase *)req->io_Device;
    struct ahci_Bus *b = (struct ahci_Bus *)OOP_INST_DATA(AHCIBase->busClass, obj);
    ULONG bus = reqUnit >> 1;
    UBYTE dev = reqUnit & 1;

    D(bug("[ATA%02ld] Checking bus %u dev %u\n", reqUnit, bus, dev));
    
    if ((b->ab_BusNum == bus) && b->ab_Units[dev])
    {
        /* Got the unit */
        req->io_Unit  = &b->ab_Units[dev]->au_Unit;
        req->io_Error = 0;

        b->ab_Units[dev]->au_Unit.unit_OpenCnt++;
        return TRUE;
    }
#endif
    return FALSE;

    AROS_USERFUNC_EXIT
}
