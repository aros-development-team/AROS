/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ system for standalone i386 AROS
    Lang: english
*/

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

#include "irq.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME         irqHidd_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          90
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_OPENLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct irqbase
{
    struct Library          library;
    struct ExecBase         *sysbase;
    BPTR                    seglist;
    struct irq_staticdata   *isd;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef SysBase

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct irq_staticdata *isd;

    D(bug("IRQ: Initializing\n"));

    isd = AllocMem( sizeof (struct irq_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    lh->isd = isd;
    if (isd)
    {
        isd->sysbase = SysBase;
        isd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
        if (isd->oopbase)
        {
            isd->utilitybase = OpenLibrary(UTILITYNAME, 37);
            if (isd->utilitybase)
            {
                int i;
		
                /* Initialize IRQ lists */
                for (i = 0; i < 16; i++)
                {
                    NEWLIST(&isd->irqlist[i]);
                }
		
                isd->irqclass = init_irqclass(isd);

                if(isd->irqclass)
                {
		    Disable();
                    init_Servers(isd);	/* Initialize all known IRQ servers */
                    Enable();		/* Turn interrupts on */

                    D(bug("     Init OK\n"));
                    return TRUE;
                }
            }
            CloseLibrary(isd->oopbase);
        }
        FreeMem(isd, sizeof (struct irq_staticdata));
    }
    return FALSE;
}

