/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd initialization code.
    Lang: English.
*/

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

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include "x11.h"

#undef SysBase

/****************************************************************************************/

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->seglist)
#define LC_RESIDENTNAME		X11Hidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB

/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

/****************************************************************************************/

struct x11clbase
{
    struct Library   library;
    struct ExecBase *sysbase;
    BPTR	     seglist;
};

#include <libcore/libheader.c>

#undef XSD
#undef SysBase
#undef OOPBase

#define OOPBase     	xsd->oopbase

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

    xsd->gfxclass = init_gfxclass(xsd);
    if (NULL == xsd->gfxclass)
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

    xsd->kbdclass = init_kbdclass(xsd);
    if (NULL == xsd->kbdclass)
    	goto failure;

    return TRUE;
        
failure:
    freeclasses(xsd);

    return FALSE; 
}

/****************************************************************************************/

static VOID freeclasses(struct x11_staticdata *xsd)
{
    if (xsd->kbdclass)
    	free_kbdclass(xsd);

    if (xsd->mouseclass)
    	free_mouseclass(xsd);

    if (xsd->gfxclass)
    	free_gfxclass(xsd);

    if (xsd->offbmclass)
    	free_offbmclass(xsd);

    if (xsd->onbmclass)
    	free_onbmclass(xsd);

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

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct x11_staticdata *xsd;
    
    xsd = AllocMem( sizeof (struct x11_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (xsd)
    {
        xsd->sysbase = SysBase;
	
	InitSemaphore( &xsd->sema );
	InitSemaphore( &xsd->x11sema );
		
        xsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (xsd->oopbase)
	{
	    xsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
	    if (xsd->utilitybase)
	    {
    	    #if X11_LOAD_KEYMAPTABLE
	        xsd->dosbase = OpenLibrary(DOSNAME, 37);
		if (xsd->dosbase)
    	    #endif
		{
	            STRPTR displayname;

		    /* Try to get the display */
		    if (!(displayname = (STRPTR)getenv("DISPLAY")))
			displayname =":0.0";

    	    	    if ((strncmp(displayname, ":", 1) == 0) ||
		        (strncmp(displayname, "unix:", 5) == 0))
		    {
		    	xsd->local_display = TRUE;
		    }
		    
		    /* Do not need to singlethead this
		       since no other tasks are using X currently
		    */

    		    xsd->display = XOpenDisplay(displayname);
		    if (xsd->display)
		    {
    			struct x11task_params 	 xtp;
		        struct Task 	    	*x11task;

			XSetErrorHandler (MyErrorHandler);
			XSetIOErrorHandler (MySysErrorHandler);
			
			xsd->delete_win_atom 	     = XInternAtom(xsd->display, "WM_DELETE_WINDOW", FALSE);
			xsd->clipboard_atom  	     = XInternAtom(xsd->display, "CLIPBOARD", FALSE);
    	    	    	xsd->clipboard_property_atom = XInternAtom(xsd->display, "AROS_HOSTCLIP", FALSE);
    	    	    	xsd->clipboard_incr_atom     = XInternAtom(xsd->display, "INCR", FALSE);
    	    	    	xsd->clipboard_targets_atom  = XInternAtom(xsd->display, "TARGETS", FALSE);
			
    			xtp.parent = FindTask(NULL);
    			xtp.ok_signal	= SIGBREAKF_CTRL_E;
    			xtp.fail_signal = SIGBREAKF_CTRL_F;
			xtp.kill_signal = SIGBREAKF_CTRL_C;
    			xtp.xsd 	= xsd;

    			if ((x11task = create_x11task(&xtp, SysBase)))
    			{			
    		    	    if (initclasses(xsd))
			    {
				return TRUE;
    			    }
			    
			    Signal(x11task, xtp.kill_signal);
			}

			XCloseDisplay(xsd->display);

		    }
    	    	#if X11_LOAD_KEYMAPTABLE
		    CloseLibrary(xsd->dosbase);
    	    	#endif
		}
		CloseLibrary(xsd->utilitybase);
	    }
	    CloseLibrary(xsd->oopbase);
	}
	FreeMem(xsd, sizeof (struct x11_staticdata));
    }

    return FALSE;
        
}

/****************************************************************************************/


