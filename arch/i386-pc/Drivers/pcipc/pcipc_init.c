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

#include <utility/utility.h>

#define DEBUG 0

#include <proto/exec.h>
#include <aros/debug.h>


#include "pci.h"
#include LC_LIBDEFS_FILE

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

static const struct Resident Pci_Resident = {
    RTC_MATCHWORD,
    &Pci_Resident,
    &LIBEND,
    RTF_SINGLETASK | RTF_AUTOINIT,
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
    &Pci_init
};

AROS_UFH3(LIBBASETYPEPTR, Pci_init,
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(ULONG, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct pci_staticdata *psd;
        
    LIBBASE->lib_Node.ln_Pri = Pci_Resident.rt_Pri;
    LIBBASE->lib_Node.ln_Name = Pci_Resident.rt_Name;
    LIBBASE->lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->lib_Version = VERSION_NUMBER;
    LIBBASE->lib_Revision = REVISION_NUMBER;
    LIBBASE->lib_IdString = &Pci_VersionID[6];

    D(bug("PCPCI: Initializing\n"));

    psd = AllocMem(sizeof(struct pci_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
    
    if (psd)
    {
	psd->sysbase = SysBase;
	psd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (psd->oopbase)
	{
	    psd->utilitybase = OpenLibrary(UTILITYNAME, 0);
	    {
#if 0
		int ret;
		asm volatile(
			"int $0x80"
			:"=a"(ret)
			:"a"(__NR_iopl),"b"(3));

		D(bug("LinuxPCI: iopl(3)=%d\n", ret));
#endif
    		if (init_pcidriverclass(psd))
		{
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

AROS_LH1(LIBBASETYPEPTR, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPEPTR, LIBBASE, 1, pcipc)
{
    AROS_LIBFUNC_INIT

    LIBBASE->lib_OpenCnt++;
    return (LIBBASE);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, close,
    LIBBASETYPEPTR, LIBBASE, 2, pcipc)
{
    AROS_LIBFUNC_INIT
    
    LIBBASE->lib_OpenCnt--;
    return(0);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, expunge,
    LIBBASETYPEPTR, LIBBASE, 3, pcipc)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(LONG, null,
    LIBBASETYPEPTR, LIBBASE, 4, pcipc)
{
    AROS_LIBFUNC_INIT

    return(0);

    AROS_LIBFUNC_EXIT
}

