/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define DEBUG_FreeMem 1

#define X11_LOCK
#define no_X11_LOCK_DEBUG
#define no_X11_LOCK_SEMAPHORE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/semaphores.h>

#include <clib/exec_protos.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>
#include "graphics_intern.h"
#include "graphics_internal.h"

#define static	/* nothing */

#define BMT_XIMAGE	(BMT_DRIVER+0)
#define BMT_XWINDOW	(BMT_DRIVER+1)

static Display	       * sysDisplay;
static int		 sysScreen;
static Cursor		 sysCursor;
#if defined (X11_LOCK_SEMAPHORE)
static struct SignalSemaphore * X11lock;
#endif

extern struct Task * inputDevice;
#define SIGID()      Signal (inputDevice, SIGBREAKF_CTRL_F)

/* Table which links TextAttr with X11 font names */
struct FontTable
{
    struct TextAttr ta;
    char	  * name;
}
AROSFontTable[] =
{
    { { "topaz.font",      8, FS_NORMAL, FPF_ROMFONT  }, "8x13bold" },
    { { "topaz.font",     13, FS_NORMAL, FPF_ROMFONT  }, "8x13bold" },
    { { "helvetica.font", 11, FS_NORMAL, FPF_DISKFONT }, "-adobe-helvetica-medium-r-normal--11-*-100-100-*-*-iso8859-1" },
    { { "helvetica.font", 12, FS_NORMAL, FPF_DISKFONT }, "-adobe-helvetica-medium-r-normal--12-*-100-100-*-*-iso8859-1" },
    { { "helvetica.font", 14, FS_NORMAL, FPF_DISKFONT }, "-adobe-helvetica-medium-r-normal--14-*-100-100-*-*-iso8859-1" },
};

#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)

static const char * sysColName[NUM_COLORS] =
{
    "grey70",
    "black",
    "white",
    "orange",

    "blue",
    "green",
    "red",
    "cyan",

    "magenta",
    "violet",
    "brown",
    "bisque",

    "lavender",
    "navy",
    "khaki",
    "sienna",
};

static long sysCMap[256];
static long maxPen;
static unsigned long sysPlaneMask;

struct ETextFont
{
    struct TextFont etf_Font;
    XFontStruct     etf_XFS;
};

#define ETF(tf)         ((struct ETextFont *)tf)

#if defined (X11_LOCK)
/* static struct GfxBase *lock_GfxBase; */

void LockX11 (struct GfxBase * GfxBase)
{
    /* struct GfxBase *GfxBase = lock_GfxBase; */
#if defined (X11_LOCK_DEBUG)
    fprintf (stderr, "LockX11()\n");
    fflush (stderr);
#endif
#if defined (X11_LOCK_SEMAPHORE)
    ObtainSemaphore (X11lock);
#else
    Disable ();
#endif
#if defined (X11_LOCK_DEBUG)
    fprintf (stderr, "  got X11 lock\n");
#endif
}

void UnlockX11 (struct GfxBase * GfxBase)
{
    /* struct GfxBase *GfxBase = lock_GfxBase; */
#if defined (X11_LOCK_DEBUG)
    fprintf (stderr, "UnlockX11()\n");
    fflush (stderr);
#endif
#if defined (X11_LOCK_SEMAPHORE)
    ReleaseSemaphore (X11lock);
#else
    Enable ();
#endif
}
#define LX11 LockX11(GfxBase);
#define UX11 UnlockX11(GfxBase);
#else
#define LX11
#define UX11
#endif


struct gfx_driverdata * GetDriverData (struct RastPort * rp)
{
    return (struct gfx_driverdata *) rp->longreserved[0];
}


/* SetDriverData() should only be used when cloning RastPorts           */
/* For other purposes just change the values directly in the struct.	*/

void SetDriverData (struct RastPort * rp, struct gfx_driverdata * DriverData)
{
    rp->longreserved[0] = (IPTR) DriverData;
}


/* InitDriverData() just allocates memory for the struct. To use e.g.   */
/* AreaPtrns, UpdateAreaPtrn() has to allocate the memory for the       */
/* Pattern itself (and free previously used memory!)                    */

struct gfx_driverdata * InitDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata * retval;

    retval = AllocMem (sizeof(struct gfx_driverdata), MEMF_CLEAR);
    if (!retval)
    {
	fprintf (stderr, "Can't allocate Memory for internal use\n");
	return NULL;
    }
    retval->dd_RastPort = rp;
    rp->longreserved[0] = (IPTR) retval;

    return retval;
}

void DeinitDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata * driverdata;

    driverdata = (struct gfx_driverdata *) rp->longreserved[0];

/* Free all allocated memory */
    if (driverdata->dd_AreaPtrn)
	FreeMem (driverdata->dd_AreaPtrn
	, sizeof(UWORD) * (1<<driverdata->dd_AreaPtSz)
	);

    if (driverdata->dd_DashList)
	FreeMem (driverdata->dd_DashList
	    , 16 * sizeof(char)
	);

    FreeMem (driverdata
	, sizeof(struct gfx_driverdata)
    );
}

BOOL CorrectDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    BOOL retval = True;
    struct gfx_driverdata * dd, * old;

    if (!rp)
    {
	retval = False;
    }
    else
    {
	old = GetDriverData(rp);
	if (!old)
	{
	    old = InitDriverData(rp, GfxBase);
	    return False;
	}
	else if (rp != old->dd_RastPort)
	{
	    dd = InitDriverData(rp, GfxBase);
	    if (old->dd_Window)
		dd->dd_Window = old->dd_Window;
	    else
		retval = False;
	    if (old->dd_GC)
	    {
		XGCValues gcval;
		GC gc;

		gcval.plane_mask = sysPlaneMask;
		gcval.graphics_exposures = True;
LX11
		gc = XCreateGC (sysDisplay
		    , DefaultRootWindow (sysDisplay)
		    , GCPlaneMask
			| GCGraphicsExposures
		    , &gcval
		);
UX11
		if (gc)
		{
		    XCopyGC (sysDisplay, old->dd_GC, ~0L, gc);
		    dd->dd_GC = gc;
		}
		else
		    retval = False;
	    }
	    else
		retval = False;
	}
    }
    return retval;
}


