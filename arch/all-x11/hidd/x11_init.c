/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: X11 hidd initialization code.
    Lang: English.
*/
#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>

#include <proto/exec.h>

#include <utility/utility.h>

#include "x11.h"

#warning FIXME: define NT_HIDD in libraries.h or something else
#define NT_HIDD NT_LIBRARY

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
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

struct x11clbase
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

#define SysBase      (LC_SYSBASE_FIELD(lh))

#undef XSD(cl) xsd
#undef SysBase


static BOOL initclasses( struct x11_staticdata *xsd );
static VOID freeclasses( struct x11_staticdata *xsd );
struct Task *create_x11task( struct x11task_params *params, struct ExecBase *ExecBase);
VOID x11task_entry(struct x11task_params *xtp);

static BOOL initclasses(struct x11_staticdata *xsd)
{

    xsd->x11class = init_x11class(xsd);
    if (NULL == xsd->x11class)
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

    if (xsd->x11class)
    	free_x11class(xsd);
	
    return;
}

static int MyErrorHandler (Display * display, XErrorEvent * errevent)
{
    char buffer[256];

    XGetErrorText (display, errevent->error_code, buffer, sizeof (buffer));
    fprintf (stderr
	, "XError %d (Major=%d, Minor=%d)\n%s\n"
	, errevent->error_code
	, errevent->request_code
	, errevent->minor_code
	, buffer
    );
    fflush (stderr);

    return 0;
}

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    return 0;
}


ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct x11_staticdata *xsd;
    xsd = AllocMem( sizeof (struct x11_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (xsd)
    {
        xsd->sysbase = SysBase;
	
	NEWLIST( &xsd->xwindowlist );
	InitSemaphore( &xsd->winlistsema );
	InitSemaphore( &xsd->sema );
	InitSemaphore( &xsd->x11sema );
	
	
        xsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (xsd->oopbase)
	{
	    xsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
	    if (xsd->utilitybase)
	    {
	        STRPTR displayname;
	    
		/* Try to get the display */
		if (!(displayname = getenv("DISPLAY")))
		    displayname =":0.0";
		    
		/* Do not need to singlethead this
		   since no other tasks are using X currently
		*/
		   		    
    		xsd->display = XOpenDisplay(displayname);
		if (xsd->display)
		{
		   D(bug("x11_init: got display\n"));

		    XSetErrorHandler (MyErrorHandler);
		    XSetIOErrorHandler (MySysErrorHandler);
		    D(bug("error handlers set\n"));
		    
		    if (initclasses(xsd))
		    {
			/* The X11 task should be the last one up.
			   (It is difficult to clean up after it
			   otherwise, if something should fail) */
			       
			struct x11task_params xtp;
			    
		   	D(bug("x11_init: got classes\n"));
			
			xtp.parent = FindTask(NULL);
			xtp.ok_signal   = SIGBREAKF_CTRL_E;
			xtp.fail_signal = SIGBREAKF_CTRL_F;
			xtp.xsd	    = xsd;
			    
			if (create_x11task(&xtp, SysBase))
			{
			    D(bug("x11_init: Task up& running\n"));
			    return TRUE;
			}
			freeclasses(xsd);
			
		    }
		    
		    XCloseDisplay(xsd->display);
		
		}
		
		CloseLibrary(xsd->utilitybase);
	    }
	    CloseLibrary(xsd->oopbase);
	}
	FreeMem(xsd, sizeof (struct x11_staticdata));
    }

    return FALSE;
        
}



