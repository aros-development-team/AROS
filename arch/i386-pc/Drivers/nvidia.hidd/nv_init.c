/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vga gfx Hidd for standalone i386 AROS
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include <hidd/pci.h>

#include "nv.h"
//#include "vgaclass.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		nvHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB

#define NOEXPUNGE

struct nvbase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef SysBase
#undef OOPBase

#define OOPBase (nsd->oopbase)

int initclasses(struct nv_staticdata *nsd)
{
//	nsd->nvclass = init_nvclass(nsd);
	if (nsd->nvclass == NULL)
		goto failure;
	
	return TRUE;
	
failure:
	return FALSE;
}

int findCard(struct nv_staticdata *nsd)
{
	nsd->card = NULL;



	return (nsd->card) ? TRUE : FALSE;
}

#undef SysBase
#define SysBase      (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
	struct nv_staticdata *nsd;
	nsd = AllocMem( sizeof (struct nv_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
	if (nsd)
	{
		nsd->sysbase = SysBase;
	
		nsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
		if (nsd->oopbase)
		{
			nsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
			if (nsd->utilitybase)
			{
				if (initclasses(nsd))
				{
					D(bug("Everything OK\n"));
					return TRUE;
				}
				CloseLibrary(nsd->utilitybase);
			}
			CloseLibrary(nsd->oopbase);
		}
		FreeMem(nsd, sizeof (struct nv_staticdata));
	}
	return FALSE;
}

