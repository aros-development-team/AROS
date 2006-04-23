/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd. Host clipboard support.
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

#undef XSD
#define XSD(cl) xsd

/****************************************************************************************/

#define REQ_CMD(msg) 	    ((msg)->mn_Node.ln_Pri)
#define REQ_PARAM(msg)	    ((msg)->mn_Node.ln_Name)

#define REQ_RETVAL(msg)     ((msg)->mn_Node.ln_Name)
#define REQ_SUCCESS(msg)    ((msg)->mn_Node.ln_Pri)

/****************************************************************************************/

STATIC VOID listen_for_xevent(struct x11_staticdata *xsd, long event, BOOL yesno)
{
    XWindowAttributes xwa;

    LOCK_X11
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
    UNLOCK_X11
}

/****************************************************************************************/

STATIC VOID reply_async_request(struct x11_staticdata *xsd, void *primary_retval, int success)
{
    xsd->hostclipboard_readstate = HOSTCLIPBOARDSTATE_IDLE;
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
	char cmd = REQ_CMD(msg);
	BOOL  async = FALSE;

	REQ_SUCCESS(msg) = FALSE;

	if (cmd == 'R')
	{
    	    D(bug("X11CLIPBOARD: handle_commands - READ\n"));

	    if ((xsd->hostclipboard_readstate == HOSTCLIPBOARDSTATE_IDLE))
	    {
		LOCK_X11
		XConvertSelection(xsd->display,
		    	    	  xsd->clipboard_atom,
				  XA_STRING,
				  xsd->clipboard_property_atom,
				  xsd->dummy_window_for_creating_pixmaps,
				  CurrentTime);

		UNLOCK_X11

		xsd->hostclipboard_readstate = HOSTCLIPBOARDSTATE_READ;
    	    	async = TRUE;
	    }
	}
    	else if (cmd == 'W')
	{
	    unsigned char *srcbuffer = (unsigned char *)REQ_PARAM(msg);
	    ULONG   	   size = strlen(srcbuffer);
	    unsigned char *newbuffer;

    	    D(bug("X11CLIPBOARD: handle_commands: WRITE\n"));
	    
	    newbuffer = AllocVec(size, MEMF_ANY);
	    if (newbuffer)
	    {
	    	memcpy(newbuffer, srcbuffer, size);
		
		if (xsd->hostclipboard_writebuffer)
		{
	    	    FreeVec(xsd->hostclipboard_writebuffer);
		}
		
	    	xsd->hostclipboard_writebuffer = newbuffer;
		xsd->hostclipboard_writebuffer_size = size;
		
		LOCK_X11
		XSetSelectionOwner(xsd->display, xsd->clipboard_atom,
		    	    	   xsd->dummy_window_for_creating_pixmaps, xsd->x_time);
		UNLOCK_X11
		
		REQ_SUCCESS(msg) = TRUE;
		REQ_RETVAL(msg) = NULL;
	    }
	    else
	    {
	    	REQ_SUCCESS(msg) = FALSE;
		REQ_RETVAL(msg) = NULL;
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
    if ((event->type == SelectionNotify) ||
    	(event->type == PropertyNotify) ||
	(event->type == SelectionRequest))
    {
    	return TRUE;
    }
    
    return FALSE;    
}

/****************************************************************************************/

VOID x11clipboard_handle_event(struct x11_staticdata *xsd, XEvent *event)
{
    switch(event->type)
    {
	case SelectionNotify:
	    D(bug("X11CLIPBOARD: SelectionNotify Event\n"));

	    if (xsd->hostclipboardmsg && (xsd->hostclipboard_readstate == HOSTCLIPBOARDSTATE_READ))
	    {
		Atom    	    actual_type;
		int     	    actual_format;
		unsigned long   nitems;
		unsigned long   bytes_after;
		unsigned char  *buffer, *arosbuffer;

		LOCK_X11
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
		UNLOCK_X11

		if (actual_type == xsd->clipboard_incr_atom)
		{
		    LOCK_X11
		    listen_for_xevent(xsd, PropertyChangeMask, TRUE);
		    XDeleteProperty(xsd->display,
				    xsd->dummy_window_for_creating_pixmaps,
				    xsd->clipboard_property_atom);
    	    	    XFlush(xsd->display);
		    UNLOCK_X11;

		    xsd->hostclipboard_readstate = HOSTCLIPBOARDSTATE_READ_INCR;

    	    	    D(bug("X11CLIPBOARD: SelectionNotify - INCR protocol\n"));
		    break;
		}       

		if (actual_format != 8)
		{
    	    	    D(bug("X11CLIPBOARD: SelectionNotify - format is <> 8, so terminating READ request with error.\n"));

    	    	    reply_async_request(xsd, NULL, FALSE);
		    break;				
		}

		LOCK_X11
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
		UNLOCK_X11;

    	    	D(bug("X11CLIPBOARD: SelectionNotify - terminating READ request with %s\n", arosbuffer ? "success" : "failure"));

    	    	reply_async_request(xsd, arosbuffer, arosbuffer ? TRUE : FALSE);
		
	    } /* if (xsd->hostclipboardmsg && (xsd->hostclipboard_readstate == HOSTCLIPBOARDSTATE_READ))*/
	    break;

	case PropertyNotify:
	    D(bug("X11CLIPBOARD: PropertyNotify Event\n"));

	    if (xsd->hostclipboardmsg && (xsd->hostclipboard_readstate == HOSTCLIPBOARDSTATE_READ_INCR))
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

		    LOCK_X11
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
		    UNLOCK_X11

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
			    LOCK_X11
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
			    UNLOCK_X11

			} /* if (bytes_after == 0) else ... */
			
		    } /* if (actual_format == 8) */

		    LOCK_X11
		    XDeleteProperty(xsd->display,
				    xsd->dummy_window_for_creating_pixmaps,
				    xsd->clipboard_property_atom);
		    XFlush(xsd->display);
		    UNLOCK_X11
		    
		} /* if it's right property and the property has new value */
		
	    } /* if (xsd->hostclipboardmsg && (xsd->hostclipboard_readstate == HOSTCLIPBOARDSTATE_READ_INCR)) */

	    break;

	case SelectionRequest:
	    D(bug("X11CLIPBOARD: SelectionRequest Event\n"));

	    if (xsd->hostclipboard_writebuffer)
	    {
	    	XEvent e;

	    	D(bug("X11CLIPBOARD: SelectionRequest Event - state okay\n"));

		e.xselection.type   	= SelectionNotify;
		e.xselection.display 	= event->xselectionrequest.display;
		e.xselection.requestor  = event->xselectionrequest.requestor;
		e.xselection.selection  = event->xselectionrequest.selection;
		e.xselection.target 	= event->xselectionrequest.target;
		e.xselection.property 	= event->xselectionrequest.property;
		e.xselection.time   	= event->xselectionrequest.time;
		
		xsd->hostclipboard_writerequest_window = event->xselectionrequest.requestor;
		xsd->hostclipboard_writerequest_property = event->xselectionrequest.property;

		if (event->xselectionrequest.target == xsd->clipboard_targets_atom)
		{
		    long supported_targets[] = {XA_STRING};

		    LOCK_X11
		    XChangeProperty(xsd->display,
		    	    	    xsd->hostclipboard_writerequest_window,
				    xsd->hostclipboard_writerequest_property,
				    xsd->clipboard_targets_atom,
				    32,
				    PropModeReplace,
				    (unsigned char *)supported_targets,
				    sizeof(supported_targets) / sizeof(supported_targets[0]));
		    UNLOCK_X11
		    
		}
		else if (event->xselectionrequest.target == XA_STRING)
		{

		    LOCK_X11
		    xsd->hostclipboard_write_chunks = XMaxRequestSize(xsd->display) * 4 / 2;
		    
		    /* FIXME TODO: Use INCR protocol for large requests */
		    if (1)// && xsd->hostclipboard_writebuffer_size <= xsd->hostclipboard_write_chunks)
		    {
	    		D(bug("X11CLIPBOARD: SelectionRequest Event - one chunk is enough\n"));

			//D(bug("[%s]\n", xsd->hostclipboard_writebuffer));

			XChangeProperty(xsd->display,
		    	    		xsd->hostclipboard_writerequest_window,
					xsd->hostclipboard_writerequest_property,
					XA_STRING,
					8,
					PropModeReplace,
					(unsigned char *)xsd->hostclipboard_writebuffer,
					(int)xsd->hostclipboard_writebuffer_size);
		    }
		    else
		    {
		    	/* FIXME TODO: Use INCR protocol for large requests */
			
		    	e.xselection.property = None;
		    }
		    UNLOCK_X11
		}
		else
		{
		    e.xselection.property = None;
		}
			
    	    	D(bug("X11CLIPBOARD: SelectionRequest Event - sending SelectionNotify event to clipboard requestor\n"));
					
		LOCK_X11
		XSendEvent(xsd->display,
		    	   xsd->hostclipboard_writerequest_window,
			   False,
			   NoEventMask,
			   &e);
		XFlush(xsd->display);
		UNLOCK_X11
	    }
	    break;
	    
    } /* switch(event->type) */

}

/****************************************************************************************/