int driver_init (struct GfxBase * GfxBase)
{
    char * displayName;
    Colormap cm;
/*    XColor xc; */
    XColor fg, bg;
/*    short t; */
    short depth;

/* #if defined (X11_LOCK)
    lock_GfxBase = GfxBase;
#endif */

    if (!XInitThreads ())
    {
	fprintf (stderr, "Warning: XInitThreads() failed or threads not supported\n");
    }

    if (!(displayName = getenv ("DISPLAY")) )
	displayName = ":0.0";

    if (!(sysDisplay = XOpenDisplay (displayName)) )
    {
	fprintf (stderr, "Cannot open display %s\n", displayName);
	return False;
    }

#if defined (X11_LOCK) && defined (X11_LOCK_SEMAPHORE)
    X11lock = AllocMem (sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);

    if (!X11lock)
    {
	fprintf (stderr, "Can't create X11lock\n");
	return False;
    }
    InitSemaphore (X11lock);
#endif

    sysScreen = DefaultScreen (sysDisplay);

    depth = DisplayPlanes (sysDisplay, sysScreen);
    cm = DefaultColormap (sysDisplay, sysScreen);

    sysPlaneMask = 0;

/* nlorentz: This code most probably isn't necessary, as the palette now is set via.
   LoadRGB32(). But it should be kept here for a while anyway.

    for (t=0; t < NUM_COLORS; t++)
    {
	if (depth == 1)
	{
	    sysCMap[t] = !(t & 1) ?
		    WhitePixel(sysDisplay, sysScreen) :
		    BlackPixel(sysDisplay, sysScreen);
	}
	else
	{
	    if (XParseColor (sysDisplay, cm, sysColName[t], &xc))
	    {

		if (!XAllocColor (sysDisplay, cm, &xc))
		{
		    fprintf (stderr, "Couldn't allocate color %s\n",
			    sysColName[t]);
		    sysCMap[t] = !(t & 1) ?
			    WhitePixel(sysDisplay, sysScreen) :
			    BlackPixel(sysDisplay, sysScreen);
		}
		else
		    sysCMap[t] = xc.pixel;

		if (t == 0)
		    bg = xc;
		else if (t == 1)
		    fg = xc;
	    }
	    else
	    {
		fprintf (stderr, "Couldn't get color %s\n", sysColName[t]);
	    }
	}

	sysPlaneMask |= sysCMap[t];
    }
*/
    maxPen = NUM_COLORS;


    sysCursor = XCreateFontCursor (sysDisplay, XC_top_left_arrow);
    
    fg.pixel = BlackPixel(sysDisplay, sysScreen);
    fg.red = 0x0000; fg.green = 0x0000; fg.blue = 0x0000;
    fg.flags = (DoRed|DoGreen|DoBlue);
    
    bg.pixel = WhitePixel(sysDisplay, sysScreen);
    bg.red = 0xFFFF; bg.green = 0xFFFF; bg.blue = 0xFFFF;
    bg.flags = (DoRed|DoGreen|DoBlue);
    
    XRecolorCursor (sysDisplay, sysCursor, &fg, &bg);
    return True;
}

int driver_open (struct GfxBase * GfxBase)
{
    return True;
}

void driver_close (struct GfxBase * GfxBase)
{
    return;
}

void driver_expunge (struct GfxBase * GfxBase)
{
    return;
}

/* Called after DOS is up & running */
BOOL driver_LateGfxInit (APTR data, struct GfxBase *GfxBase)
{

    return TRUE;
}


GC GetGC (struct RastPort * rp, struct GfxBase * GfxBase)
{
    return (GC) GetDriverData(rp)->dd_GC;
}

int GetXWindow (struct RastPort * rp, struct GfxBase * GfxBase)
{
    return (int) GetDriverData(rp)->dd_Window;
}

void SetGC (struct RastPort * rp, GC gc, struct GfxBase * GfxBase)
{
    if (GetDriverData(rp)->dd_GC)
    {
LX11
	XFreeGC (sysDisplay, GetDriverData(rp)->dd_GC);
UX11
    }

    GetDriverData(rp)->dd_GC = gc;
}

void SetXWindow (struct RastPort * rp, int win, struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);

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

    GetDriverData(rp)->dd_Window = win;
}

Display * GetSysDisplay (void)
{
    return sysDisplay;
}

int GetSysScreen (void)
{
    return sysScreen;
}

BOOL CompareAreaPtrn(UWORD * pat1, UWORD * pat2, BYTE size)
{
    int y;
    BOOL equal = True;

    for (y=0; y < (1<<size) && equal; y++)
	equal = (pat1[y] == pat2[y]);

    return equal;
}

