/*
    Copyright  1995-2008, The AROS Development Team. All rights reserved.
    $Id: gdi_init.c 27757 2008-01-26 15:05:40Z verhaegs $

    Desc: GDI hidd initialization code.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <utility/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdi.h"

/****************************************************************************************/

#undef XSD

/****************************************************************************************/

static BOOL initclasses( struct gdi_staticdata *xsd );
static VOID freeclasses( struct gdi_staticdata *xsd );

/****************************************************************************************/

static OOP_AttrBase HiddPixFmtAttrBase;

static struct OOP_ABDescr abd[] =
{
    { IID_Hidd_PixFmt   , &HiddPixFmtAttrBase   },
    { NULL     	    	, NULL	    	    	}
};

/****************************************************************************************/

static BOOL initclasses(struct gdi_staticdata *xsd)
{
    /* Get some attrbases */
    
    if (!OOP_ObtainAttrBases(abd))
    	goto failure;

    return TRUE;
        
failure:
    freeclasses(xsd);

    return FALSE; 
}

/****************************************************************************************/

static VOID freeclasses(struct gdi_staticdata *xsd)
{
    OOP_ReleaseAttrBases(abd);
}

/****************************************************************************************/

static int GDI_Init(LIBBASETYPEPTR LIBBASE)
{
    struct Task 	    	*gditask;
    struct gdi_staticdata *xsd = &LIBBASE->xsd;

    D(bug("Entering GDI_Init\n"));
    
    InitSemaphore( &xsd->sema );
    InitSemaphore( &xsd->gdisema );
		
    /* Do not need to singlethread this
     * since no other tasks are using GDI currently
     */

    xsd->display = GDICALL(CreateDC, "DISPLAY", NULL, NULL, NULL);
    if (xsd->display) {
/*
        xsd->delete_win_atom         = XCALL(XInternAtom, xsd->display, "WM_DELETE_WINDOW", FALSE);
        xsd->clipboard_atom          = XCALL(XInternAtom, xsd->display, "CLIPBOARD", FALSE);
        xsd->clipboard_property_atom = XCALL(XInternAtom, xsd->display, "AROS_HOSTCLIP", FALSE);
        xsd->clipboard_incr_atom     = XCALL(XInternAtom, xsd->display, "INCR", FALSE);
        xsd->clipboard_targets_atom  = XCALL(XInternAtom, xsd->display, "TARGETS", FALSE);
*/
	if (NATIVECALL(GDI_Init)) {
	    if (initclasses(xsd))
	    {
	        D(bug("GDI_Init succeeded\n"));
	        return TRUE;
	    }
	    NATIVECALL(GDI_PutMsg, NULL, WM_QUIT, 0, 0);
        }
    }
    
    D(bug("GDI_Init failed\n"));
    
    return FALSE;
}

/****************************************************************************************/

ADD2OPENLIB(GDI_Init, 0);
