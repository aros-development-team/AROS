#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define DEBUG_FreeMem 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>
#include "graphics_intern.h"

#define static	/* nothing */

#define BMT_XIMAGE	(BMT_DRIVER+0)
#define BMT_XWINDOW	(BMT_DRIVER+1)

static Display	     * sysDisplay;
static int	       sysScreen;
static Cursor	       sysCursor;

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

int driver_init (struct GfxBase * GfxBase)
{
    char * displayName;
    Colormap cm;
    XColor xc;
    XColor fg, bg;
    short t;
    short depth;

    if (!(displayName = getenv ("DISPLAY")) )
	displayName = ":0.0";

    if (!(sysDisplay = XOpenDisplay (displayName)) )
    {
	fprintf (stderr, "Cannot open display %s\n", displayName);
	return False;
    }

    sysScreen = DefaultScreen (sysDisplay);

    depth = DisplayPlanes (sysDisplay, sysScreen);
    cm = DefaultColormap (sysDisplay, sysScreen);

    sysPlaneMask = 0;

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
/* printf ("Color(1) %d = %02x %02x %02x flags=%04x pixel=%08lx\n",
    t, xc.red, xc.green, xc.blue, xc.flags, xc.pixel); */

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
/* printf ("Color(2) %d = %02x %02x %02x flags=%04x pixel=%08lx\n",
    t, xc.red, xc.green, xc.blue, xc.flags, xc.pixel); */

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

    maxPen = NUM_COLORS;

    sysCursor = XCreateFontCursor (sysDisplay, XC_top_left_arrow);
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

GC GetGC (struct RastPort * rp)
{
    return (GC) rp->longreserved[0];
}

int GetXWindow (struct RastPort * rp)
{
    return (int) rp->longreserved[1];
}

void SetGC (struct RastPort * rp, GC gc)
{
    rp->longreserved[0] = (IPTR)gc;
}

void SetXWindow (struct RastPort * rp, int win)
{
    if (rp->BitMap)
    {
	int width, height, depth, dummy;
	Window dummywin;

	XGetGeometry (sysDisplay, win, &dummywin, &dummy, &dummy
	    , &width, &height
	    , &dummy
	    , &depth
	);

	rp->BitMap->BytesPerRow = ((width+15) >> 4)*2;
	rp->BitMap->Rows = height;
	rp->BitMap->Depth = depth;
	rp->BitMap->Flags = 0;
	rp->BitMap->Pad = BMT_XWINDOW;
	rp->BitMap->Planes[0] = (PLANEPTR)win;
    }

    rp->longreserved[1] = (IPTR)win;
}

Display * GetSysDisplay (void)
{
    return sysDisplay;
}

int GetSysScreen (void)
{
    return sysScreen;
}

void UpdateLinePtrn (struct RastPort * rp)
{
    XGCValues gcval;

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

	XChangeGC (sysDisplay
	    , GetGC(rp)
	    , GCLineStyle
	    , &gcval
	);

	XSetDashes (sysDisplay
	    , GetGC(rp)
	    , 0
	    , dash_list
	    , n+1
	);
    }
    else
    {
	gcval.line_style = LineSolid;

	XChangeGC (sysDisplay
	    , GetGC(rp)
	    , GCLineStyle
	    , &gcval
	);

    }

    rp->Flags &= ~0x10;
}

void driver_SetABPenDrMd (struct RastPort * rp, ULONG apen, ULONG bpen,
	ULONG drmd, struct GfxBase * GfxBase)
{
    apen &= PEN_MASK;
    bpen &= PEN_MASK;

    XSetForeground (sysDisplay, GetGC (rp), sysCMap[apen]);
    XSetBackground (sysDisplay, GetGC (rp), sysCMap[bpen]);

    if (drmd & COMPLEMENT)
	XSetFunction (sysDisplay, GetGC(rp), GXxor);
    else
	XSetFunction (sysDisplay, GetGC(rp), GXcopy);
}

void driver_SetAPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    pen &= PEN_MASK;

    XSetForeground (sysDisplay, GetGC (rp), sysCMap[pen]);
}

void driver_SetBPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    pen &= PEN_MASK;

    XSetBackground (sysDisplay, GetGC (rp), sysCMap[pen]);
}

void driver_SetOutlinePen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
}

void driver_SetDrMd (struct RastPort * rp, ULONG mode,
		    struct GfxBase * GfxBase)
{
    if (mode & COMPLEMENT)
	XSetFunction (sysDisplay, GetGC(rp), GXxor);
    else
	XSetFunction (sysDisplay, GetGC(rp), GXcopy);
}

void driver_EraseRect (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    XClearArea (sysDisplay, GetXWindow (rp),
		x1, y1, x2-x1+1, y2-y1+1, False);
}

