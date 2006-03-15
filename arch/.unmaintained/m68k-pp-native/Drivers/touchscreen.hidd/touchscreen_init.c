/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$ 

    Desc: Touchscreen hidd
    Lang: english
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "touchscreen.h"

#undef SysBase
//#define SysBase (*(struct ExecBase **)4UL)
//#define SysBase      (LC_SYSBASE_FIELD(lh))

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME     	touchscreenHidd_resident
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI          9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct touchscreenbase
{
	struct Library   library;
	struct ExecBase *sysbase;
	BPTR             seglist;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0 
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
	struct mouse_staticdata *tsd;

	D(bug("_touchscreen: Initializing\n"));

	tsd = AllocMem( sizeof (struct mouse_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
	if (tsd) {
		InitSemaphore(&tsd->sema);
		tsd->sysbase = SysBase;
		tsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
		if (tsd->oopbase) {
			tsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
			if (tsd->utilitybase) {
				if (_init_mouseclass(tsd)) {
					D(bug("----------- Successfully initialized mouseclass!\n"));
					return TRUE;
				}
			}
			CloseLibrary(tsd->oopbase);
		}
		FreeMem(tsd, sizeof (struct mouse_staticdata));
	}
	return FALSE;
}
