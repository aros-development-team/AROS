#ifndef X11GFX_INTERN_H
#define X11GFX_INTERN_H
/*
    (C) 1997 AROS - The Amiga Research OS
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

/* Library stuff */
struct x11gfxbase
{
    struct Library 	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;

    struct Library	*utilitybase;
    struct Library	*oopbase;
    
    Class		*gfxclass;
    Class		*gcclass;
    Class		*bitmapclass;
    Class		*osbitmapclass;
	
};


Class *init_gfxclass   		( struct x11gfxbase *x11gfxbase );
Class *init_osbitmapclass    	( struct x11gfxbase *X11GfxBase );
Class *init_bitmapclass		( struct x11gfxbase *X11GfxBase );

VOID free_osbitmapclass	( struct x11gfxbase *X11GfxBase );
VOID free_bitmapclass	( struct x11gfxbase *X11GfxBase );
VOID free_gfxclass	( struct x11gfxbase *X11GfxBase );

ULONG map_x11_to_hidd(long *penarray, ULONG x11pixel);
XImage *alloc_ximage(Display *display, int screen, ULONG width, UBYTE depth, UBYTE height);
VOID free_ximage(XImage *image);


struct abdescr
{
    STRPTR interfaceid;
    AttrBase *attrbase;
};

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase);
VOID releaseattrbases(struct abdescr *abd, struct Library *OOPBase);

/* Private Attrs and methods for the X11Gfx Hidd */

#define IID_Hidd_X11Gfx "hidd.graphics.x11gfx"
#define IID_Hidd_X11Osbm "hidd.graphics.x11osbm"


#define HiddX11GfxAB  __abHidd_X11Gfx
#define HiddX11OsbmAB __abHidd_X11Osbm
extern AttrBase HiddX11GfxAB;
extern AttrBase HiddX11OsbmAB;

enum {
    aoHidd_X11Gfx_SysDisplay,
    aoHidd_X11Gfx_SysScreen,
    aoHidd_X11Gfx_Hidd2X11CMap,
    aoHidd_X11Gfx_SysCursor,
    aoHidd_X11Gfx_ColorMap,
    
    num_Hidd_X11Gfx_Attrs
    
};

#define aHidd_X11Gfx_SysDisplay		(HiddX11GfxAB + aoHidd_X11Gfx_SysDisplay)
#define aHidd_X11Gfx_SysScreen		(HiddX11GfxAB + aoHidd_X11Gfx_SysScreen)
#define aHidd_X11Gfx_Hidd2X11CMap	(HiddX11GfxAB + aoHidd_X11Gfx_Hidd2X11CMap)
#define aHidd_X11Gfx_SysCursor		(HiddX11GfxAB + aoHidd_X11Gfx_SysCursor)
#define aHidd_X11Gfx_ColorMap		(HiddX11GfxAB + aoHidd_X11Gfx_ColorMap)

enum {
    aoHidd_X11Osbm_XImage,
    
    num_Hidd_X11Osbm_Attrs
};

#define aHidd_X11Osbm_XImage		(HiddX11OsbmAB + aoHidd_X11Osbm_XImage)

#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)


#define expunge() \
AROS_LC0(BPTR, expunge, struct x11gfxbase *, LIBBASE, 3, X11Gfx)

#endif /* X11GFX_INTERN_H */
