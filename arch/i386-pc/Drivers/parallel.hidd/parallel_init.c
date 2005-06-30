/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/system.h>
#include <aros/asmcall.h>

#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "parallel_intern.h"


#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SegList)
#define LC_RESIDENTNAME         hiddparallel_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT | RTF_COLDSTART
#define LC_RESIDENTPRI          9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->hdg_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

/* to avoid removing the parallelhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define sysBase      (LC_SYSBASE_FIELD(lh))

struct ExecBase * SysBase;

#undef OOPBase
#define OOPBase (csd->oopbase)

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct class_static_data *csd; /* ParallelHidd static data */

    SysBase = sysBase;    
    EnterFunc(bug("ParallelHIDD_Init()\n"));

    /*
        We map the memory into the shared memory space, because it is
        to be accessed by many processes, eg searching for a HIDD etc.

        Well, maybe once we've got MP this might help...:-)
    */
    csd = AllocVec(sizeof(struct class_static_data), MEMF_CLEAR|MEMF_PUBLIC);
    lh->hdg_csd = csd;
    if(csd)
    {
        csd->sysbase = sysBase;
        
        D(bug("  Got csd\n"));

        csd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
        if (csd->oopbase)
        {
            D(bug("  Got OOPBase\n"));
            csd->utilitybase = OpenLibrary("utility.library", 37);
            if (csd->utilitybase)
            {
                D(bug("  Got UtilityBase\n"));

                csd->parallelhiddclass = init_parallelhiddclass(csd);

                D(bug("  ParallelHiddClass: %p\n", csd->parallelhiddclass));

                if(csd->parallelhiddclass)
                {
                    D(bug("  Got ParallelHIDDClass\n"));

#if 0		    
		    csd->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
		    
		    if (csd->irqhidd)
		    {
			HIDDT_IRQ_Handler *irq;
			
			/* Install COM1 and COM3 interrupt */
			irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
			if(!irq)
			{
			    kprintf("  ERROR: Cannot install Parallel\n");
			    Alert( AT_DeadEnd | AN_IntrMem );
			}
			irq->h_Node.ln_Pri=127;		/* Set the highest pri */
			irq->h_Code = parallel_int_7;
			irq->h_Data = (APTR)csd;
			HIDD_IRQ_AddHandler(csd->irqhidd, irq, vHidd_IRQ_Parallel1);

			D(bug("  Got Interrupts\n"));
#endif

			ReturnInt("ParallelHIDD_Init", ULONG, TRUE);

#if 0
		    }
#endif
                }

                CloseLibrary(csd->utilitybase);
            }
            CloseLibrary(csd->oopbase);
        }

        FreeVec(csd);
        lh->hdg_csd = NULL;
    }


    ReturnInt("ParallelHIDD_Init", ULONG, FALSE);
        
}

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    EnterFunc(bug("ParallelHIDD_Expunge()\n"));

    if(lh->hdg_csd)
    {
        free_parallelhiddclass(lh->hdg_csd);

        CloseLibrary(lh->hdg_csd->utilitybase);
        CloseLibrary(lh->hdg_csd->oopbase);

        FreeVec(lh->hdg_csd);
    }

    ReturnVoid("ParallelHIDD_Expunge");
}
