/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
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

#include "serial_intern.h"


#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SegList)
#define LC_RESIDENTNAME         hiddserial_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT | RTF_COLDSTART
#define LC_RESIDENTPRI          9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->hdg_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

/* to avoid removing the serialhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

void serial_int_uart1(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
void serial_int_uart2(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

#undef OOPBase
#define OOPBase (csd->oopbase)

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
	struct class_static_data *csd; /* SerialHidd static data */

	EnterFunc(bug("SerialHIDD_Init()\n"));

	/*
		We map the memory into the shared memory space, because it is
		to be accessed by many processes, eg searching for a HIDD etc.

		Well, maybe once we've got MP this might help...:-)
	*/
	csd = AllocVec(sizeof(struct class_static_data), MEMF_CLEAR|MEMF_PUBLIC);
	lh->hdg_csd = csd;
	if(csd) {
		csd->sysbase = SysBase;
		
		D(bug("  Got csd\n"));

		csd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
		if (csd->oopbase) {
			D(bug("  Got OOPBase\n"));
			csd->utilitybase = OpenLibrary("utility.library", 37);
			if (csd->utilitybase) {
				D(bug("  Got UtilityBase\n"));

				csd->serialhiddclass = init_serialhiddclass(csd);

				D(bug("  SerialHiddClass: %p\n", csd->serialhiddclass));

				if(csd->serialhiddclass) {
					D(bug("  Got SerialHIDDClass\n"));
			
					csd->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
			
					if (csd->irqhidd) {
						HIDDT_IRQ_Handler *irq;
			
						/* Install COM1 and COM3 interrupt */
						irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
						if(!irq) {
							kprintf("  ERROR: Cannot install Serial\n");
							Alert( AT_DeadEnd | AN_IntrMem );
						}
						irq->h_Node.ln_Pri=127;		/* Set the highest pri */
						irq->h_Code = serial_int_uart1;
						irq->h_Data = (APTR)csd;
						HIDD_IRQ_AddHandler(csd->irqhidd, irq, vHidd_IRQ_Serial1);

						/* Install UART1 and UART2 interrupt */
						irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
						if(!irq) {
							kprintf("  ERROR: Cannot install Serial\n");
							Alert( AT_DeadEnd | AN_IntrMem );
						}
						irq->h_Node.ln_Pri=127;		/* Set the highest pri */
						irq->h_Code = serial_int_uart2;
						irq->h_Data = (APTR)csd;
						HIDD_IRQ_AddHandler(csd->irqhidd, irq, vHidd_IRQ_Serial2);

						D(bug("  Got Interrupts\n"));
						ReturnInt("SerialHIDD_Init", ULONG, TRUE);
					}
				}

				CloseLibrary(csd->utilitybase);
			}
			CloseLibrary(csd->oopbase);
		}

		FreeVec(csd);
		lh->hdg_csd = NULL;
	}


	ReturnInt("SerialHIDD_Init", ULONG, FALSE);
		
}

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
	EnterFunc(bug("SerialHIDD_Expunge()\n"));

	if(lh->hdg_csd) {
		free_serialhiddclass(lh->hdg_csd);

		CloseLibrary(lh->hdg_csd->utilitybase);
		CloseLibrary(lh->hdg_csd->oopbase);

		FreeVec(lh->hdg_csd);
	}

	ReturnVoid("SerialHIDD_Expunge");
}
