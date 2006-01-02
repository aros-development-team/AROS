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

#include <dos/bptr.h>

#include <hidd/pci.h>

#include <utility/utility.h>

//#include "/usr/include/asm/unistd.h"

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>


#include "pci.h"
#include LC_LIBDEFS_FILE

#undef LIBBASETYPE
#undef LIBBASETYPEPTR

#define LIBBASETYPE	struct pcibase
#define LIBBASETYPEPTR	struct pcibase *

#ifdef SysBase
#undef SysBase
#endif

#define __NR_iopl   (110)
#define __NR_open   (5)
#define __NR_close  (6)

#ifndef __ROM__
__used static LONG __no_exec() 
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
    RTF_COLDSTART | RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    90,
    Pci_Name,
    &Pci_VersionID[6],
    (ULONG*)inittabl
};

static const APTR inittabl[4] =
{
    (APTR)sizeof(LIBBASETYPE),
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
        
    LIBBASE->sysBase = SysBase;
    LIBBASE->LibNode.lib_Node.ln_Pri = Pci_Resident.rt_Pri;
    LIBBASE->LibNode.lib_Node.ln_Name = Pci_Resident.rt_Name;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString = &Pci_VersionID[6];

    D(bug("LinuxPCI: Initializing\n"));

    psd = AllocMem(sizeof(struct pci_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
    LIBBASE->psd = psd;
    
    if (psd)
    {
	psd->slist = slist;
	psd->sysbase = SysBase;
	psd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (psd->oopbase)
	{
	    psd->utilitybase = OpenLibrary(UTILITYNAME, 0);
	    {
		int ret;
		asm volatile(
			"int $0x80"
			:"=a"(ret)
			:"a"(__NR_iopl),"b"(3));

		asm volatile(
			"int $0x80"
			:"=a"(psd->fd)
			:"a"(__NR_open),"b"("/dev/mem"),"c"(2));

		D(bug("LinuxPCI: iopl(3)=%d\n", ret));
		D(bug("LinuxPCI: /dev/mem fd=%d\n", psd->fd));

    		if (ret==0 && init_pcidriverclass(psd))
		{
		    return LIBBASE;
		}

		D(bug("LinuxPCI: has to be root in order to use this hidd\n"));
	    }
	    CloseLibrary(psd->oopbase);
	}
	FreeMem(psd, sizeof(struct pci_staticdata));
    }

    return LIBBASE;
    
    AROS_USERFUNC_EXIT
}

#define SysBase ((struct ExecBase *)LIBBASE->sysBase)
#define OOPBase ((struct Library *)LIBBASE->psd->oopbase)

AROS_LH1(LIBBASETYPEPTR, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPEPTR, LIBBASE, 1, pcilinux)
{
    AROS_LIBFUNC_INIT

    LIBBASE->LibNode.lib_OpenCnt++;
    return (LIBBASE);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, close,
    LIBBASETYPEPTR, LIBBASE, 2, pcilinux)
{
    AROS_LIBFUNC_INIT
    
    LIBBASE->LibNode.lib_OpenCnt--;
    return(0);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
    LIBBASETYPEPTR, LIBBASE, 3, pcilinux)
{
    AROS_LIBFUNC_INIT
    BPTR slist = LIBBASE->psd->slist;
    
    D(bug("[PCILinux] expunge\n"));

    /* Try to open PCI subsystem */
    OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
	/* If PCI successed to open, remove your driver from subsystem */
	struct pHidd_PCI_RemHardwareDriver msg;

	msg.driverClass = LIBBASE->psd->driverClass;
	msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_RemHardwareDriver);

	D(bug("[PCILinux] Removing driver\n"));
	if (OOP_DoMethod(pci, (OOP_Msg)&msg) == FALSE)
	{
	    /*
		Failed to remove itself? There has to be a reason. Set delayed
		expunge flag and do nothing now
	    */
	    LIBBASE->LibNode.lib_Flags |= LIBF_DELEXP;
	    slist = NULL; /* DOn't allow expunge */
	    D(bug("[PCILinux] PCI class refused to remove driver for some reason. Delaying expunge then\n"));
	}
	OOP_DisposeObject(pci);
    }
    
    /* slist not null? That means we may expunge now */
    if (slist)
    {
	UBYTE *negptr = (UBYTE*)LIBBASE;
	ULONG negsize, possize, fullsize;

	asm volatile(
		"int $0x80"
		:
		:"a"(__NR_close),"b"(LIBBASE->psd->fd));

	free_pcidriverclass(LIBBASE->psd, LIBBASE->psd->driverClass);
	CloseLibrary(LIBBASE->psd->utilitybase);
	CloseLibrary(LIBBASE->psd->oopbase);

	Remove((struct Library *)LIBBASE);

	FreeMem(LIBBASE->psd, sizeof(struct pci_staticdata));
	
	negsize = LIBBASE->LibNode.lib_NegSize;
	possize = LIBBASE->LibNode.lib_PosSize;
	fullsize = negsize + possize;
	negptr -= negsize;

	FreeMem(negptr, fullsize);
    }
    
    return slist;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(LONG, null,
    LIBBASETYPEPTR, LIBBASE, 4, pcilinux)
{
    AROS_LIBFUNC_INIT

    return(0);

    AROS_LIBFUNC_EXIT
}

