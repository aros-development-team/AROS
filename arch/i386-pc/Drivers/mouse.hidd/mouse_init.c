/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$ 

    Desc: Mouse hidd (COM/PS2/USB) for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "mouse.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME         mouseHidd_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct mousebase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR   seglist;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct mouse_staticdata *msd;

    D(bug("_mouse: Initializing\n"));

    msd = AllocMem( sizeof (struct mouse_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (msd)
    {
        InitSemaphore(&msd->sema);
        msd->sysbase = SysBase;
        msd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
        if (msd->oopbase)
        {
            msd->utilitybase = OpenLibrary(UTILITYNAME, 37);
            if (msd->utilitybase)
            {
                if (_init_mouseclass(msd))
                {
                    return TRUE;
                }
            }
            CloseLibrary(msd->oopbase);
        }
        FreeMem(msd, sizeof (struct mouse_staticdata));
    }
    return FALSE;
}

