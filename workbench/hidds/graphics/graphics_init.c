/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include "graphics_intern.h"


#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->hdg_SegList)
#define LC_RESIDENTNAME         hiddgraphics_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT
#define LC_RESIDENTPRI          0
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->hdg_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef SysBase
#define SysBase      (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct class_static_data *csd; /* GfxHidd static data */

    EnterFunc(bug("GfxHIDD_Init()\n"));

    /*
        We map the memory into the shared memory space, because it is
        to be accessed by many processes, eg searching for a HIDD etc.

        Well, maybe once we've got MP this might help...:-)
    */
    csd = AllocVec(sizeof(struct class_static_data), MEMF_CLEAR|MEMF_PUBLIC);
    lh->hdg_csd = csd;
    if(csd)
    {
        csd->sysbase = SysBase;
        
        D(bug("  Got csd\n"));

        csd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
        if (csd->oopbase)
        {
            D(bug("  Got OOPBase\n"));
            csd->utilitybase = OpenLibrary("utility.library", 37);
            if (csd->utilitybase)
            {
                D(bug("  Got UtilityBase\n"));

                csd->gfxhiddclass = init_gfxhiddclass(csd);

                D(bug("  GfxHiddClass: %p\n", csd->gfxhiddclass));

                if(csd->gfxhiddclass)
                {
                    D(bug("  Got GfxHIDDClass\n"));
                    ReturnInt("GfxHIDD_Init", ULONG, TRUE);
                }

                CloseLibrary(csd->utilitybase);
            }
            CloseLibrary(csd->oopbase);
        }

        FreeVec(csd);
        lh->hdg_csd = NULL;
    }


    ReturnInt("GfxHIDD_Init", ULONG, FALSE);
        
}


void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    EnterFunc(bug("GfxHIDD_Expunge()\n"));

    if(lh->hdg_csd)
    {
        free_gfxhiddclass(lh->hdg_csd);

        CloseLibrary(lh->hdg_csd->utilitybase);
        CloseLibrary(lh->hdg_csd->oopbase);

        FreeVec(lh->hdg_csd);
    }

    ReturnVoid("GfxHIDD_Expunge");
}