Pixmap ConvertAreaPtrn (struct RastPort * rp, struct GfxBase * GfxBase)
{
    int y, z, width, height, depth;
    char * pattern;
    Pixmap stipple;

    width = 16;

    if (rp->AreaPtSz >= 0)
    {
	/* Singlecolored AreaPtrn */
	height = 1<<(rp->AreaPtSz);
	if (height == 1)
	    pattern = AllocMem(2*2*sizeof(char),MEMF_CHIP|MEMF_CLEAR);
	pattern = AllocMem(2*height*sizeof(char),MEMF_CHIP|MEMF_CLEAR);

	for (y=0; y<height; y++)
	{
	    pattern[2*y] = (char)( (rp->AreaPtrn[y]) & (255) );
	    pattern[2*y+1] = (char)( ( (rp->AreaPtrn[y]) & (255<<8) ) >> 8);
	}
	if (height == 1)
	{
	    pattern[2] = pattern[0];
	    pattern[3] = pattern[1];
	    height = 2;
	}
LX11
	stipple = XCreateBitmapFromData (sysDisplay
	    , GetXWindow (rp, GfxBase)
	    , pattern
	    , width
	    , height
	);
UX11
	FreeMem (pattern, 2*height*sizeof(BYTE));

    }
    else
    {
	/* Multicolored AreaPtrn */
	unsigned long foreground, background;

	depth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);   /* Number of bitplanes used for AreaPtrn */
	height = 1>>(rp->AreaPtSz);     /* AreaPtSz is negative */

/* Specify the foreground and background pixel values to use.(Xlib Ref.Man.) */
	foreground = BlackPixel(sysDisplay, sysScreen);
	background = WhitePixel(sysDisplay, sysScreen);

	pattern = AllocMem(2*height*depth*sizeof(char),MEMF_CHIP|MEMF_CLEAR);
	for (z=0; z<depth; z++)
	    for (y=0; y<height; y++)
	    {
		pattern[2*y+2*z*height] = (char)( (rp->AreaPtrn[y+z*height]) & (0xff) );
		pattern[2*y+2*z*height+1] = (char)( ( (rp->AreaPtrn[y+z*height]) & (0xff<<8) ) >> 8);
	    }
LX11
	stipple = XCreatePixmapFromBitmapData (sysDisplay
	    , GetXWindow (rp, GfxBase)
	    , pattern
	    , width
	    , height
	    , foreground
	    , background
	    , depth
	);
UX11

	FreeMem (pattern, 2*height*depth*sizeof(char));
    }

    return stipple;
}

void CopyAreaPtrn (UWORD * dest, UWORD * src ,int height)
{
    int y;

    for (y=0; y<height; y++)
	dest[y] = src[y];
}

void UpdateAreaPtrn (struct RastPort * rp, struct GfxBase * GfxBase)
{
    if (rp->AreaPtrn != NULL)
    {
	int height;
	Pixmap stipple;
	struct gfx_driverdata * dd;

	dd = GetDriverData (rp);

	if (rp->AreaPtSz >= 0)
	{
	    /* Singlecolored AreaPtrn */

	    height = 1<<(rp->AreaPtSz);

	    if ( (dd->dd_AreaPtrn) && (dd->dd_AreaPtSz == rp->AreaPtSz) )
	    {
		if (!CompareAreaPtrn(dd->dd_AreaPtrn, rp->AreaPtrn, rp->AreaPtSz))
		{
LX11
		    XFreePixmap (sysDisplay, dd->dd_Pixmap);
UX11
		    stipple = ConvertAreaPtrn (rp, GfxBase);
		    CopyAreaPtrn (dd->dd_AreaPtrn, rp->AreaPtrn,height);
		}
		else
		    stipple = dd->dd_Pixmap;
	    }
	    else
	    {
		if (!dd->dd_AreaPtrn)
		    FreeMem (dd->dd_AreaPtrn, sizeof(UWORD)*(1<<dd->dd_AreaPtSz));
		dd->dd_AreaPtSz = rp->AreaPtSz;
		dd->dd_AreaPtrn = AllocMem (sizeof(UWORD)*height, MEMF_CHIP|MEMF_CLEAR);
LX11
		XFreePixmap (sysDisplay, dd->dd_Pixmap);
UX11
		stipple = ConvertAreaPtrn (rp, GfxBase);
		CopyAreaPtrn (dd->dd_AreaPtrn, rp->AreaPtrn,height);
	    }
	    dd->dd_Pixmap = stipple;

LX11
	    XSetFillStyle (sysDisplay
		, GetGC(rp, GfxBase)
		, FillStippled
	    );
	    XSetStipple( sysDisplay, GetGC (rp, GfxBase), stipple);
UX11
	}
	else {
	/* Multicolored AreaPtrn */

	    height = 1>>(rp->AreaPtSz);

	    if ( (dd->dd_AreaPtrn != NULL) && (dd->dd_AreaPtSz == rp->AreaPtSz) )
	    {
		if (!CompareAreaPtrn(dd->dd_AreaPtrn, rp->AreaPtrn, -rp->AreaPtSz))
		{
LX11
		    XFreePixmap (sysDisplay, dd->dd_Pixmap);
UX11
		    stipple = ConvertAreaPtrn (rp, GfxBase);
		    CopyAreaPtrn (dd->dd_AreaPtrn, rp->AreaPtrn,height);
		}
		else
		    stipple = dd->dd_Pixmap;
	    }
	    else
	    {
		if (!dd->dd_AreaPtrn)
		    FreeMem (dd->dd_AreaPtrn, sizeof(UWORD)*(1>>dd->dd_AreaPtSz));
		dd->dd_AreaPtSz = rp->AreaPtSz;
		dd->dd_AreaPtrn = AllocMem (sizeof(UWORD)*height, MEMF_CHIP|MEMF_CLEAR);
LX11
		XFreePixmap (sysDisplay, dd->dd_Pixmap);
UX11
		stipple = ConvertAreaPtrn (rp, GfxBase);
		CopyAreaPtrn (dd->dd_AreaPtrn, rp->AreaPtrn,height);
	    }
	    dd->dd_Pixmap = stipple;

LX11
	    XSetFillStyle (sysDisplay
		, GetGC(rp, GfxBase)
		, FillTiled
	    );

	    XSetTile( sysDisplay, GetGC (rp, GfxBase), stipple);
UX11
	}
    }
    else {
	/* No AreaPtrn -> Use solid fill style */
LX11
	XSetFillStyle (sysDisplay
	    , GetGC (rp, GfxBase)
	    , FillSolid
	);
UX11
    }
}

