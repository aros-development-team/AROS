/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

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
}


/* Functions to access struct gfx_driverdata for a given rp		*/

/* Read values */
Window GetXWindow (struct RastPort * rp)
{
    return (Window) rp->longreserved[0]->dd_Window;
}

GC GetGC (struct RastPort * rp)
{
    return (GC) rp->longreserved[0]->dd_GC;
}

UWORD * GetAreaPtrn (struct RastPort * rp)
{
    return (UWORD *) rp->longreserved[0]->dd_AreaPtrn;
}

BYTE GetAreaPtSz (struct RastPort * rp)
{
    return rp->longreserved[0]->dd_AreaPtSz;
}

Pixmap GetPixmap (struct RastPort * rp)
{
    return rp->longreserved[0]->dd_Pixmap;
}

UWORD GetLinePtrn (struct RastPort * rp)
{
    return rp->longreserved[0]->dd_LinePtrn;
}

char * GetDashList (struct RastPort * rp)
{
    return rp->longreserved[0]->dd_DashList;
}

struct RastPort * GetRP (struct RastPort * rp)
{
    return (struct RastPort *) rp->longreserved[0]->dd_RastPort;
}

/* Set values */
void SetXWindow (struct RastPort * rp, int win)
{
    if (rp->BitMap)
    {
	int width, height, depth, dummy;
	Window dummywin;

LX11
	XGetGeometry (sysDisplay, win, &dummywin, &dummy, &dummy
	    , &width, &height
	    , &dummy
	    , &depth
	);
UX11

	rp->BitMap->BytesPerRow = ((width+15) >> 4)*2;
	rp->BitMap->Rows = height;
	rp->BitMap->Depth = depth;
	rp->BitMap->Flags = 0;
	rp->BitMap->Pad = BMT_XWINDOW;
	rp->BitMap->Planes[0] = (PLANEPTR)win;
    }

    rp->longreserved[0]->dd_Window = win;
}

void SetGC (struct RastPort * rp, GC gc)
{
    rp->longreserved[0]->dd_GC = gc;
}

void SetAreaPtrn (struct RastPort * rp, UWORD *AreaPtrn)
{
    rp->longreserved[0]->dd_AreaPtrn = AreaPtrn;
}

void SetAreaPtSz (struct RastPort * rp, BYTE AreaPtSz)
{
    rp->longreserved[0]->dd_AreaPtSz = AreaPtSz;
}

void SetPixmap (struct RastPort * rp, Pixmap AreaPixmap)
{
    rp->longreserved[0]->dd_Pixmap = AreaPixmap;
}

void SetLinePtrn (struct RastPort * rp, UWORD LinePtrn)
{
    rp->longreserved[0]->dd_LinePtrn = LinePtrn;
}

void SetDashList (struct RastPort * rp, char * DashList)
{
    rp->longreserved[0]->dd_DashList = DashList;
}

void SetRP (struct RastPort * rp, struct RastPort * rp_new)
{
    rp->longreserved[0]->dd_RastPort = rp_new;
}