void driver_RectFill (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    if (rp->DrawMode & COMPLEMENT)
    {
	ULONG pen;

	pen = ((ULONG)(rp->FgPen)) & PEN_MASK;

	XSetForeground (sysDisplay, GetGC (rp), sysPlaneMask);

	XFillRectangle (sysDisplay, GetXWindow (rp), GetGC (rp),
		x1, y1, x2-x1+1, y2-y1+1);

	XSetForeground (sysDisplay, GetGC (rp), sysCMap[pen]);
    }
    else
	XFillRectangle (sysDisplay, GetXWindow (rp), GetGC (rp),
		x1, y1, x2-x1+1, y2-y1+1);
}

#define SWAP(a,b)       { a ^= b; b ^= a; a ^= b; }
#define ABS(x)          ((x) < 0 ? -(x) : (x))

void driver_ScrollRaster (struct RastPort * rp, LONG dx, LONG dy,
	LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase * GfxBase)
{
    LONG w, h, x3, y3, x4, y4, _dx_, _dy_;
    ULONG apen = GetAPen (rp);

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

    if (dx <= 0) {
	if (dy <= 0) {
	    XCopyArea (sysDisplay,GetXWindow(rp),GetXWindow(rp),GetGC(rp),
		    x1, y1, w, h, x3, y3);

	    if (_dy_) XClearArea (sysDisplay,GetXWindow(rp),
		    x1, y1, w+_dx_, _dy_, FALSE);

	    if (_dx_) XClearArea (sysDisplay,GetXWindow(rp),
		    x1, y1, _dx_, h, FALSE);

	} else { /* dy > 0 */
	    XCopyArea (sysDisplay,GetXWindow(rp),GetXWindow(rp),GetGC(rp),
		    x1, y3, w, h, x3, y1);

	    XClearArea (sysDisplay,GetXWindow(rp),
		    x1, y4, w+_dx_, _dy_, FALSE);

	    if (_dx_) XClearArea (sysDisplay,GetXWindow(rp),
		    x1, y1, _dx_, h, FALSE);
	}
    } else { /* dx > 0 */
	if (dy <= 0) {
	    XCopyArea (sysDisplay,GetXWindow(rp),GetXWindow(rp),GetGC(rp),
		    x3, y1, w, h, x1, y3);

	    if (_dy_) XClearArea (sysDisplay,GetXWindow(rp),
		    x1, y1, w+_dx_, _dy_, FALSE);

	    XClearArea (sysDisplay,GetXWindow(rp),
		    x4, y3, _dx_, h, FALSE);
	} else { /* dy > 0 */
	    XCopyArea (sysDisplay,GetXWindow(rp),GetXWindow(rp),GetGC(rp),
		    x3, y3, w, h, x1, y1);

	    XClearArea (sysDisplay,GetXWindow(rp),
		    x1, y4, w+_dx_, _dy_, FALSE);

	    XClearArea (sysDisplay,GetXWindow(rp),
		    x4, y1, _dx_, h, FALSE);
	}
    }

    SetAPen (rp, apen);
}

void driver_DrawEllipse (struct RastPort * rp, LONG x, LONG y, LONG rx, LONG ry,
		struct GfxBase * GfxBase)
{
    UpdateLinePtrn (rp);

    XDrawArc (sysDisplay, GetXWindow(rp), GetGC(rp),
	    x-rx, y-ry, rx*2, ry*2,
	    0, 360*64);
}

void driver_Text (struct RastPort * rp, STRPTR string, LONG len,
		struct GfxBase * GfxBase)
{
    if (rp->DrawMode & JAM2)
	XDrawImageString (sysDisplay, GetXWindow(rp), GetGC(rp), rp->cp_x,
	    rp->cp_y, string, len);
    else
	XDrawString (sysDisplay, GetXWindow(rp), GetGC(rp), rp->cp_x,
	    rp->cp_y, string, len);

    rp->cp_x += TextLength (rp, string, len);
}

WORD driver_TextLength (struct RastPort * rp, STRPTR string, ULONG len,
		    struct GfxBase * GfxBase)
{
    struct ETextFont * etf;

    etf = (struct ETextFont *)rp->Font;

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
    UpdateLinePtrn (rp);

    XDrawLine (sysDisplay, GetXWindow(rp), GetGC(rp),
	    rp->cp_x, rp->cp_y,
	    x, y);
}

