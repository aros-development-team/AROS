/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 specific Internal Information Management
    Lang: english
*/

#ifndef _XLIB_H_
#   include <X11/Xlib.h>
#endif

/* A Pointer to this struct is stored in each RastPort->longreserved[0] */

struct gfx_driverdata {
    Window	  dd_Window;		/* X11 Window			*/
    GC		  dd_GC;		/* X11 GC			*/
    UWORD	* dd_AreaPtrn;		/* Amiga current AreaPtrn	*/
    BYTE	  dd_AreaPtSz;		/* Amiga AreaPtSz		*/
    Pixmap	  dd_Pixmap;		/* X11 converted AreaPtrn	*/
    UWORD	  dd_LinePtrn;		/* Amiga current LinePtrn	*/
    char	* dd_DashList;		/* X11 converted DashList	*/
    struct RastPort * dd_RastPort;	/* This RastPort		*/
};

extern Display * GetSysDisplay (void);
extern int	 GetSysScreen (void);
extern void	 SetGC (struct RastPort * rp, GC gc, struct GfxBase * GfxBase);
extern GC	 GetGC (struct RastPort * rp, struct GfxBase * GfxBase);
extern void	 SetXWindow (struct RastPort * rp, int win, struct GfxBase * GfxBase);

