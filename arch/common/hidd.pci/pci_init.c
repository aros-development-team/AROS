/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <utility/utility.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>


#include "pci.h"
#include LC_LIBDEFS_FILE

AROS_SET_LIBFUNC(PCI_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("[PCI] Initializing PCI system\n"));
    LIBBASE->MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);

    LIBBASE->psd.MemPool = LIBBASE->MemPool;

    InitSemaphore(&LIBBASE->psd.driver_lock);
    NEWLIST(&LIBBASE->psd.drivers);

    return LIBBASE->psd.MemPool != NULL;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(PCI_Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

#if 0	// Removing of drivers already done by driver classes	
	/*
	    Ok. Class is not used ATM and therefore it is safe (well 
	    Disable/Enable protected) to iterate through driver lists and free
	    everything that can be freed
	*/
    D(bug("[PCI] Expunging drivers and devices\n"));
    ForeachNodeSafe(&LIBBASE->psd.drivers, (struct Node *)dn, (struct Node *)next)
    {
	struct PciDevice *dev, *next;
	    
	Remove((struct Node *)dn);

	/* For every device */
	ForeachNodeSafe(&dn->devices, (struct Node *)dev, (struct Node *)next)
	{
	    /* Dispose PCIDevice object instance */
	    OOP_DisposeObject(dev->device);

	    /* Remove device from device list */
	    Remove((struct Node *)dev);
	}
	
	/* Dispose driver */
	OOP_DisposeObject(dn->driverObject);
    }

    /* All objects deleted by now. Free classes */
#endif

    D(bug("[PCI] Destroying MemoryPool\n"));
    DeletePool(LIBBASE->MemPool);
    
    D(bug("[PCI] Goodbye\n"));
   
    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(PCI_Init, 0)
ADD2EXPUNGELIB(PCI_Expunge, 0)
