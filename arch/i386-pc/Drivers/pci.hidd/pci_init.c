/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ system for standalone i386 AROS
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "pci.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME         pciHidd_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_OPENLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct pcibase
{
    struct Library          library;
    struct ExecBase         *sysbase;
    BPTR                    seglist;
    struct pci_staticdata   *psd;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef SysBase

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct pci_staticdata *psd;

    D(bug("PCI: Initializing\n"));

    psd = AllocMem( sizeof (struct pci_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    lh->psd = psd;
    if (psd)
    {
		NEWLIST(&psd->devices);

		scanPCIBuses(psd, SysBase);
		
        psd->sysbase = SysBase;
        psd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
        if (psd->oopbase)
        {
            psd->utilitybase = OpenLibrary(UTILITYNAME, 37);
            if (psd->utilitybase)
            {
                psd->pciclass = init_pciclass(psd);

				if(psd->pciclass)
                {
					D(bug("     Init OK\n"));
					return TRUE;
				}
            }
            CloseLibrary(psd->oopbase);
        }
        FreeMem(psd, sizeof (struct pci_staticdata));
    }
    return FALSE;
}