ULONG driver_ReadPixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    XImage * image;
    unsigned long pixel;
    ULONG t;

    XSync (sysDisplay, False);

    image = XGetImage (sysDisplay
	, GetXWindow(rp)
	, x, y
	, 1, 1
	, AllPlanes
	, ZPixmap
    );

    if (!image)
	return ((ULONG)-1L);

    pixel = XGetPixel (image, 0, 0);

    XDestroyImage (image);

    for (t=0; t<NUM_COLORS; t++)
	if (pixel == sysCMap[t])
	    return t;

    return ((ULONG)-1L);
}

LONG driver_WritePixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    XDrawPoint (sysDisplay, GetXWindow(rp), GetGC(rp),
	    x, y);

    return 0;
}

void driver_PolyDraw (struct RastPort * rp, LONG count, WORD * coords,
		    struct GfxBase * GfxBase)
{
    UpdateLinePtrn (rp);

    XDrawLines (sysDisplay, GetXWindow(rp), GetGC(rp),
	    (XPoint *)coords, count,
	    CoordModeOrigin
    );
}

void driver_SetRast (struct RastPort * rp, ULONG color,
		    struct GfxBase * GfxBase)
{
    XClearArea (sysDisplay, GetXWindow(rp),
	    0, 0,
	    1000, 1000,
	    FALSE);
}

void driver_SetFont (struct RastPort * rp, struct TextFont * font,
		    struct GfxBase * GfxBase)
{
    if (GetGC(rp))
	XSetFont (sysDisplay, GetGC(rp), ETF(font)->etf_XFS.fid);
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
	XUnloadFont (sysDisplay, tf->etf_XFS.fid);
	FreeMem (tf, sizeof (struct ETextFont));
	return (NULL);
    }

    tf->etf_Font.tf_Accessors ++;

    return (struct TextFont *)tf;
}

void driver_CloseFont (struct TextFont * tf, struct GfxBase * GfxBase)
{
    if (!ETF(tf)->etf_Font.tf_Accessors)
    {
	XUnloadFont (sysDisplay, ETF(tf)->etf_XFS.fid);
	FreeVec (ETF(tf)->etf_Font.tf_Message.mn_Node.ln_Name);
	FreeMem (tf, sizeof (struct ETextFont));
    }
    else
	ETF(tf)->etf_Font.tf_Accessors --;
}

int driver_InitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    XGCValues gcval;
    GC gc;

    gcval.plane_mask = sysPlaneMask;
    gcval.graphics_exposures = True;

    gc = XCreateGC (sysDisplay
	, DefaultRootWindow (sysDisplay)
	, GCPlaneMask
	    | GCGraphicsExposures
	, &gcval
    );

    if (!gc)
	return FALSE;

    if (!rp->BitMap)
    {
	rp->BitMap = AllocMem (sizeof (struct BitMap), MEMF_CLEAR|MEMF_ANY);

	if (!rp->BitMap)
	{
	    XFreeGC (sysDisplay, gc);

	    return FALSE;
	}
    }

    SetGC (rp, gc);

    return TRUE;
}

int driver_CloneRastPort (struct RastPort * newRP, struct RastPort * oldRP,
			struct GfxBase * GfxBase)
{
    GC gc;

    gc = XCreateGC (sysDisplay
	, GetXWindow (oldRP)
	, 0L
	, NULL
    );

    if (!gc)
	return FALSE;

    XCopyGC (sysDisplay, GetGC(oldRP), (1L<<(GCLastBit+1))-1, gc);
    SetGC (newRP, gc);

    return TRUE;
}

void driver_DeinitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    GC gc;

    if ((gc = GetGC (rp)))
    {
	XFreeGC (sysDisplay, gc);

	SetGC (rp, NULL);
    }

    if (rp->BitMap)
    {
	FreeMem (rp->BitMap, sizeof (struct BitMap));
    }
}

ULONG driver_SetWriteMask (struct RastPort * rp, ULONG mask,
			struct GfxBase * GfxBase)
{
    XGCValues gcval;
    GC gc;

    if ((gc = GetGC (rp)))
    {
	gcval.plane_mask = sysPlaneMask;

	XChangeGC (sysDisplay
	    , gc
	    , GCPlaneMask
	    , &gcval
	);

	return TRUE;
    }

    return FALSE;
}

void driver_WaitTOF (struct GfxBase * GfxBase)
{
    /* TODO */
}

void driver_LoadRGB4 (struct ViewPort * vp, UWORD * colors, LONG count,
	    struct GfxBase * GfxBase)
{
    int t;
    XColor xc;
    Colormap cm;

    cm = DefaultColormap (sysDisplay, sysScreen);

    t = (count < maxPen) ? count : maxPen;

    /* Return colors */
    if (t)
	XFreeColors (sysDisplay, cm, sysCMap, t, 0L);

