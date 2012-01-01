/*
    Copyright (C) 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

#undef HiddKbdAB
#define	HiddKbdAB		(CSD(cl)->hiddKbdAB)

#include <hidd/keyboard.h>

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	CLID_Hidd_Kbd

    NOTES
	This class represents a "hub" for collecting input from various
	keyboard devices in the system and sending them to clients.

	In order to get an access to keyboard input subsystem you need to
	create an object of CLID_Hidd_Kbd class. There can be two use
	scenarios: driver mode and client mode.

	If you wish to run in client mode (receive keyboard events), you
	have to supply a callback using	aoHidd_Kbd_IrqHandler attribute.
	After this your callback will be called every time the event arrives
	until you dispose your object.

	Events from all keyboard devices are merged into a single stream
	and propagated to all clients.

	In driver mode you don't need to supply a callback (however it's not
	forbidden). Instead you use the master object for registering your
	hardware driver using HIDD_Kbd_AddHardwareDriver(). It is safe to
	dispose the master object after adding a driver, the driver will
	be internally kept in place.

*****************************************************************************************/

/*****************************************************************************************

    NAME
	--hardware_drivers--

    LOCATION
	CLID_Hidd_Kbd

    NOTES
	A hardware driver should implement the same interface according to the following
	rules:

	1. A single object of driver class represents a single hardware unit.
	2. A single driver object maintains a single callback address (passed to it
	   using aoHidd_Kbd_IrqHandler). Under normal conditions this callback is supplied
	   by CLID_Hidd_Kbd class.
	3. HIDD_Kbd_AddHardwareDriver() and HIDD_Kbd_RemHardwareDriver() on a driver object
	   itself do not make sense, so there's no need to implement them.

	A hardware driver class should be a subclass of CLID_Hidd in order to ensure
	compatibility in future.

*****************************************************************************************/

static void GlobalCallback(struct kbd_staticdata *csd, UWORD code)
{
    struct kbd_data *data;
    
    for (data = (struct kbd_data *)csd->callbacks.mlh_Head; data->node.mln_Succ;
         data = (struct kbd_data *)data->node.mln_Succ)
	data->callback(data->callbackdata, code);
}

/*****************************************************************************************

    NAME
	aoHidd_Kbd_IrqHandler

    SYNOPSIS
	[I..], APTR

    LOCATION
	CLID_Hidd_Kbd

    FUNCTION
	Specifies a keyboard event handler. The handler will called be every time a
	keyboard event happens. A "C" calling convention is used, declare the handler
	functions as follows:

	void KeyboardIRQ(APTR data, UWORD keyCode)

	Handler parameters are:
	    data    - Anything you specify using aoHidd_Kbd_IrqHandlerData
	    keyCode - A raw key code as specified in devices/rawkeycodes.h.
		      Key release event is indicated by ORing this value
		      with IECODE_UP_PREFIX (defined in devices/inputevent.h)

	The handler is called inside interrupts, so usual restrictions apply to it.

    NOTES

    EXAMPLE

    BUGS
	Not all hosted drivers provide this attribute.

    SEE ALSO
	aoHidd_Kbd_IrqHandlerData

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	aoHidd_Kbd_IrqHandlerData

    SYNOPSIS
	[I..], APTR

    LOCATION
	CLID_Hidd_Kbd

    FUNCTION
	Specifies a user-defined value that will be passed to IRQ handler as a first
	parameter. The purpose of this is to pass some static data to the handler.
	The system will not assume anything about this value.

	Defaults to NULL if not specified.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aoHidd_Kbd_IrqHandler

    INTERNALS

******************************************************************************************/

