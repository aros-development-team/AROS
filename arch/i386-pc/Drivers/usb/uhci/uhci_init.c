/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: USB UHCI Host Controller Driver
    Lang: english
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <hidd/graphics.h>
#include <hidd/pcibus.h>
#include <oop/oop.h>
#include <utility/utility.h>
#warning: prototypes which should actually be in some public header
HIDDT_PCI_Device **HIDD_PCI_FindDevice(OOP_Object *obj, struct TagItem *tags);
VOID HIDD_PCI_FreeQuery(OOP_Object *obj, HIDDT_PCI_Device **devices);

#include "hardware.h"
#include "uhciclass.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

STATIC BOOL findCard(struct USBUHCI_staticdata *xsd) {
HIDDT_PCI_Device **ptr;
HIDDT_PCI_Device **res;
struct TagItem findpcitags[] =
{
	{tHidd_PCI_VendorID,  VENDOR_INTEL},
	{tHidd_PCI_Class,     BASE_CLASS_SERIAL},
	{tHidd_PCI_SubClass,  SUB_CLASS_USB},
	{tHidd_PCI_Interface, INTERFACE_UHCI},
	{TAG_DONE,            0UL}
	
};
	xsd->card = NULL;
	res = ptr = HIDD_PCI_FindDevice(xsd->pcihidd, findpcitags);
	if (*ptr)
	{
		if ((*ptr)->DeviceID == DEVICE_82371AB_2)
		{
			D(bug("found intel 82371AB_2\n"));
		}
		else
			D(bug("found unknown USB HC device (0x%lx)\n", (*ptr)->DeviceID));
		xsd->card = *ptr;
	}
	HIDD_PCI_FreeQuery(xsd->pcihidd, res);
	if (xsd->card)
	{
		if (!initUSBUHCIHW(&xsd->data, xsd->card))
		{
			D(bug("init hw error\n"));
			xsd->card = NULL;
		}
	}
	return (xsd->card) ? TRUE : FALSE;
}

AROS_SET_LIBFUNC(PCUSB_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    struct USBUHCI_staticdata *xsd = &LIBBASE;

    xsd->pcihidd = OOP_NewObject(NULL, CLID_Hidd_PCIBus, NULL);
    if (xsd->pcihidd)
    {
	if (findCard(xsd))
	{
	    D(bug("Found USB UHCI\n"));
	    return TRUE;
	}
    }
    
    return FALSE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(PCUSB_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    struct USBUHCI_staticdata *xsd = &LIBBASE->usd;

    OOP_DisposeObject(xsd->pcihidd);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(PCUSB_Init, 0)
ADD2INITLIB(PCUSB_Expunge, 0)