    /* Allocate new colors */
    for (t=0; t<count; t++)
    {
	xc.flags = DoRed | DoGreen | DoBlue;
	xc.red = ((colors[t] & 0x0F00) >> 8) << 12;
	xc.green = ((colors[t] & 0x00F0) >> 4) << 12;
	xc.blue = ((colors[t] & 0x000F) >> 0) << 12;

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

/* printf ("Color(1) %d = %04x %04x %04x flags=%04x pixel=%08lx\n",
    t, xc.red, xc.green, xc.blue, xc.flags, xc.pixel); */
    }

    XSync (sysDisplay, False);

    if (count > maxPen)
	maxPen = count;

/* printf ("maxPen = %d\n", maxPen); */

    for (t=0,sysPlaneMask=0; t<maxPen; t++)
	sysPlaneMask |= sysCMap[t];

} /* driver_LoadRGB4 */

void driver_LoadRGB32 (struct ViewPort * vp, ULONG * table,
	    struct GfxBase * GfxBase)
{
    int t;
    XColor xc;
    Colormap cm;
    UWORD count, first;

    cm = DefaultColormap (sysDisplay, sysScreen);

    while (*table)
    {
	count = (*table) >> 16;
	first = *table & 0xFFFF;

	table ++;

	t = (count+first < maxPen) ? count : maxPen-first;

	/* Return colors */
	if (t > 0)
	    XFreeColors (sysDisplay, cm, &sysCMap[first], t, 0L);

	/* Allocate new colors */
	for (t=0; t<count; t++)
	{
	    xc.flags = DoRed | DoGreen | DoBlue;
	    xc.red = *table++ >> 16;
	    xc.green = *table++ >> 16;
	    xc.blue = *table++ >> 16;

	    if (!XAllocColor (sysDisplay, cm, &xc))
	    {
		fprintf (stderr, "Couldn't allocate color %s\n",
			sysColName[t]);
		sysCMap[t] = !(t & 1) ?
			WhitePixel(sysDisplay, sysScreen) :
			BlackPixel(sysDisplay, sysScreen);
	    }
	    else
		sysCMap[t+first] = xc.pixel;

    /* printf ("Color(1) %d = %04x %04x %04x flags=%04x pixel=%08lx\n",
	t, xc.red, xc.green, xc.blue, xc.flags, xc.pixel); */
	}
    }

    XSync (sysDisplay, False);

    if (count > maxPen)
	maxPen = count;

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
    ULONG minterm, ULONG mask)
{
    ULONG sColor, dColor;

    if (xSrc < 0 || xSrc >= (srcBitMap->BytesPerRow * 8)
	|| ySrc < 0 || ySrc >= srcBitMap->Rows
	|| xDest < 0 || yDest < 0
	|| xDest >= xImage->width || yDest >= xImage->height
    )
	return;

    sColor = getBitMapPixel (srcBitMap, xSrc, ySrc);
    dColor = XGetPixel (xImage, xDest, yDest);

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

    return planecnt;
}

void driver_FreeBitMap (struct BitMap * bm, struct GfxBase * GfxBase)
{
    switch (bm->Pad)
    {
    case BMT_XIMAGE:
	XDestroyImage ((XImage *)(bm->Planes[1]));
	break;

    }
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

    /* Return color */
    if (color < maxPen)
	XFreeColors (sysDisplay, cm, &sysCMap[color], 1, 0L);

    /* Allocate new color */
    xc.flags = DoRed | DoGreen | DoBlue;
    xc.red   = red >> 16;
    xc.green = green >> 16;
    xc.blue  = blue >> 16;

    if (!XAllocColor (sysDisplay, cm, &xc))
    {
	fprintf (stderr, "Couldn't allocate color %s\n",
		sysColName[t]);
	sysCMap[t] = !(t & 1) ?
		WhitePixel(sysDisplay, sysScreen) :
		BlackPixel(sysDisplay, sysScreen);
    }
    else
	sysCMap[color] = xc.pixel;

    XSync (sysDisplay, False);

    if (color > maxPen)
	maxPen = color;

/* printf ("maxPen = %d\n", maxPen); */

    for (t=0,sysPlaneMask=0; t<maxPen; t++)
	sysPlaneMask |= sysCMap[t];

} /* driver_SetRGB32 */

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

    gc = GetGC(rp);
    win = GetXWindow(rp);

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
    }
#else
    for (y=ystart; y<=ystop; y++)
    {
	for (x=xstart; x<=xstop; x++)
	{
	    XSetForeground (sysDisplay, gc, sysCMap[array[y*width+x]]);
	    XDrawPoint (sysDisplay, win, gc, x, y);
	}
    }
#endif

    return width*(ystop - ystart + 1);
} /* driver_WritePixelArray8 */
