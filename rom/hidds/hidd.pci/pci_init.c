/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
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
#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>


#include "pci.h"
#include LC_LIBDEFS_FILE

static int PCI_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI] Initializing PCI system\n"));
    
    LIBBASE->psd.kernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->psd.kernelBase)
        return FALSE;

    LIBBASE->psd.MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);
    D(bug("[PCI] Created pool 0x%p\n", LIBBASE->psd.MemPool));
    if (!LIBBASE->psd.MemPool)
        return FALSE;
        
    InitSemaphore(&LIBBASE->psd.driver_lock);
    NEWLIST(&LIBBASE->psd.drivers);

    return TRUE;
}

static int PCI_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI] Destroying MemoryPool\n"));
    DeletePool(LIBBASE->psd.MemPool);

    D(bug("[PCI] Goodbye\n"));

    return TRUE;
}

ADD2INITLIB(PCI_Init, 0)
ADD2EXPUNGELIB(PCI_Expunge, 0)
