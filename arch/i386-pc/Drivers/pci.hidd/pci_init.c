/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI bus Hidd for standalone i386 AROS
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "pci.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		pciHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct pcibase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef kprintf

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct pci_staticdata *xsd;
    int i;

    D(bug("PCI: Initializing\n"));

    xsd = AllocMem( sizeof (struct pci_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (xsd)
    {
        xsd->sysbase = SysBase;
        xsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (xsd->oopbase)
	{
	    xsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
	    if (xsd->utilitybase)
	    {
		PCIHWProbe();
		return TRUE;
	    }
	    CloseLibrary(xsd->oopbase);
	}
	FreeMem(xsd, sizeof (struct pci_staticdata));
    }
    return FALSE;
}
