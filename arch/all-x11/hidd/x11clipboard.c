/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd. Connects to the X server and receives events.
    Lang: English.
*/


#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#define size_t aros_size_t
#include <hidd/unixio.h>
#include <hidd/hidd.h>

#include <oop/ifmeta.h>

#include <dos/dos.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <hardware/intbits.h>
#include <utility/utility.h>

#include <aros/asmcall.h>
#undef size_t

#define timeval sys_timeval
#include <sys/types.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#undef timeval

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "x11.h"
#include "x11gfx_intern.h"

#define DEBUG 1
#include <aros/debug.h>

#undef SysBase
#undef XSD
#define XSD(cl) xsd

/****************************************************************************************/

#define REQ_CMD(msg) 	    ((msg)->mn_Node.ln_Name)
#define REQ_RETVAL(msg)     ((msg)->mn_Node.ln_Name)
#define REQ_SUCCESS(msg)    ((msg)->mn_Node.ln_Pri)

/****************************************************************************************/

STATIC VOID listen_for_xevent(struct x11_staticdata *xsd, long event, BOOL yesno)
{
    XWindowAttributes xwa;

    LX11
    XGetWindowAttributes(xsd->display,
			 xsd->dummy_window_for_creating_pixmaps,
			 &xwa);
    if (yesno)
    {
    	event = xwa.your_event_mask | event;
    }
    else
    {
    	event = xwa.your_event_mask &~ event;
    }
    
    XSelectInput(xsd->display, xsd->dummy_window_for_creating_pixmaps, event);
    UX11
}

/****************************************************************************************/

STATIC VOID reply_async_request(struct x11_staticdata *xsd, void *primary_retval, int success)
{
    xsd->hostclipboardstate = HOSTCLIPBOARDSTATE_IDLE;
    REQ_SUCCESS(xsd->hostclipboardmsg) = success;
    REQ_RETVAL(xsd->hostclipboardmsg) = (char *)primary_retval;
    ReplyMsg(xsd->hostclipboardmsg);
    
    xsd->hostclipboardmsg = NULL;
    
    /* In case messages were sent, while an async request was in progress */
    Signal(xsd->hostclipboardmp->mp_SigTask, 1L << xsd->hostclipboardmp->mp_SigBit);    
}

/****************************************************************************************/

ULONG x11clipboard_init(struct x11_staticdata *xsd)
{
    ULONG hostclipboardmask = 0;
    
    xsd->hostclipboardmp = CreateMsgPort();
    if (xsd->hostclipboardmp)
    {
    	xsd->hostclipboardmp->mp_Node.ln_Name = "HOST_CLIPBOARD";	
    	hostclipboardmask = 1L << xsd->hostclipboardmp->mp_SigBit;
	
	AddPort(xsd->hostclipboardmp);
    }
    
    return hostclipboardmask;
}

/****************************************************************************************/

VOID x11clipboard_handle_commands(struct x11_staticdata *xsd)
{
    struct Message *msg;
    
    if (xsd->hostclipboardmsg) return;
    
    D(bug("X11CLIPBOARD: handle_commands\n"));
    while((msg = GetMsg(xsd->hostclipboardmp)))
    {
	char *cmd = REQ_CMD(msg);
	BOOL  async = FALSE;

	REQ_SUCCESS(msg) = FALSE;

	if (strcmp(cmd, "READ") == 0)
	{
    	    D(bug("X11CLIPBOARD: handle_commands: READ\n"));

	    if ((xsd->hostclipboardstate == HOSTCLIPBOARDSTATE_IDLE))
	    {
		LX11
		XConvertSelection(xsd->display,
		    	    	  xsd->clipboard_atom,
				  XA_STRING,
				  xsd->clipboard_property_atom,
				  xsd->dummy_window_for_creating_pixmaps,
				  CurrentTime);

		UX11

		xsd->hostclipboardstate = HOSTCLIPBOARDSTATE_READ;
    	    	async = TRUE;
	    }
	}

	if (async)
	{
	    xsd->hostclipboardmsg = msg;
	    break;
	}

	ReplyMsg(msg);
    }    
}

/****************************************************************************************/

BOOL x11clipboard_want_event(XEvent *event)
{
    if ((event->type == SelectionNotify) || (event->type == PropertyNotify))
    	return TRUE;
    
    return FALSE;    
}