void UpdateLinePtrn (struct RastPort * rp, struct GfxBase * GfxBase)
{
    XGCValues gcval;

LX11
    XSetFillStyle (sysDisplay
	, GetGC(rp, GfxBase)
	, FillSolid
    );
UX11

    if (!(rp->Flags & 0x10) )
	return;

    if (rp->LinePtrn != 0xFFFF)
    {
	char dash_list[16];
	int  n;
	int  mode;
	UWORD bit;

	bit=0x8000;
	mode = (rp->LinePtrn & bit) != 0;
	dash_list[0] = 0;

	for (n=0; bit; bit>>=1)
	{
	    if (((rp->LinePtrn & bit) != 0) != mode)
	    {
		mode = !mode;
		n ++;
		dash_list[n] = 0;
	    }

	    dash_list[n] ++;
	}

	gcval.line_style = LineOnOffDash;
LX11
	XChangeGC (sysDisplay
	    , GetGC(rp, GfxBase)
	    , GCLineStyle
	    , &gcval
	);

	XSetDashes (sysDisplay
	    , GetGC(rp, GfxBase)
	    , 0
	    , dash_list
	    , n+1
	);
UX11

    }
    else
    {
	gcval.line_style = LineSolid;
LX11
	XChangeGC (sysDisplay
	    , GetGC(rp, GfxBase)
	    , GCLineStyle
	    , &gcval
	);
UX11
    }

    rp->Flags &= ~0x10;
}

void driver_SetABPenDrMd (struct RastPort * rp, ULONG apen, ULONG bpen,
	ULONG drmd, struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    apen &= PEN_MASK;
    bpen &= PEN_MASK;
LX11
    XSetForeground (sysDisplay, GetGC (rp, GfxBase), sysCMap[apen]);
    XSetBackground (sysDisplay, GetGC (rp, GfxBase), sysCMap[bpen]);

    if (drmd & COMPLEMENT)
	XSetFunction (sysDisplay, GetGC(rp, GfxBase), GXxor);
    else
	XSetFunction (sysDisplay, GetGC(rp, GfxBase), GXcopy);
UX11
}

void driver_SetAPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    pen &= PEN_MASK;
LX11
    XSetForeground (sysDisplay, GetGC (rp, GfxBase), sysCMap[pen]);
UX11
}

void driver_SetBPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    pen &= PEN_MASK;
LX11
    XSetBackground (sysDisplay, GetGC (rp, GfxBase), sysCMap[pen]);
UX11
}

void driver_SetOutlinePen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
}

void driver_SetDrMd (struct RastPort * rp, ULONG mode,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
LX11
    if (mode & COMPLEMENT)
	XSetFunction (sysDisplay, GetGC(rp, GfxBase), GXxor);
    else
	XSetFunction (sysDisplay, GetGC(rp, GfxBase), GXcopy);
UX11
}

void driver_EraseRect (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
LX11
    XClearArea (sysDisplay, GetXWindow (rp, GfxBase),
		x1, y1, x2-x1+1, y2-y1+1, False);

UX11
    SIGID ();
}

void driver_RectFill (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    UpdateAreaPtrn (rp, GfxBase);
LX11
    if (rp->DrawMode & COMPLEMENT)
    {
	ULONG pen;

	pen = ((ULONG)(rp->FgPen)) & PEN_MASK;

	XSetForeground (sysDisplay, GetGC (rp, GfxBase), sysPlaneMask);

	XFillRectangle (sysDisplay, GetXWindow (rp, GfxBase), GetGC (rp, GfxBase),
		x1, y1, x2-x1+1, y2-y1+1);

	XSetForeground (sysDisplay, GetGC (rp, GfxBase), sysCMap[pen]);
    }
    else
	XFillRectangle (sysDisplay, GetXWindow (rp, GfxBase), GetGC (rp, GfxBase),
		x1, y1, x2-x1+1, y2-y1+1);
UX11
    SIGID ();
}

#define SWAP(a,b)       { a ^= b; b ^= a; a ^= b; }
#define ABS(x)          ((x) < 0 ? -(x) : (x))

void driver_ScrollRaster (struct RastPort * rp, LONG dx, LONG dy,
	LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase * GfxBase)
{
    LONG w, h, x3, y3, x4, y4, _dx_, _dy_;
    ULONG apen;

    CorrectDriverData (rp, GfxBase);
    apen = GetAPen (rp);

    if (!dx && !dy) return;

    if (x2 < x1) SWAP (x1,x2)
    if (y2 < y1) SWAP (y1,y2)

    _dx_ = ABS(dx);
    _dy_ = ABS(dy);

    x3 = x1 + _dx_;
    y3 = y1 + _dy_;

    x4 = x2 - _dx_ +1;
    y4 = y2 - _dy_ +1;

    w = x2 - x3 +1;
    h = y2 - y3 +1;

    SetAPen (rp, rp->BgPen);
LX11
    if (dx <= 0) {
	if (dy <= 0) {
	    XCopyArea (sysDisplay,GetXWindow(rp, GfxBase),GetXWindow(rp, GfxBase),GetGC(rp, GfxBase),
		    x1, y1, w, h, x3, y3);

	    if (_dy_) XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x1, y1, w+_dx_, _dy_, FALSE);

	    if (_dx_) XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x1, y1, _dx_, h, FALSE);

	} else { /* dy > 0 */
	    XCopyArea (sysDisplay,GetXWindow(rp, GfxBase),GetXWindow(rp, GfxBase),GetGC(rp, GfxBase),
		    x1, y3, w, h, x3, y1);

	    XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x1, y4, w+_dx_, _dy_, FALSE);

	    if (_dx_) XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x1, y1, _dx_, h, FALSE);
	}
    } else { /* dx > 0 */
	if (dy <= 0) {
	    XCopyArea (sysDisplay,GetXWindow(rp, GfxBase),GetXWindow(rp, GfxBase),GetGC(rp, GfxBase),
		    x3, y1, w, h, x1, y3);

	    if (_dy_) XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x1, y1, w+_dx_, _dy_, FALSE);

	    XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x4, y3, _dx_, h, FALSE);
	} else { /* dy > 0 */
	    XCopyArea (sysDisplay,GetXWindow(rp, GfxBase),GetXWindow(rp, GfxBase),GetGC(rp, GfxBase),
		    x3, y3, w, h, x1, y1);

	    XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x1, y4, w+_dx_, _dy_, FALSE);

	    XClearArea (sysDisplay,GetXWindow(rp, GfxBase),
		    x4, y1, _dx_, h, FALSE);
	}
    }

    SetAPen (rp, apen);
UX11
    SIGID ();
}

