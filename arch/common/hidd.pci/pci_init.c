/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
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

#include <utility/utility.h>

#define DEBUG 0

#include <proto/exec.h>
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

static
AROS_UFH3(LIBBASETYPEPTR, Pci_init,
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(ULONG, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct pci_staticdata *psd;

    /* Initialize the library-related things */
    LIBBASE->sysBase = SysBase;
    LIBBASE->LibNode.lib_Node.ln_Pri = Pci_Resident.rt_Pri;
    LIBBASE->LibNode.lib_Node.ln_Name = Pci_Resident.rt_Name;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString = &Pci_VersionID[6];

    D(bug("[PCI] Initializing PCI system\n"));

    /* Get some space for static data of the classes */
    psd = AllocMem(sizeof(struct pci_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
    
    if (psd)
    {
	D(bug("[PCI] Got StaticData\n"));
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

#define SysBase ((struct ExecBase *)LIBBASE->sysBase)

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

AROS_LH0I(void, expunge,
    LIBBASETYPEPTR, LIBBASE, 3, pci)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(LONG, null,
    LIBBASETYPEPTR, LIBBASE, 4, pci)
{
    AROS_LIBFUNC_INIT

    return(0);

    AROS_LIBFUNC_EXIT
}

