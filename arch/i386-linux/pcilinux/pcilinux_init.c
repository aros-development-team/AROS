/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <dos/bptr.h>

#include <hidd/pci.h>

#include <utility/utility.h>

//#include "/usr/include/asm/unistd.h"

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>

#define DEBUG 1
#include <aros/debug.h>


#include "pci.h"

#define __NR_iopl   (110)
#define __NR_open   (5)
#define __NR_close  (6)

static int PCILx_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("LinuxPCI: Initializing\n"));

    {
	int ret;
	asm volatile(
		"int $0x80"
		:"=a"(ret)
		:"a"(__NR_iopl),"b"(3)
	);

	asm volatile(
		"int $0x80"
		:"=a"(LIBBASE->psd.fd)
		:"a"(__NR_open),"b"("/dev/mem"),"c"(2)
	);

	D(bug("LinuxPCI: iopl(3)=%d\n", ret));
	D(bug("LinuxPCI: /dev/mem fd=%d\n", LIBBASE->psd.fd));

	if (ret==0)
	{
	    return TRUE;
	}

	D(bug("LinuxPCI: has to be root in order to use this hidd\n"));
    }

    return FALSE;
}

static BOOL PCILx_Expunge(LIBBASETYPEPTR LIBBASE)
{
    BOOL ret = TRUE;
    
    D(bug("[PCILinux] expunge\n"));

    /* Try to open PCI subsystem */
    OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
	/* If PCI successed to open, remove your driver from subsystem */
	struct pHidd_PCI_RemHardwareDriver msg;

	msg.driverClass = LIBBASE->psd.driverClass;
	msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_RemHardwareDriver);

	D(bug("[PCILinux] Removing driver\n"));
	if (OOP_DoMethod(pci, (OOP_Msg)&msg) == FALSE)
	{
	    ret = FALSE;
	    D(bug("[PCILinux] PCI class refused to remove driver for some reason. Delaying expunge then\n"));
	}
	OOP_DisposeObject(pci);
    }

    return ret;
}

ADD2INITLIB(PCILx_Init, 0)
ADD2EXPUNGELIB(PCILx_Expunge, 0)