void driver_DrawEllipse (struct RastPort * rp, LONG x, LONG y, LONG rx, LONG ry,
		struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    UpdateLinePtrn (rp, GfxBase);
LX11
    XDrawArc (sysDisplay, GetXWindow(rp, GfxBase), GetGC(rp, GfxBase),
	    x-rx, y-ry, rx*2, ry*2,
	    0, 360*64);
UX11
    SIGID ();
}

void driver_Text (struct RastPort * rp, STRPTR string, LONG len,
		struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
LX11
    if (rp->DrawMode & JAM2)
	XDrawImageString (sysDisplay, GetXWindow(rp, GfxBase), GetGC(rp, GfxBase), rp->cp_x,
	    rp->cp_y, string, len);
    else
	XDrawString (sysDisplay, GetXWindow(rp, GfxBase), GetGC(rp, GfxBase), rp->cp_x,
	    rp->cp_y, string, len);

    rp->cp_x += TextLength (rp, string, len);
    
    XFlush(sysDisplay);
UX11
    SIGID ();
}

WORD driver_TextLength (struct RastPort * rp, STRPTR string, ULONG len,
		    struct GfxBase * GfxBase)
{
    struct ETextFont * etf;
LX11
    etf = (struct ETextFont *)rp->Font;
UX11
    return XTextWidth (&etf->etf_XFS, string, len);
}

void driver_Move (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    return;
}

void driver_Draw (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    UpdateLinePtrn (rp, GfxBase);
LX11
    XDrawLine (sysDisplay, GetXWindow(rp, GfxBase), GetGC(rp, GfxBase),
	    rp->cp_x, rp->cp_y,
	    x, y);
UX11
    SIGID ();
}

ULONG driver_ReadPixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    XImage * image;
    unsigned long pixel;
    ULONG t;

    if(!CorrectDriverData (rp, GfxBase))
	return ((ULONG)-1L);
LX11
    XSync (sysDisplay, False);
    SIGID ();

    image = XGetImage (sysDisplay
	, GetXWindow(rp, GfxBase)
	, x, y
	, 1, 1
	, AllPlanes
	, ZPixmap
    );
UX11
    if (!image)
	return ((ULONG)-1L);
LX11
    pixel = XGetPixel (image, 0, 0);

    XDestroyImage (image);
UX11
    for (t=0; t<NUM_COLORS; t++)
	if (pixel == sysCMap[t])
	    return t;

    return ((ULONG)-1L);
}

LONG driver_WritePixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    if(!CorrectDriverData (rp, GfxBase))
	return ((ULONG)-1L);
LX11
    XDrawPoint (sysDisplay, GetXWindow(rp, GfxBase), GetGC(rp, GfxBase),
	    x, y);
UX11
    /* SIGID (); */

    return 0;
}

void driver_PolyDraw (struct RastPort * rp, LONG count, WORD * coords,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    UpdateLinePtrn (rp, GfxBase);
LX11
    XDrawLines (sysDisplay, GetXWindow(rp, GfxBase), GetGC(rp, GfxBase),
	    (XPoint *)coords, count,
	    CoordModeOrigin
    );
UX11
    SIGID ();
}

void driver_SetRast (struct RastPort * rp, ULONG color,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
LX11
    XClearArea (sysDisplay, GetXWindow(rp, GfxBase),
	    0, 0,
	    1000, 1000,
	    FALSE);
UX11
    SIGID ();
}

void driver_SetFont (struct RastPort * rp, struct TextFont * font,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
LX11
    if (GetGC(rp, GfxBase))
	XSetFont (sysDisplay, GetGC(rp, GfxBase), ETF(font)->etf_XFS.fid);
UX11
}

struct TextFont * driver_OpenFont (struct TextAttr * ta,
	struct GfxBase * GfxBase)
{
    struct ETextFont * tf;
    XFontStruct      * xfs;
    int t;
    char * name;

    if (!ta->ta_Name)
	return NULL;

    if (!(tf = AllocMem (sizeof (struct ETextFont), MEMF_ANY)) )
	return (NULL);

    xfs = NULL;
LX11
    for (t=0; t<sizeof(AROSFontTable)/sizeof(AROSFontTable[0]); t++)
    {
	if (AROSFontTable[t].ta.ta_YSize == ta->ta_YSize
	    && !strcasecmp (AROSFontTable[t].ta.ta_Name, ta->ta_Name)
	)
	{
	    xfs = XLoadQueryFont (sysDisplay, AROSFontTable[t].name);
	    break;
	}
    }
UX11
    if (!xfs)
    {
	FreeMem (tf, sizeof (struct ETextFont));
	return (NULL);
    }

    tf->etf_XFS = *xfs;

    t = strlen (ta->ta_Name);

    name = AllocVec (t+1, MEMF_ANY);

    if (name)
	strcpy (name, ta->ta_Name);
    else
    {
	FreeMem (tf, sizeof (struct ETextFont));
	return (NULL);
    }

    tf->etf_Font.tf_Message.mn_Node.ln_Name = name;
    tf->etf_Font.tf_YSize = tf->etf_XFS.max_bounds.ascent +
		    tf->etf_XFS.max_bounds.descent;
    tf->etf_Font.tf_XSize = tf->etf_XFS.max_bounds.rbearing -
		    tf->etf_XFS.min_bounds.lbearing;
    tf->etf_Font.tf_Baseline = tf->etf_XFS.ascent;
    tf->etf_Font.tf_LoChar = tf->etf_XFS.min_char_or_byte2;
    tf->etf_Font.tf_HiChar = tf->etf_XFS.max_char_or_byte2;

    if (!tf->etf_Font.tf_XSize || !tf->etf_Font.tf_YSize)
    {
LX11
	XUnloadFont (sysDisplay, tf->etf_XFS.fid);
UX11
	FreeMem (tf, sizeof (struct ETextFont));
	return (NULL);
    }
    tf->etf_Font.tf_Accessors ++;

    return (struct TextFont *)tf;
}

