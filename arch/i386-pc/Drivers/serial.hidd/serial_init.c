/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/system.h>
#include <aros/machine.h>
#include <aros/asmcall.h>

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
#define DEBUG 0
#include <aros/debug.h>

#define sysBase      (LC_SYSBASE_FIELD(lh))

struct ExecBase * SysBase;

AROS_UFP4(int, serial_int_13,
    AROS_UFHA(ULONG, dummy1, A0),
    AROS_UFHA(struct class_static_data *, csd, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

AROS_UFP4(int, serial_int_24,
    AROS_UFHA(ULONG, dummy1, A0),
    AROS_UFHA(struct class_static_data *, csd, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

int init_interrupts(struct class_static_data *csd);

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct class_static_data *csd; /* SerialHidd static data */

    SysBase = sysBase;    
    EnterFunc(bug("SerialHIDD_Init()\n"));

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

                csd->serialhiddclass = init_serialhiddclass(csd);

                D(bug("  SerialHiddClass: %p\n", csd->serialhiddclass));

                if(csd->serialhiddclass)
                {
		    struct Interrupt *is;
		    
                    D(bug("  Got SerialHIDDClass\n"));
		    
		    /* Install COM1 and COM3 interrupt */
		    is = (struct Interrupt *)AllocMem(sizeof(struct Interrupt), MEMF_CLEAR|MEMF_PUBLIC);
		    if(!is)
		    {
			kprintf("  ERROR: Cannot install Serial\n");
			Alert( AT_DeadEnd | AN_IntrMem );
		    }
		    is->is_Node.ln_Pri=127;		/* Set the highest pri */
		    is->is_Code = (void (*)())&serial_int_13;
		    is->is_Data = (APTR)csd;
		    AddIntServer(0x80000004,is);	//<-- int_ser13

		    /* Install COM2 and COM4 interrupt */
		    is = (struct Interrupt *)AllocMem(sizeof(struct Interrupt), MEMF_CLEAR|MEMF_PUBLIC);
		    if(!is)
		    {
			kprintf("  ERROR: Cannot install Serial\n");
		    	Alert( AT_DeadEnd | AN_IntrMem );
		    }
		    is->is_Node.ln_Pri=127;		/* Set the highest pri */
		    is->is_Code = (void (*)())&serial_int_24;
		    is->is_Data = (APTR)csd;
		    AddIntServer(0x80000003,is);	//<-- int_ser24

		    D(bug("  Got Interrupts\n"));
		    ReturnInt("SerialHIDD_Init", ULONG, TRUE);
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

    if(lh->hdg_csd)
    {
        free_serialhiddclass(lh->hdg_csd);

        CloseLibrary(lh->hdg_csd->utilitybase);
        CloseLibrary(lh->hdg_csd->oopbase);

        FreeVec(lh->hdg_csd);
    }

    ReturnVoid("SerialHIDD_Expunge");
}

int init_interrupts(struct class_static_data *csd)
{
    struct Interrupt *is;

    /* Install COM1 and COM3 interrupt */
    is = (struct Interrupt *)AllocMem(sizeof(struct Interrupt), MEMF_CLEAR|MEMF_PUBLIC);
    if(!is)
    {
	kprintf("  ERROR: Cannot install Serial\n");
	Alert( AT_DeadEnd | AN_IntrMem );
    }
    is->is_Node.ln_Pri=127;		/* Set the highest pri */
    is->is_Code = (void (*)())&serial_int_13;
    is->is_Data = (APTR)csd;
    AddIntServer(0x80000004,is);	//<-- int_ser13

    /* Install COM2 and COM4 interrupt */
    is = (struct Interrupt *)AllocMem(sizeof(struct Interrupt), MEMF_CLEAR|MEMF_PUBLIC);
    if(!is)
    {
	kprintf("  ERROR: Cannot install Serial\n");
	Alert( AT_DeadEnd | AN_IntrMem );
    }
    is->is_Node.ln_Pri=127;		/* Set the highest pri */
    is->is_Code = (void (*)())&serial_int_24;
    is->is_Data = (APTR)csd;
    AddIntServer(0x80000003,is);	//<-- int_ser24

    return 1;
}
