/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include "serial_intern.h"


#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SegList)
#define LC_RESIDENTNAME         hiddserial_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT
#define LC_RESIDENTPRI          0
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

#define sysBase      (LC_SYSBASE_FIELD(lh))

struct ExecBase * SysBase;

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
                    D(bug("  Got SerialHIDDClass\n"));
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

int __pthread_do_exit(void)
{
    kprintf("serial.hidd: __pthread_do_exit() was called\n");
    return 0;
}


int __pthread_thread_self(void)
{
    kprintf("serial.hidd: __pthread_thread_self() was called\n");
    return 0;
}