void driver_CloseFont (struct TextFont * tf, struct GfxBase * GfxBase)
{
LX11
    if (!ETF(tf)->etf_Font.tf_Accessors)
    {
	XUnloadFont (sysDisplay, ETF(tf)->etf_XFS.fid);
	FreeVec (ETF(tf)->etf_Font.tf_Message.mn_Node.ln_Name);
	FreeMem (tf, sizeof (struct ETextFont));
    }
    else
	ETF(tf)->etf_Font.tf_Accessors --;
UX11
}

int driver_InitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    XGCValues gcval;
    GC gc;

    gcval.plane_mask = sysPlaneMask;
    gcval.graphics_exposures = True;
LX11
    gc = XCreateGC (sysDisplay
	, DefaultRootWindow (sysDisplay)
	, GCPlaneMask
	    | GCGraphicsExposures
	, &gcval
    );
UX11
    if (!gc)
	return FALSE;

    if (!rp->BitMap)
    {
	rp->BitMap = AllocMem (sizeof (struct BitMap), MEMF_CLEAR|MEMF_ANY);

	if (!rp->BitMap)
	{
LX11
	    XFreeGC (sysDisplay, gc);
UX11
	    return FALSE;
	}
    }

    if(!GetDriverData(rp))
	InitDriverData (rp, GfxBase);
    else
	CorrectDriverData(rp, GfxBase);
    SetGC (rp, gc, GfxBase);

    rp->Flags |= 0x8000;

    return TRUE;
}

int driver_CloneRastPort (struct RastPort * newRP, struct RastPort * oldRP,
			struct GfxBase * GfxBase)
{
    GC gc;
LX11
    gc = XCreateGC (sysDisplay
	, GetXWindow (oldRP, GfxBase)
	, 0L
	, NULL
    );
UX11
    if (!gc)
	return FALSE;
LX11
    XCopyGC (sysDisplay, GetGC(oldRP, GfxBase), (1L<<(GCLastBit+1))-1, gc);
    CorrectDriverData (newRP, GfxBase);
    SetGC (newRP, gc, GfxBase);
UX11
    if (oldRP->BitMap)
    {
	newRP->BitMap = AllocMem (sizeof (struct BitMap), MEMF_CLEAR|MEMF_ANY);

	if (!newRP->BitMap)
	{
LX11
	    XFreeGC (sysDisplay, gc);
UX11
	    return FALSE;
	}

	CopyMem (oldRP->BitMap, newRP->BitMap, sizeof (struct BitMap));
    }

    return TRUE;
}

void driver_DeinitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    if (GetGC (rp, GfxBase))
	SetGC (rp, NULL, GfxBase);

    if (rp->BitMap)
	FreeMem (rp->BitMap, sizeof (struct BitMap));

    if (GetDriverData(rp)->dd_RastPort == rp)
	DeinitDriverData (rp, GfxBase);
}

void driver_InitView(struct View * View, struct GfxBase * GfxBase)
{
  /* To Do */
  View->DxOffset = 0;
  View->DyOffset = 0;
} /* driver_InitView */

void driver_InitVPort(struct ViewPort * ViewPort, struct GfxBase * GfxBase)
{
  /* To Do (maybe even an unnecessary function) */
} /* driver_InitVPort */

ULONG driver_SetWriteMask (struct RastPort * rp, ULONG mask,
			struct GfxBase * GfxBase)
{
    XGCValues gcval;
    GC gc;

    CorrectDriverData (rp, GfxBase);
    if ((gc = GetGC (rp, GfxBase)))
    {
	gcval.plane_mask = sysPlaneMask;
LX11
	XChangeGC (sysDisplay
	    , gc
	    , GCPlaneMask
	    , &gcval
	);
UX11
	return TRUE;
    }

    return FALSE;
}

void driver_WaitTOF (struct GfxBase * GfxBase)
{
#warning TODO: Write x11/driver_WaitTOF
}

void driver_LoadRGB4 (struct ViewPort * vp, UWORD * colors, LONG count,
	    struct GfxBase * GfxBase)
{
    int t;
/*    XColor xc; */
    Colormap cm;

    cm = DefaultColormap (sysDisplay, sysScreen);

    t = (count < maxPen) ? count : maxPen;
LX11
    /* Return colors */
    if (t)
	XFreeColors (sysDisplay, cm, sysCMap, t, 0L);
UX11
    /* Allocate new colors */
    for (t=0; t<count; t++)
    {
	driver_SetRGB32 (vp, t
	    , (colors[t] & 0x0F00) << 20
	    , (colors[t] & 0x00F0) << 24
	    , (colors[t] & 0x000F) << 28
	    , GfxBase
	);
    }
LX11
    XSync (sysDisplay, False);
UX11
    SIGID ();

    if (count > maxPen)
	maxPen = count;

/* printf ("maxPen = %d\n", maxPen); */

    for (t=0,sysPlaneMask=0; t<maxPen; t++)
	sysPlaneMask |= sysCMap[t];

} /* driver_LoadRGB4 */

void driver_LoadRGB32 (struct ViewPort * vp, ULONG * table,
	    struct GfxBase * GfxBase)
{
    int t /*, r, g, b */ ;
    /* XColor xc; */
    Colormap cm;
    UWORD count, first;

    cm = DefaultColormap (sysDisplay, sysScreen);

    while (*table)
    {
	count = (*table) >> 16;
	first = *table & 0xFFFF;

	table ++;

	t = (count+first < maxPen) ? count : maxPen-first;
LX11
	/* Return colors */
	if (t > 0)
/*	    XFreeColors (sysDisplay, cm, &sysCMap[first], t, 0L) */;
#warning FIXME:The XfreeColors call above makes XAllocColor crash
UX11
	/* Allocate new colors */
	for (t=0; t<count; t++)
	{
	    driver_SetRGB32 (vp, t
		, table[0]
		, table[1]
		, table[2]
		, GfxBase
	    );

	    table += 3;
	}

	if (count > maxPen)
	    maxPen = count;
    }
LX11
    XSync (sysDisplay, False);
UX11
    SIGID();

/* printf ("maxPen = %d\n", maxPen); */

    for (t=0,sysPlaneMask=0; t<maxPen; t++)
	sysPlaneMask |= sysCMap[t];

} /* driver_LoadRGB32 */

