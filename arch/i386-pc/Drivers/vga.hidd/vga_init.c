/*
    (C) 1999 AROS - The Amiga Research OS
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

#include "vga.h"
#include "vgaclass.h"

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		vgaHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct vgabase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
};

extern struct vgaModeDesc vgaDefMode[];

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

#undef SysBase
#undef OOPBase


#define OOPBase xsd->oopbase

extern AttrBase HiddPixFmtAttrBase;	// = 0;

static struct ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

static BOOL initclasses(struct vga_staticdata *xsd)
{

    /* Get some attrbases */
    
    if (!ObtainAttrBases(abd))
    	goto failure;

    xsd->vgaclass = init_vgaclass(xsd);
    if (NULL == xsd->vgaclass)
    	goto failure;

    xsd->onbmclass = init_onbmclass(xsd);
    if (NULL == xsd->onbmclass)
    	goto failure;

    xsd->offbmclass = init_offbmclass(xsd);
    if (NULL == xsd->offbmclass)
    	goto failure;

    xsd->mouseclass = init_mouseclass(xsd);
    if (NULL == xsd->mouseclass)
    	goto failure;

    return TRUE;
        
failure:
    freeclasses(xsd);

    return FALSE;
    
}

static VOID freeclasses(struct vga_staticdata *xsd)
{

    if (xsd->mouseclass)
    	free_mouseclass(xsd);

    if (xsd->vgaclass)
    	free_vgaclass(xsd);

    if (xsd->offbmclass)
    	free_offbmclass(xsd);

    if (xsd->onbmclass)
    	free_onbmclass(xsd);

    ReleaseAttrBases(abd);
	
    return;
}


ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct vga_staticdata *xsd;
    struct vgaModeEntry *entry;
    int i;
    xsd = AllocMem( sizeof (struct vga_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (xsd)
    {
        xsd->sysbase = SysBase;
	
	InitSemaphore(&xsd->sema);
	InitSemaphore(&xsd->HW_acc);
	NEWLIST(&xsd->modelist);

	/* Insert default videomodes */
	
	for (i=0; i<NUM_MODES; i++)
	{
	    entry = AllocMem(sizeof(struct vgaModeEntry),MEMF_CLEAR|MEMF_PUBLIC);
	    if (entry)
	    {
		entry->Desc=&(vgaDefMode[i]);
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
	if (entry) FreeMem(entry, sizeof(struct vgaModeEntry));
	FreeMem(xsd, sizeof (struct vga_staticdata));
    }
    return FALSE;
}
