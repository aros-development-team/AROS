/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: display Hidd for standalone palm AROS
    Lang: english
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "display.h"
#include "displayclass.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		displayHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct displaybase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
};

extern struct DisplayModeDesc DisplayDefMode[];

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

//#undef SysBase
#undef OOPBase


#define OOPBase xsd->oopbase

//static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;
#define AROS_CREATE_ROM_BUG 1

#ifndef AROS_CREATE_ROM_BUG 
static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};
#endif

static VOID freeclasses(struct display_staticdata *xsd);

static BOOL initclasses(struct display_staticdata *xsd)
{

    /* Get some attrbases */
    __IHidd_PixFmt = OOP_ObtainAttrBase(IID_Hidd_PixFmt);

#ifndef AROS_CREATE_ROM_BUG
    if (!OOP_ObtainAttrBases(abd))
    	goto failure;
#endif

    xsd->displayclass = init_displayclass(xsd);
    if (NULL == xsd->displayclass)
    	goto failure;

    xsd->onbmclass = init_onbmclass(xsd);
    if (NULL == xsd->onbmclass)
    	goto failure;

    xsd->offbmclass = init_offbmclass(xsd);
    if (NULL == xsd->offbmclass)
    	goto failure;

    return TRUE;
        
failure:
    freeclasses(xsd);

    return FALSE;
    
}

static VOID freeclasses(struct display_staticdata *xsd)
{
    if (xsd->displayclass)
    	free_displayclass(xsd);

    if (xsd->offbmclass)
    	free_offbmclass(xsd);

    if (xsd->onbmclass)
    	free_onbmclass(xsd);

#ifndef AROS_CREATE_ROM_BUG
    OOP_ReleaseAttrBases(abd);
#endif
	
    return;
}


ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct display_staticdata *xsd;
    struct displayModeEntry *entry;
    int i;

    xsd = AllocMem( sizeof (struct display_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (xsd)
    {
        xsd->sysbase = SysBase;
        xsd->displaybase = lh;
	
	InitSemaphore(&xsd->sema);
	InitSemaphore(&xsd->HW_acc);
	NEWLIST(&xsd->modelist);

	/* Insert default videomodes */
	
	for (i=0; i<NUM_MODES; i++)
	{
	    entry = AllocMem(sizeof(struct DisplayModeEntry),MEMF_CLEAR|MEMF_PUBLIC);
	    if (entry)
	    {
//		entry->Desc=&(DisplayDefMode[i]);
		ADDHEAD(&xsd->modelist,entry);
		D(bug("Added default mode: %s\n", entry->Desc->name));
	    }
	}
	
        xsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (xsd->oopbase)

	{
	    xsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
	    if (xsd->utilitybase)
	    {
		if (initclasses(xsd))
		{
		    D(bug("Everything OK\n"));
		    return TRUE;
		}
		CloseLibrary(xsd->utilitybase);
	    }
	    CloseLibrary(xsd->oopbase);
	}
	if (entry) FreeMem(entry, sizeof(struct DisplayModeEntry));
	FreeMem(xsd, sizeof (struct display_staticdata));
    }
    return FALSE;
}
