/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics amiga intuition hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>

#include <proto/exec.h>

#include "graphics_intern.h"


#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct IntHIDDGraphicsAmigaIntuiBase *)(lib))->hdg_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct IntHIDDGraphicsAmigaIntuiBase *)(lib))->hdg_SegList)
/*
#define LC_RESIDENTNAME         HIDDGraphics_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT
*/
#define LC_RESIDENTPRI          94
#define LC_LIBBASESIZE          sizeof(struct IntHIDDGraphicsAmigaIntuiBase)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct IntHIDDGraphicsAmigaIntuiBase *)(lib))->hdg_LibNode)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_STATIC_EXPUNGELIB
#define LC_STATIC_INITLIB

/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#define SysBase      (LC_SYSBASE_FIELD(lh))

static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct class_static_data *csd; /* GfxHidd static data */
    
    EnterFunc(bug("GfxHIDDAmigaIntui_Init()\n"));

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
            csd->utilitybase = OpenLibrary("utility.library", 37);
            if (csd->utilitybase)
            {
                csd->intuitionbase = OpenLibrary("intuition.library", 37);
                if (csd->intuitionbase)
                {
                    csd->superclassbase = OpenLibrary("graphics.hidd", 0);
                    if (csd->superclassbase)
                    {
                        D(bug("Got Libraries\n"));
                        csd->gfxhiddclass = init_gfxhiddclass(csd);
        
                        D(bug("GfxHiddAmigaIntuiClass: %p\n", csd->gfxhiddclass));
        
                        if(csd->gfxhiddclass)
                        {
                            D(bug("  Got GfxHIDDClass\n"));
                            ReturnInt("GfxHIDD_Init", ULONG, TRUE);
        
                        }

                        CloseLibrary(csd->superclassbase);
                        csd->superclassbase = NULL;

                    }

                    CloseLibrary(csd->intuitionbase);
                    csd->intuitionbase = NULL;
                }

                CloseLibrary(csd->utilitybase);
                csd->utilitybase = NULL;
            }

            CloseLibrary(csd->oopbase);
            csd->oopbase = NULL;
        }

        FreeVec(csd);
        lh->hdg_csd = NULL;
    }

    ReturnInt("GfxHIDDAmigaIntui_Init", ULONG, FALSE);
        
}


static void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    EnterFunc(bug("GfxHIDDAmigaIntui_Expunge()\n"));

    if(lh->hdg_csd)
    {
        free_gfxhiddclass(lh->hdg_csd);

        if(lh->hdg_csd->superclassbase) CloseLibrary(lh->hdg_csd->superclassbase);
        if(lh->hdg_csd->intuitionbase)  CloseLibrary(lh->hdg_csd->intuitionbase);
        if(lh->hdg_csd->oopbase)        CloseLibrary(lh->hdg_csd->oopbase);
        if(lh->hdg_csd->utilitybase)    CloseLibrary(lh->hdg_csd->utilitybase);

        FreeVec(lh->hdg_csd);
    }

    ReturnVoid("GfxHIDDAmigaIntui_Expunge");
}