/****************************************************************************************/

VOID x11clipboard_handle_event(struct x11_staticdata *xsd, XEvent *event)
{
    switch(event->type)
    {
	case SelectionNotify:
	    D(bug("X11CLIPBOARD: SelectionNotify Event\n"));

	    if (xsd->hostclipboardmsg && (xsd->hostclipboardstate == HOSTCLIPBOARDSTATE_READ))
	    {
		Atom    	    actual_type;
		int     	    actual_format;
		unsigned long   nitems;
		unsigned long   bytes_after;
		unsigned char  *buffer, *arosbuffer;

		LX11
		XGetWindowProperty(xsd->display,
				   xsd->dummy_window_for_creating_pixmaps,
				   xsd->clipboard_property_atom,
				   0,
				   0,
				   False,
				   AnyPropertyType,
				   &actual_type,
				   &actual_format,
				   &nitems,
				   &bytes_after,
				   &buffer);
		XFree(buffer);
		UX11

		if (actual_type == xsd->clipboard_incr_atom)
		{
		    LX11
		    listen_for_xevent(xsd, PropertyChangeMask, TRUE);
		    XDeleteProperty(xsd->display,
				    xsd->dummy_window_for_creating_pixmaps,
				    xsd->clipboard_property_atom);
    	    	    XFlush(xsd->display);
		    UX11;

		    xsd->hostclipboardstate = HOSTCLIPBOARDSTATE_READ_INCR;

    	    	    D(bug("X11CLIPBOARD: SelectionNotify - INCR protocol\n"));
		    break;
		}       

		if (actual_format != 8)
		{
    	    	    D(bug("X11CLIPBOARD: SelectionNotify - format is <> 8, so terminating READ request with error.\n"));

    	    	    reply_async_request(xsd, NULL, FALSE);
		    break;				
		}

		LX11
		XGetWindowProperty(xsd->display,
				   xsd->dummy_window_for_creating_pixmaps,
				   xsd->clipboard_property_atom,
				   0,
				   bytes_after,
				   False,
				   AnyPropertyType,
				   &actual_type,
				   &actual_format,
				   &nitems,
				   &bytes_after,
				   &buffer);

		XDeleteProperty(xsd->display,
				xsd->dummy_window_for_creating_pixmaps,
				xsd->clipboard_property_atom);

		arosbuffer = AllocVec(nitems + 1, MEMF_ANY);
		if (arosbuffer)
		{
		    memcpy(arosbuffer, buffer, nitems);
		    arosbuffer[nitems] = '\0';
		}
    		XFree(buffer);
		UX11;

    	    	D(bug("X11CLIPBOARD: SelectionNotify - terminating READ request with %s\n", arosbuffer ? "success" : "failure"));

    	    	reply_async_request(xsd, arosbuffer, arosbuffer ? TRUE : FALSE);
		
	    } /* if (xsd->hostclipboardmsg && (xsd->hostclipboardstate == HOSTCLIPBOARDSTATE_READ))*/
	    break;

	case PropertyNotify:
	    D(bug("X11CLIPBOARD: PropertyNotify Event\n"));

	    if (xsd->hostclipboardmsg && (xsd->hostclipboardstate == HOSTCLIPBOARDSTATE_READ_INCR))
	    {
		if ((event->xproperty.atom == xsd->clipboard_property_atom) &&
		    (event->xproperty.state == PropertyNewValue))
		{
		    Atom    	    actual_type;
		    int     	    actual_format;
		    unsigned long   nitems;
		    unsigned long   bytes_after;
		    unsigned char  *buffer, *arosbuffer;

    	    	    D(bug("X11CLIPBOARD: PropertyNotify - property event okay\n"));

		    LX11
		    XGetWindowProperty(xsd->display,
			    	       xsd->dummy_window_for_creating_pixmaps,
				       xsd->clipboard_property_atom,
				       0,
				       0,
				       False,
				       AnyPropertyType,
				       &actual_type,
				       &actual_format,
				       &nitems,
				       &bytes_after,
				       &buffer);
		    XFree(buffer);
		    UX11

		    if (actual_format == 8)
		    {
    	    	    	D(bug("X11CLIPBOARD: PropertyNotify - format(8) okay\n"));
 			if (bytes_after == 0)
			{
			    D(bug("X11CLIPBOARD: PropertyNotify - last one detected. Terminating READ request with %s\n",
			    	  xsd->hostclipboard_incrbuffer ? "success" : "failure"));

			    reply_async_request(xsd, xsd->hostclipboard_incrbuffer,
			    	    	    	xsd->hostclipboard_incrbuffer ? TRUE : FALSE);

			    xsd->hostclipboard_incrbuffer = NULL;
			    xsd->hostclipboard_incrbuffer_size = 0;
			    
			    listen_for_xevent(xsd, PropertyChangeMask, FALSE);			    
			}
			else
			{
			    LX11
			    XGetWindowProperty(xsd->display,
			    	    	       xsd->dummy_window_for_creating_pixmaps,
					       xsd->clipboard_property_atom,
					       0,
					       bytes_after,
					       False,
					       AnyPropertyType,
					       &actual_type,
					       &actual_format,
					       &nitems,
					       &bytes_after,
					       &buffer);

			    if (!xsd->hostclipboard_incrbuffer)
			    {
				/* No buffer allocated yet. */

			    	D(bug("X11CLIPBOARD: PropertyNotify - First INCR packet of size %d\n", nitems));
				xsd->hostclipboard_incrbuffer = AllocVec(nitems + 1, MEMF_ANY);
				if (xsd->hostclipboard_incrbuffer)
				{
			    	    memcpy(xsd->hostclipboard_incrbuffer, buffer, nitems);
				    xsd->hostclipboard_incrbuffer[nitems] = '\0';
				    xsd->hostclipboard_incrbuffer_size = nitems;						
				}
				else
				{
    	    	    	    	    D(bug("X11CLIPBOARD: PropertyNotify - Allocation of incrbuffer failed! Terminating READ request with failure\n"));
				    
				    reply_async_request(xsd, NULL, FALSE);
				    
				    listen_for_xevent(xsd, PropertyChangeMask, FALSE);				    	
				}
			    }
			    else
			    {
				/* Buffer already allocated. Do a re-allocation! */

    	    	    	    	D(bug("X11CLIPBOARD: PropertyNotify - One more INCR packet of size %d. Total size now %d\n",
				      nitems, xsd->hostclipboard_incrbuffer_size + nitems));
				
				arosbuffer = AllocVec(xsd->hostclipboard_incrbuffer_size + nitems + 1, MEMF_ANY);
				if (arosbuffer)
				{
				    memcpy(arosbuffer, xsd->hostclipboard_incrbuffer, xsd->hostclipboard_incrbuffer_size);
				    FreeVec(xsd->hostclipboard_incrbuffer);
				    xsd->hostclipboard_incrbuffer = arosbuffer;

				    memcpy(xsd->hostclipboard_incrbuffer + xsd->hostclipboard_incrbuffer_size,
					   buffer,
					   nitems);

				    xsd->hostclipboard_incrbuffer_size += nitems;
				    xsd->hostclipboard_incrbuffer[xsd->hostclipboard_incrbuffer_size] = '\0';
				}
				else
				{
    	    	    	    	    D(bug("X11CLIPBOARD: PropertyNotify - Reallocation of incrbuffer failed! Terminating READ request with failure\n"));

				    FreeVec(xsd->hostclipboard_incrbuffer);
				    
				    reply_async_request(xsd, NULL, FALSE);

				    xsd->hostclipboard_incrbuffer = NULL;
				    xsd->hostclipboard_incrbuffer_size = 0;
				    
				    listen_for_xevent(xsd, PropertyChangeMask, FALSE);
				}
				
			    } /* if (!xsd->hostclipboard_incrbuffer) else ... */

			    XFree(buffer);
			    UX11

			} /* if (bytes_after == 0) else ... */
			
		    } /* if (actual_format == 8) */

		    LX11
		    XDeleteProperty(xsd->display,
				    xsd->dummy_window_for_creating_pixmaps,
				    xsd->clipboard_property_atom);
		    XFlush(xsd->display);
		    UX11
		    
		} /* if it's right property and the property has new value */
		
	    } /* if (xsd->hostclipboardmsg && (xsd->hostclipboardstate == HOSTCLIPBOARDSTATE_READ_INCR)) */

	    break;
	    
    } /* switch(event->type) */

}

/****************************************************************************************/

