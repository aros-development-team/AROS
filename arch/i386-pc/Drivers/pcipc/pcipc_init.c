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

#include <hidd/pci.h>

#include <dos/bptr.h>

#include <utility/utility.h>

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

static const struct Resident Pcipc_Resident __used = {
    RTC_MATCHWORD,
    &Pcipc_Resident,
    &LIBEND,
    RTF_COLDSTART | RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    89,
    Pci_Name,
    &Pci_VersionID[6],
    (ULONG*)inittabl
};

static const APTR inittabl[4] =
{
    (APTR)sizeof(LIBBASETYPE),
    (APTR)LIBFUNCTABLE,
    NULL,
    &Pcipc_init
};

AROS_UFH3(LIBBASETYPEPTR, Pcipc_init,
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(BPTR, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct pci_staticdata *psd;

    LIBBASE->sysBase = SysBase;
    LIBBASE->LibNode.lib_Node.ln_Pri = Pcipc_Resident.rt_Pri;
    LIBBASE->LibNode.lib_Node.ln_Name = Pcipc_Resident.rt_Name;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString = &Pci_VersionID[6];

    D(bug("PCIPC: Initializing\n"));

    psd = AllocMem(sizeof(struct pci_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
    LIBBASE->psd = psd;
    
    if (psd)
    {
	D(bug("PCIPC: Got psd\n"));
	psd->sysbase = SysBase;
	psd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (psd->oopbase)
	{
	    D(bug("PCIPC: Got OOP\n"));
	    psd->utilitybase = OpenLibrary(UTILITYNAME, 0);
	    if (psd->utilitybase)
	    {
		D(bug("PCIPC: Got Utility\n"));
    		if (init_pcipcdriverclass(psd))
		{
		    D(bug("PCIPC: Init ok\n"));
		    return LIBBASE;
		}
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
    LIBBASETYPEPTR, LIBBASE, 1, pcipc)
{
    AROS_LIBFUNC_INIT

    LIBBASE->LibNode.lib_OpenCnt++;
    return (LIBBASE);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, close,
    LIBBASETYPEPTR, LIBBASE, 2, pcipc)
{
    AROS_LIBFUNC_INIT
    
    LIBBASE->LibNode.lib_OpenCnt--;
    return(0);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
    LIBBASETYPEPTR, LIBBASE, 3, pcipc)
{
    AROS_LIBFUNC_INIT

    BPTR slist = LIBBASE->slist;

    OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
	struct pHidd_PCI_RemHardwareDriver msg;

	msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_RemHardwareDriver);
	msg.driverClass = LIBBASE->psd->driverClass;

	if (OOP_DoMethod(pci, (OOP_Msg)&msg) == FALSE)
	{
	    LIBBASE->LibNode.lib_Flags |= LIBF_DELEXP;
	    slist = NULL;
	}

	OOP_DisposeObject(pci);
    }

    if (slist)
    {
	UBYTE *negptr = (UBYTE*)LIBBASE;
	ULONG negsize, possize, fullsize;

	free_pcipcdriverclass(LIBBASE->psd, LIBBASE->psd->driverClass);
	CloseLibrary(LIBBASE->psd->utilitybase);
	CloseLibrary(LIBBASE->psd->oopbase);

	Remove((struct Node*)LIBBASE);

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
    LIBBASETYPEPTR, LIBBASE, 4, pcipc)
{
    AROS_LIBFUNC_INIT

    return(0);

    AROS_LIBFUNC_EXIT
}

