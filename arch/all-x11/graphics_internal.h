/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    $Log$
    Revision 1.2  1998/01/09 21:05:19  hkiel
    Now contains only the struct for internal driver information.


    Desc: X11 specific Internal Information Management
    Lang: english
*/


/* A Pointer to this struct is stored in each RastPort->longreserved[0]	*/

struct gfx_driverdata {
    Window	dd_Window;		/* X11 Window			*/
    GC		dd_GC;			/* X11 GC			*/
    UWORD	* dd_AreaPtrn;		/* Amiga current AreaPtrn	*/
    BYTE	dd_AreaPtSz;		/* Amiga AreaPtSz		*/
    Pixmap	dd_Pixmap;		/* X11 converted AreaPtrn	*/
    UWORD	dd_LinePtrn;		/* Amiga current LinePtrn	*/
    char	* dd_DashList;		/* X11 converted DashList	*/
    struct RastPort	* dd_RastPort;	/* This RastPort 		*/
};
