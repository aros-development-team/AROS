/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vga gfx Hidd for standalone i386 AROS
    Lang: english
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "vga.h"
#include "vgaclass.h"

#include LC_LIBDEFS_FILE

extern struct vgaModeDesc vgaDefMode[];

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

AROS_SET_LIBFUNC(PCVGA_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct vga_staticdata *xsd = &LIBBASE->vsd;
    struct vgaModeEntry *entry;
    int i;
	
    InitSemaphore(&xsd->sema);
    InitSemaphore(&xsd->HW_acc);
    NEWLIST(&xsd->modelist);

    if (!OOP_ObtainAttrBases(abd))
	return FALSE;
    
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

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(PCVGA_Init, 0)