struct BitMap * driver_AllocBitMap (ULONG sizex, ULONG sizey, ULONG depth,
	ULONG flags, struct BitMap * friend, struct GfxBase * GfxBase)
{
    struct BitMap * nbm;

    nbm = AllocMem (sizeof (struct BitMap), MEMF_ANY|MEMF_CLEAR);

    if (nbm)
    {
	if (flags & BMF_DISPLAYABLE)
	    depth = DefaultDepth (GetSysDisplay (), GetSysScreen ());

	nbm->BytesPerRow = 0;
	nbm->Rows	 = sizey;
	nbm->Pad	 = BMT_XIMAGE;

	nbm->Planes[0] = malloc (RASSIZE(sizex,sizey)*4);

	if (flags & BMF_CLEAR)
	    memset (nbm->Planes[0], 0, RASSIZE(sizex,sizey)*4);

	nbm->Planes[1] = (PLANEPTR) XCreateImage (sysDisplay
	    , DefaultVisual (GetSysDisplay (), GetSysScreen ())
	    , depth
	    , ZPixmap
	    , 0
	    , nbm->Planes[0]
	    , sizex
	    , sizey
	    , 16
	    , 0
	);

	if (!nbm->Planes[0] || !nbm->Planes[1])
	{
	    if (nbm->Planes[0])
		free (nbm->Planes[0]);

	    FreeMem (nbm, sizeof (struct BitMap));

	    return NULL;
	}
    }

    return nbm;
}

ULONG getBitMapPixel (struct BitMap * bm, LONG x, LONG y)
{
    ULONG pen, plane;
    ULONG bit;
    PLANEPTR ptr;

    pen = 0;

    bit = 1L << (x & 7);

    for (plane=0; plane<bm->Depth; plane++)
    {
	ptr = bm->Planes[plane] + y*bm->BytesPerRow + x/8;

	if (*ptr & bit)
	    pen |= 1L << plane;
    }

    if (bm->Depth < PEN_BITS)
	return sysCMap[pen];
    else
	return pen;
}

void copyBitMapToImage (struct BitMap * srcBitMap, LONG xSrc, LONG ySrc,
    XImage * xImage, LONG xDest, LONG yDest,
    ULONG minterm, ULONG mask, struct GfxBase * GfxBase)
{
    ULONG sColor, dColor;

    if (xSrc < 0 || xSrc >= (srcBitMap->BytesPerRow * 8)
	|| ySrc < 0 || ySrc >= srcBitMap->Rows
	|| xDest < 0 || yDest < 0
	|| xDest >= xImage->width || yDest >= xImage->height
    )
	return;
LX11
    sColor = getBitMapPixel (srcBitMap, xSrc, ySrc);
    dColor = XGetPixel (xImage, xDest, yDest);
UX11
    switch (minterm)
    {
    case 0x00C0:
	dColor = (dColor & ~mask) | (sColor & mask);
	break;

    case 0x0030:
	dColor = (dColor & ~mask) | (~sColor & mask);
	break;

    case 0x0050:
	dColor ^= mask;
	break;

    }
}

LONG driver_BltBitMap (struct BitMap * srcBitMap, LONG xSrc,
	LONG ySrc, struct BitMap * destBitMap, LONG xDest,
	LONG yDest, LONG xSize, LONG ySize, ULONG minterm,
	ULONG mask, PLANEPTR tempA, struct GfxBase * GfxBase)
{
    LONG planecnt = 0;

    switch (srcBitMap->Pad)
    {
    case BMT_STANDARD:
	switch (destBitMap->Pad)
	{
	case BMT_STANDARD:
	    /* Not possible */
	    break;

	case BMT_XIMAGE: {
	    LONG x, y;

	    for (y=0; y<ySize; y++)
	    {
		for (x=0; x<xSize; x++)
		{
		    copyBitMapToImage (srcBitMap, x+xSrc, y+ySrc
			, (XImage *)(destBitMap->Planes[0])
			, x+xDest, y+yDest
			, minterm, mask
			, GfxBase
		    );
		}
	    }

	    planecnt = ((XImage *)(destBitMap->Planes[0]))->depth;

	    break; }

	}

	break;

    case BMT_XIMAGE:
	switch (destBitMap->Pad)
	{
	case BMT_STANDARD:
	    break;

	case BMT_XIMAGE:
	    break;

	}

	break;
    }

    SIGID();

    return planecnt;
}

void driver_BltClear (void * memBlock, ULONG bytecount, ULONG flags,
    struct GfxBase * GfxBase)
{
  /* this will do the job on *any* system
   * without using the blitter
   */
  ULONG count, end;

  if (0 != (flags & 2) )
    /* use row/bytesperrow */
    bytecount = (bytecount & 0xFFFF) * (bytecount >> 16);

  /* we have an even number of BYTES to clear here */
  /* but use LONGS for clearing the block */
  count = 0;
  end = bytecount >> 2;
  while(count < end)
    ((ULONG *)memBlock)[count++] = 0;
  /* see whether we had an odd number of WORDS */
  if (0 != (bytecount & 2))
    ((UWORD *)memBlock)[(count * 2)] = 0;
}

void driver_FreeBitMap (struct BitMap * bm, struct GfxBase * GfxBase)
{
LX11
    switch (bm->Pad)
    {
    case BMT_XIMAGE:
	XDestroyImage ((XImage *)(bm->Planes[1]));
	break;

    }
UX11
}

void driver_SetRGB32 (struct ViewPort * vp, ULONG color,
	    ULONG red, ULONG green, ULONG blue,
	    struct GfxBase * GfxBase)
{
	int t;
    XColor xc;
    Colormap cm;

    if (color >= 256)
	return;

