/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd initialization code.
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

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "x11.h"
#include "fullscreen.h"

/****************************************************************************************/

#undef XSD

/****************************************************************************************/

static BOOL initclasses( struct x11_staticdata *xsd );
static VOID freeclasses( struct x11_staticdata *xsd );
struct Task *create_x11task( struct x11task_params *params, struct ExecBase *ExecBase);
VOID x11task_entry(struct x11task_params *xtp);

/****************************************************************************************/

static OOP_AttrBase HiddPixFmtAttrBase;

static struct OOP_ABDescr abd[] =
{
    { IID_Hidd_PixFmt   , &HiddPixFmtAttrBase   },
    { NULL     	    	, NULL	    	    	}
};

/****************************************************************************************/

static BOOL initclasses(struct x11_staticdata *xsd)
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

static VOID freeclasses(struct x11_staticdata *xsd)
{
    OOP_ReleaseAttrBases(abd);
}

/****************************************************************************************/

static int MyErrorHandler (Display * display, XErrorEvent * errevent)
{
    char buffer[256];

    XGetErrorText (display, errevent->error_code, buffer, sizeof (buffer));

    fprintf(stderr,
    	    "XError %d (Major=%d, Minor=%d) task = %s\n%s\n",
	    errevent->error_code,
	    errevent->request_code,
	    errevent->minor_code,
	    FindTask(0)->tc_Node.ln_Name,
	    buffer);
	     
    fflush (stderr);

    return 0;
}

/****************************************************************************************/

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    // *((ULONG *)0) = 0;

    return 0;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(X11_Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct x11_staticdata *xsd = &LIBBASE->xsd;
    STRPTR displayname;

    D(bug("Entering X11_Init\n"));
    
    InitSemaphore( &xsd->sema );
    InitSemaphore( &xsd->x11sema );
		
    /* Try to get the display */
    if (!(displayname = (STRPTR)getenv("DISPLAY")))
	displayname =":0.0";

    if ((strncmp(displayname, ":", 1) == 0) ||
	(strncmp(displayname, "unix:", 5) == 0))
    {
	xsd->local_display = TRUE;
    }
		    
		    
    /* Do not need to singlethead this
     * since no other tasks are using X currently
     */

    xsd->display = XOpenDisplay(displayname);
    if (xsd->display)
    {
	struct x11task_params 	 xtp;
	struct Task 	    	*x11task;

	XSetErrorHandler (MyErrorHandler);
	XSetIOErrorHandler (MySysErrorHandler);

	if (getenv("AROS_X11_FULLSCREEN"))
	{
	    xsd->fullscreen = x11_fullscreen_supported(xsd->display);
	}
	
	xsd->delete_win_atom         = XInternAtom(xsd->display, "WM_DELETE_WINDOW", FALSE);
	xsd->clipboard_atom          = XInternAtom(xsd->display, "CLIPBOARD", FALSE);
	xsd->clipboard_property_atom = XInternAtom(xsd->display, "AROS_HOSTCLIP", FALSE);
	xsd->clipboard_incr_atom     = XInternAtom(xsd->display, "INCR", FALSE);
	xsd->clipboard_targets_atom  = XInternAtom(xsd->display, "TARGETS", FALSE);
	
	xtp.parent = FindTask(NULL);
	xtp.ok_signal	= SIGBREAKF_CTRL_E;
	xtp.fail_signal	= SIGBREAKF_CTRL_F;
	xtp.kill_signal	= SIGBREAKF_CTRL_C;
	xtp.xsd		= xsd;

	if ((x11task = create_x11task(&xtp, SysBase)))
	{			
	    if (initclasses(xsd))
	    {
		D(bug("X11_Init succeeded\n"));
		return TRUE;
	    }
	    
	    Signal(x11task, xtp.kill_signal);
	}

	XCloseDisplay(xsd->display);

    }
    
    D(bug("X11_Init failed\n"));
    
    return FALSE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2OPENLIB(X11_Init, 0);