OOP_Object *KBD__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct kbd_data *data;
    struct TagItem *tag, *tstate;
    struct Library *UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);

    if (!UtilityBase)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o) {
        CloseLibrary(UtilityBase);
        return NULL;
    }

    data = OOP_INST_DATA(cl, o);
    data->callback = NULL;
    data->callbackdata = NULL;

    tstate = msg->attrList;
    D(bug("tstate: %p\n", tstate));

    while ((tag = NextTagItem(&tstate)))
    {
	ULONG idx;

	D(bug("Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));

	if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Kbd hidd tag\n"));
	    switch (idx)
	    {
		case aoHidd_Kbd_IrqHandler:
		    data->callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
		    break;

		case aoHidd_Kbd_IrqHandlerData:
		    data->callbackdata = (APTR)tag->ti_Data;
		    D(bug("Got data %p\n", (APTR)tag->ti_Data));
		    break;
	    }
	}
    } /* while (tags to process) */

    /* Add to interrupts list if we have a callback */
    if (data->callback) {
        Disable();
        AddTail((struct List *)&CSD(cl)->callbacks, (struct Node *)data);
	Enable();
    }

    CloseLibrary(UtilityBase);
    return o;
}

VOID KBD__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);

    if (data->callback) {
        Disable();
	Remove((struct Node *)data);
	Enable();
    }
    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * The following two methods are small stubs providing means for future expansion.
 * In future we could support enumeration of devices and specifying
 * which device we wish to read events from (in case if we want to implement
 * amigainput.library or something like it)
 */

/*****************************************************************************************

    NAME
	moHidd_Kbd_AddHardwareDriver

    SYNOPSIS
	OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Kbd_AddHardwareDriver *Msg);

	OOP_Object *HIDD_Kbd_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass,
                                               struct TagItem *tags);

    LOCATION
	CLID_Hidd_Kbd

    FUNCTION
	Creates a hardware driver object and registers it in the system.

	It does not matter on which instance of CLID_Hidd_Kbd class this method is
	used. Hardware driver objects are shared between all of them.

    INPUTS
	obj	    - Any object of CLID_Hidd_Kbd class.
	driverClass - A pointer to OOP class of the driver. In order to create an object
		      of some previously registered public class, use
		      oop.library/OOP_FindClass().
	tags	    - An optional taglist which will be passed to driver class' New() method.

    RESULT
	A pointer to driver object.

    NOTES
	Do not dispose the returned object yourself, use HIDD_Kbd_RemHardwareDriver() for it.

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Kbd_RemHardwareDriver

    INTERNALS
	This method supplies own interrupt handler to the driver, do not override this.

*****************************************************************************************/

OOP_Object *KBD__Hidd_Kbd__AddHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_AddHardwareDriver *Msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct TagItem tags[] = {
	{ aHidd_Kbd_IrqHandler	  , (IPTR)GlobalCallback	},
	{ aHidd_Kbd_IrqHandlerData, (IPTR)CSD(cl) 		},
	{ TAG_MORE		  , (IPTR)Msg->tags		}
    };

    return OOP_NewObject(Msg->driverClass, NULL, tags);
}

/*****************************************************************************************

    NAME
	moHidd_Kbd_RemHardwareDriver

    SYNOPSIS
	void OOP_DoMethod(OOP_Object *obj, struct pHidd_Kbd_RemHardwareDriver *Msg);

	void HIDD_Kbd_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver);

    LOCATION
	CLID_Hidd_Kbd

    FUNCTION
	Unregisters and disposes keyboard hardware driver object.

	It does not matter on which instance of CLID_Hidd_Kbd class this method is
	used. Hardware driver objects are shared between all of them.

    INPUTS
	obj    - Any object of CLID_Hidd_Kbd class.
	driver - A pointer to a driver object, returned by HIDD_Kbd_AddHardwareDriver().

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	moHidd_Kbd_AddHardwareDriver

    INTERNALS

*****************************************************************************************/

void KBD__Hidd_Kbd__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_RemHardwareDriver *Msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;

    OOP_DisposeObject(Msg->driverObject);
}

/* Class initialization and destruction */

static int KBD_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(bug("[KBD] Base Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_Kbd);

    return TRUE;
}

static int KBD_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(bug("[KBD] base class initialization\n"));

    LIBBASE->csd.hiddKbdAB = OOP_ObtainAttrBase(IID_Hidd_Kbd);

    if (LIBBASE->csd.hiddKbdAB)
    {
	NewList((struct List *)&LIBBASE->csd.callbacks);
	D(bug("[KBD] Everything OK\n"));
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(KBD_InitClass, 0)
ADD2EXPUNGELIB(KBD_ExpungeClass, 0)