    cm = DefaultColormap (sysDisplay, sysScreen);
LX11
    /* Return color */
    if (color < maxPen)
/*	XFreeColors (sysDisplay, cm, &sysCMap[color], 1, 0L) */ ;
#warning FIXME:The XfreeColors call above makes XAllocColor crash
UX11
    /* Allocate new color */
    xc.flags = DoRed | DoGreen | DoBlue;
    xc.red   = red   >> 16;
    xc.green = green >> 16;
    xc.blue  = blue  >> 16;
LX11
    if (!XAllocColor (sysDisplay, cm, &xc))
    {
      fprintf (stderr, "Couldn't allocate color");
      if (color <=15)    
	fprintf (stderr, "%s\n",sysColName[color]);
      else
        fprintf (stderr, "\n");

	sysCMap[color] = !(color & 1) ?
		WhitePixel(sysDisplay, sysScreen) :
		BlackPixel(sysDisplay, sysScreen);
    }
    else
	sysCMap[color] = xc.pixel;

    XSync (sysDisplay, False);
UX11
    SIGID ();

    if (color > maxPen)
	maxPen = color;

/* printf ("maxPen = %d\n", maxPen); */

    for (t=0,sysPlaneMask=0; t<maxPen; t++)
	sysPlaneMask |= sysCMap[t];

} /* driver_SetRGB32 */

void driver_SetRGB4 (struct ViewPort * vp, ULONG color,
	    ULONG red, ULONG green, ULONG blue,
	    struct GfxBase * GfxBase)
{
 	driver_SetRGB32 (vp, color
	    , (ULONG)(red<<28)
	    , (ULONG)(green<<28)
	    , (ULONG)(blue<<28)
	    , GfxBase
	);
}

int highbit (int mask)
{
    int bit;

    for (bit=0; mask; bit++)
	mask >>= 1;

    return bit;
}

LONG driver_WritePixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    GC gc;
    Window win;
    int width, x, y;
    XImage *xim;
    Visual * theVisual;

    width = xstop - xstart + 1;

    CorrectDriverData (rp, GfxBase);
    gc = GetGC(rp, GfxBase);
    win = GetXWindow(rp, GfxBase);

#if 1
    theVisual = DefaultVisual (GetSysDisplay (), GetSysScreen ());

    if (theVisual->class == TrueColor || theVisual->class == DirectColor)
    {
	/************************************************************************/
	/* Non-ColorMapped Visuals:  TrueColor, DirectColor			*/
	/************************************************************************/
	int	      bperpix, bperline;
	UBYTE	     *imagedata, *ip;
	int	      height, border;

	height = ystop - ystart + 1;
LX11
	xim = XCreateImage (GetSysDisplay ()
	    , theVisual
	    , DefaultDepth (GetSysDisplay (), GetSysScreen ())
	    , ZPixmap
	    , 0
	    , NULL
	    , width
	    , height
	    , 32
	    , 0
	);
UX11
	if (!xim)
	    return 0;

	bperline = xim->bytes_per_line;
	bperpix  = xim->bits_per_pixel;
	border	 = xim->byte_order;

	imagedata = malloc((size_t) (height * bperline));

	if (!imagedata)
	    return 0;

	xim->data = (char *) imagedata;

	if (bperpix != 16)
	{
	    fprintf(stderr, "Sorry, no code written to handle %d-bit %s",
	      bperpix, "TrueColor/DirectColor displays!");
	    return 0;
	}

	switch (bperpix)
	{
	case 16:
	    for (y=0; y<height; y++, imagedata+=bperline)
	    {
		for (x=0, ip=imagedata; x<width; x++)
		{
		    *((UWORD *)ip)++ = sysCMap[*array++];
		}
	    }
	    break;
	}
LX11
	XPutImage (sysDisplay
	    , win
	    , gc
	    , xim
	    , 0
	    , 0
	    , 0
	    , 0
	    , width
	    , height
	);

	XDestroyImage (xim);
UX11
    }
#else
    for (y=ystart; y<=ystop; y++)
    {
	for (x=xstart; x<=xstop; x++)
	{
LX11
	    XSetForeground (sysDisplay, gc, sysCMap[array[y*width+x]]);
	    XDrawPoint (sysDisplay, win, gc, x, y);
UX11
	}
    }
#endif

    SIGID();

    return width*(ystop - ystart + 1);
} /* driver_WritePixelArray8 */



void driver_BltBitMapRastPort (struct BitMap   * srcBitMap,
	LONG xSrc, LONG ySrc,
	struct RastPort * destRP,
	LONG xDest, LONG yDest,
	LONG xSize, LONG ySize,
	ULONG minterm, struct GfxBase * GfxBase
)
{
    return;
}

LONG driver_WritePixelLine8 (struct RastPort * rp, ULONG xstart,
			    ULONG ystart, ULONG width,
			    UBYTE * array, struct RastPort * temprp,
			    struct GfxBase *GfxBase)
{
    return 0;
}

LONG driver_ReadPixelLine8 (struct RastPort * rp, ULONG xstart,
			    ULONG ystart, ULONG width,
			    UBYTE * array, struct RastPort * temprp,
			    struct GfxBase *GfxBase)
{

    return 0;
}

VOID driver_WriteChunkyPixels(struct RastPort * rp, ULONG xstart, ULONG ystart,
		ULONG xstop, ULONG ystop, UBYTE * array,
		LONG bytesperrow, struct GfxBase *GfxBase)
{
    return;
    
}

VOID driver_BltTemplate(PLANEPTR source, LONG xSrc, LONG srcMod, struct RastPort * destRP,
	LONG xDest, LONG yDest, LONG xSize, LONG ySize, struct GfxBase *GfxBase)
{
    return;
}

LONG driver_ReadPixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    return 0;
}

VOID driver_BltPattern(struct RastPort *rp, PLANEPTR mask, LONG xMin, LONG yMin,
		LONG xMax, LONG yMax, ULONG byteCnt, struct GfxBase *GfxBase)
{
    return;
}

VOID driver_BltMaskBitMapRastPort(struct BitMap *srcBitMap
    		, LONG xSrc, LONG ySrc
		, struct RastPort *destRP
		, LONG xDest, LONG yDest
		, ULONG xSize, ULONG ySize
		, ULONG minterm
		, PLANEPTR bltMask
		, struct GfxBase *GfxBase )
{
    return;
}
