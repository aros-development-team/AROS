#ifndef X11GFX_INTERN_H
#define X11GFX_INTERN_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <X11/Xlib.h>

#include "x11_class.h"

/****************************************************************************************/

ULONG map_x11_to_hidd(long *penarray, ULONG x11pixel);
XImage *alloc_ximage(Display *display, int screen, ULONG width, UBYTE depth, UBYTE height);
VOID free_ximage(XImage *image);

/****************************************************************************************/

#define USE_X11_DRAWFUNCS   1
#define USE_FRAMEBUFFER     1	/* Only for debug. Do not attempt to set to 0 in production! Managing several screens will break. */
#define X11SOFTMOUSE	    1	/* Use software mouse sprite */
#define ADJUST_XWIN_SIZE    1	/* Resize the xwindow to the size of the actual visible screen */
#ifdef HOST_OS_darwin
/*
 * XQuartz does not like operations on unmapped window, strange effects occur (bootmenu breaks, for example).
 * X11 driver needs serious rewrite. For now i hope this will do.
 */
#define DELAY_XWIN_MAPPING  0
#else
#define DELAY_XWIN_MAPPING  1   /* Do not map (show) X window as long as there's no screen */
#endif

#if ((USE_FRAMEBUFFER && !X11SOFTMOUSE) || (!USE_FRAMEBUFFER && X11SOFTMOUSE))
#error Invalid combination of USE_FRAMEBUFFER and X11SOFTMOUSE
#endif

/****************************************************************************************/

/* Private Attrs and methods for the X11Gfx Hidd */

#define IID_Hidd_X11Gfx     	"hidd.gfx.x11gfx"


#define HiddX11GfxAB  	    	__abHidd_X11Gfx

/* extern OOP_AttrBase HiddX11GfxAB; */

enum
{
    aoHidd_X11Gfx_SysDisplay,
    aoHidd_X11Gfx_SysScreen,
    aoHidd_X11Gfx_Hidd2X11CMap,
    aoHidd_X11Gfx_SysCursor,
    aoHidd_X11Gfx_ColorMap,
    aoHidd_X11Gfx_VisualClass, /* stegerg */
    
    num_Hidd_X11Gfx_Attrs
    
};

#define aHidd_X11Gfx_SysDisplay		(HiddX11GfxAB + aoHidd_X11Gfx_SysDisplay)
#define aHidd_X11Gfx_SysScreen		(HiddX11GfxAB + aoHidd_X11Gfx_SysScreen)
#define aHidd_X11Gfx_Hidd2X11CMap	(HiddX11GfxAB + aoHidd_X11Gfx_Hidd2X11CMap)
#define aHidd_X11Gfx_SysCursor		(HiddX11GfxAB + aoHidd_X11Gfx_SysCursor)
#define aHidd_X11Gfx_ColorMap		(HiddX11GfxAB + aoHidd_X11Gfx_ColorMap)
#define aHidd_X11Gfx_VisualClass	(HiddX11GfxAB + aoHidd_X11Gfx_VisualClass) /* stegerg */


#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)


#define expunge() \
    AROS_LC0(BPTR, expunge, struct x11gfxbase *, LIBBASE, 3, X11Gfx)

#endif /* X11GFX_INTERN_H */
