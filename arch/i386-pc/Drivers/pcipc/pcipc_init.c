/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <hidd/pci.h>

#include <utility/utility.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>


#include "pci.h"
#include LC_LIBDEFS_FILE

#define __NR_iopl   (110)

static int PCPCI_Expunge(LIBBASETYPEPTR LIBBASE)
{
    int ok;
	
    OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
	struct pHidd_PCI_RemHardwareDriver msg;
	
	msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_RemHardwareDriver);
	msg.driverClass = LIBBASE->psd.driverClass;

	ok = OOP_DoMethod(pci, (OOP_Msg)&msg);

	OOP_DisposeObject(pci);
    }
    else
	ok = FALSE;

    return ok;
}

ADD2EXPUNGELIB(PCPCI_Expunge, 0)
ADD2LIBS("pci.hidd", 0, static struct Library *, __pcihidd)
