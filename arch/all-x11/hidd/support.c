/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include <X11/Xlib.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include "x11gfx_intern.h"

/* Support functions */
ULONG map_x11_to_hidd(long *penarray, ULONG x11pixel)
{
    ULONG hidd_pen = 0;
    BOOL pix_found = FALSE;
    for (hidd_pen = 0; hidd_pen < 256; hidd_pen ++)
    {
    	if (x11pixel == penarray[hidd_pen])
	{
	    pix_found = TRUE;
	    break;
	}
    }
    if (!pix_found)
    	hidd_pen = 0UL;
	
    return hidd_pen;	
    
}

VOID releaseattrbases(struct abdescr *abd, struct Library * OOPBase)
{
    for (; abd->interfaceid; abd ++)
    {
        if ( *(abd->attrbase) != 0 )
	{
	    ReleaseAttrBase(abd->interfaceid);
	    *(abd->attrbase) = 0;
	}
    }
    return;
}

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase)
{
    struct abdescr *d;
    for (d = abd; d->interfaceid; d ++)
    {
        *(d->attrbase) = ObtainAttrBase(abd->interfaceid);
	if ( *(d->attrbase) == 0 )
	{
	    releaseattrbases(abd, OOPBase);
	    return FALSE;
	}   
    }
    return TRUE;
    
}

/* Creates an XImage AND allocates bitmap */

XImage *alloc_ximage(Display *display, int screen, ULONG width, UBYTE depth, UBYTE height)
{
    XImage *image;
    image = XCreateImage( display
		, DefaultVisual(display, screen)
		, depth
		, ZPixmap
		, 0	/* Offset	*/
		, NULL	/* Data		*/
		, width
		, height
		, 16
		, 0
    );
	    
    if (image)
    {
        ULONG size;
	
	size = ((width - 1) >> 3) + 1; /* bytes per row */
	size = size * height * depth;
	
        image->data = AllocVec(size, MEMF_ANY);
	if (image->data)
	{
	    return image;
	}
	XFree(image);
    }
    return NULL;
    
}

VOID free_ximage(XImage *image)
{
    FreeVec(image->data);
    image->data = NULL;
    XFree(image);
}
