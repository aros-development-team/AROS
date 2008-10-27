/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef X11HIDD_XSHM_H
#define X11HIDD_XSHM_H

#include <aros/config.h>

#if USE_XSHM

#define XSHM_MEMSIZE 500000	/* We allocate 500K for dumping images to X */

void *init_shared_mem(Display *display);

void cleanup_shared_mem(Display *display, void *meminfo);

XImage *create_xshm_ximage(Display *display, Visual *visual, int depth,
    	    	    	   int format, int width, int height, void *xshminfo);

void put_xshm_ximage(Display *display, Drawable d, GC gc, XImage *ximage,
    	    	     int xsrc, int ysrc, int xdest, int ydest,
		     int width, int height, Bool send_event);

void get_xshm_ximage(Display *display, Drawable d, XImage *image, int x, int y);
	
void destroy_xshm_ximage(XImage *image);

#endif /* USE_XSHM */

#endif /* X11HIDD_XSHM_H */
