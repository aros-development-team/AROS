/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
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
#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->SegList)
#define LC_RESIDENTNAME		USBUHCIHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_EXPUNGELIB
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct USBUHCIBase
{
    struct Library library;
    struct ExecBase *sysBase;
    BPTR	SegList;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef SysBase
#undef OOPBase

#define SysBase xsd->sysBase
#define OOPBase xsd->oopBase

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

#undef SysBase
#define SysBase (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh) {
struct USBUHCI_staticdata *xsd;

	xsd = AllocMem(sizeof(struct USBUHCI_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
	if (xsd)
	{
		xsd->sysBase = SysBase;
		xsd->oopBase = OpenLibrary(AROSOOP_NAME, 0);
		if (xsd->oopBase)
		{
			xsd->utilityBase = OpenLibrary(UTILITYNAME, 37);
			if (xsd->utilityBase)
			{
				xsd->pcihidd = OOP_NewObject(NULL, CLID_Hidd_PCIBus, NULL);
				if (xsd->pcihidd)
				{
					if (findCard(xsd))
					{
						D(bug("Found USB UHCI\n"));
						return TRUE;
					}
				}
				CloseLibrary(xsd->utilityBase);
			}
			CloseLibrary(xsd->oopBase);
		}
		FreeMem(xsd, sizeof (struct USBUHCI_staticdata));
    }
    return FALSE;
}

