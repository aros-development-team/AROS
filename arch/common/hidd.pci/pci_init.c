/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
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
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <utility/utility.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>


#include "pci.h"
#include LC_LIBDEFS_FILE

#undef LIBBASETYPEPTR
#undef LIBBASETYPE

#define LIBBASETYPE 	struct pcibase
#define LIBBASETYPEPTR 	struct pcibase *

#ifdef SysBase
#undef SysBase
#endif

#ifndef __ROM__
static LONG __no_exec()
{
    return -1;
}
#endif

static const char Pci_VersionID[] = VERSION_STRING;
static const char Pci_Name[] = NAME_STRING;

static const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];

const struct Resident Pci_Resident = {
    RTC_MATCHWORD,
    &Pci_Resident,
    &LIBEND,
    RTF_SINGLETASK | RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    90,
    Pci_Name,
    &Pci_VersionID[6],
    (ULONG*)inittabl
};

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct pcibase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &Pci_init
};

AROS_UFH3(LIBBASETYPEPTR, Pci_init,
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(BPTR, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct pci_staticdata *psd;
    
    D(bug("[PCI] PCIBase=%x\n", LIBBASE));
    /* Initialize the library-related things */
    LIBBASE->segList = slist;
    LIBBASE->sysBase = SysBase;
    LIBBASE->LibNode.lib_Node.ln_Pri = Pci_Resident.rt_Pri;
    LIBBASE->LibNode.lib_Node.ln_Name = Pci_Resident.rt_Name;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString = &Pci_VersionID[6];

    D(bug("[PCI] Initializing PCI system\n"));
    LIBBASE->MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);

    /* Get some space for static data of the classes */
    psd = AllocPooled(LIBBASE->MemPool, sizeof(struct pci_staticdata));

    if (psd)
    {
	psd->MemPool = LIBBASE->MemPool;
	LIBBASE->psd = psd;

	D(bug("[PCI] Got StaticData\n"));

	InitSemaphore(&psd->driver_lock);
	NEWLIST(&psd->drivers);
	psd->sysbase = SysBase;
	psd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (psd->oopbase)
	{
	    D(bug("[PCI] Got OOPBase @ 0x%08x\n", psd->oopbase));
	    psd->utilitybase = OpenLibrary(UTILITYNAME, 0);
	    {
		D(bug("[PCI] Got UtilityBase @ 0x%08x\n", psd->utilitybase));
		if (init_pciclass(psd))
		{
		    if (init_pcideviceclass(psd))
		    {
			if (init_pcidriverclass(psd))
			{
			    return LIBBASE;
			}
		    }
		}
	    }
	    CloseLibrary(psd->oopbase);
	}
	FreeMem(psd, sizeof(struct pci_staticdata));
    }

    return LIBBASE;
    
    AROS_USERFUNC_EXIT
}

#define SysBase ((struct ExecBase *)((struct pcibase*)LIBBASE->sysBase))
#undef PSD
#define PSD ((struct pci_staticdata *)((struct pcibase*)LIBBASE->psd))

AROS_LH1(LIBBASETYPEPTR, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPEPTR, LIBBASE, 1, pci)
{
    AROS_LIBFUNC_INIT

    LIBBASE->LibNode.lib_OpenCnt++;
    return (LIBBASE);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, close,
    LIBBASETYPEPTR, LIBBASE, 2, pci)
{
    AROS_LIBFUNC_INIT
    
    LIBBASE->LibNode.lib_OpenCnt--;
    return(0);
    
    AROS_LIBFUNC_EXIT
}

#undef SysBase
#define SysBase sysBase
#define OOPBase (PSD->oopbase)

AROS_LH1(BPTR, expunge,
    AROS_LHA(LIBBASETYPEPTR, LIBBASE, D0),
    struct ExecBase *, sysBase, 3, pci)
{
    AROS_LIBFUNC_INIT
    BPTR segList = NULL;

    Disable();
    /*
	If classes used, don't expunge even if OpenCnt == 0
	If drivers list is not empty, set delayed expunge too and wait untill
	all drivers are expunged
    */
    if (PSD->users > 0 || LIBBASE->LibNode.lib_OpenCnt > 0 || !IsListEmpty(&PSD->drivers))
    {
	D(bug("[PCI] Cannot expunge. pciclass in use\n"));
	LIBBASE->LibNode.lib_Flags |= LIBF_DELEXP;
    }
    else
    {
//	struct DriverNode *dn = NULL, *next = NULL;
	UBYTE *negptr = (UBYTE*)LIBBASE;
	ULONG negsize, possize, fullsize;
#if 0	// Removing of drivers already done by driver classes	
	/*
	    Ok. Class is not used ATM and therefore it is safe (well 
	    Disable/Enable protected) to iterate through driver lists and free
	    everything that can be freed
	*/
	D(bug("[PCI] Expunging drivers and devices\n"));
	ForeachNodeSafe(&PSD->drivers, (struct Node *)dn, (struct Node *)next)
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
	D(bug("[PCI] Destroying classes\n"));
	free_pcidriverclass(PSD, PSD->pciDriverClass);
	free_pcideviceclass(PSD, PSD->pciDeviceClass);
	free_pciclass(PSD, PSD->pciClass);

	D(bug("[PCI] Closing libraries\n"));
	CloseLibrary(PSD->utilitybase);
	CloseLibrary(PSD->oopbase);

	D(bug("[PCI] Destroying MemoryPool\n"));
	DeletePool(LIBBASE->MemPool);
	
	segList = LIBBASE->segList;
    
	D(bug("[PCI] Hidd removed\n"));
	Remove((struct Library *)LIBBASE);

	negsize = LIBBASE->LibNode.lib_NegSize;
	possize = LIBBASE->LibNode.lib_PosSize;
	fullsize = negsize + possize;
	negptr -= negsize;

	FreeMem(negptr, fullsize);
	D(bug("[PCI] Goodbye\n"));
    }
    Enable();
   
    return segList;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(LONG, null,
    LIBBASETYPEPTR, LIBBASE, 4, pci)
{
    AROS_LIBFUNC_INIT

    return(0);

    AROS_LIBFUNC_EXIT
}

