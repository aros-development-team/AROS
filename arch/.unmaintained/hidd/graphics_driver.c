/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/semaphores.h>

#include <proto/exec.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>
#include "graphics_intern.h"
#include "graphics_internal.h"

#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)


struct ETextFont
{
    struct TextFont	etf_Font;
};

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


    FreeMem (driverdata
	, sizeof(struct gfx_driverdata)
    );
}

BOOL CorrectDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    BOOL retval = TRUE;
    struct gfx_driverdata * dd, * old;

    if (!rp)
    {
	retval = FALSE;
    }
    else
    {
	old = GetDriverData(rp);
	if (!old)
	{
	    old = InitDriverData(rp, GfxBase);
	    return FALSE;
	}
	else if (rp != old->dd_RastPort)
	{
	    dd = InitDriverData(rp, GfxBase);

	}
    }
    return retval;
}


int driver_init (struct GfxBase * GfxBase)
{
    return TRUE;
}

int driver_open (struct GfxBase * GfxBase)
{
    return TRUE;
}

void driver_close (struct GfxBase * GfxBase)
{
    return;
}

void driver_expunge (struct GfxBase * GfxBase)
{
    return;
}



void driver_SetABPenDrMd (struct RastPort * rp, ULONG apen, ULONG bpen,
	ULONG drmd, struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    apen &= PEN_MASK;
    bpen &= PEN_MASK;
}

void driver_SetAPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    pen &= PEN_MASK;
}

void driver_SetBPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    pen &= PEN_MASK;
}

void driver_SetOutlinePen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
}

void driver_SetDrMd (struct RastPort * rp, ULONG mode,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

void driver_EraseRect (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

void driver_RectFill (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

#define SWAP(a,b)       { a ^= b; b ^= a; a ^= b; }
#define ABS(x)          ((x) < 0 ? -(x) : (x))

void driver_ScrollRaster (struct RastPort * rp, LONG dx, LONG dy,
	LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase * GfxBase)
{

    CorrectDriverData (rp, GfxBase);
}

void driver_DrawEllipse (struct RastPort * rp, LONG x, LONG y, LONG rx, LONG ry,
		struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

void driver_Text (struct RastPort * rp, STRPTR string, LONG len,
		struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

WORD driver_TextLength (struct RastPort * rp, STRPTR string, ULONG len,
		    struct GfxBase * GfxBase)
{

    return 10;
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
}

ULONG driver_ReadPixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{

    if(!CorrectDriverData (rp, GfxBase))
	return ((ULONG)-1L);

    return ((ULONG)-1L);
}

LONG driver_WritePixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    if(!CorrectDriverData (rp, GfxBase))
	return ((ULONG)-1L);
    return 0;
}

void driver_PolyDraw (struct RastPort * rp, LONG count, WORD * coords,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

void driver_SetRast (struct RastPort * rp, ULONG color,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

void driver_SetFont (struct RastPort * rp, struct TextFont * font,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

struct TextFont * driver_OpenFont (struct TextAttr * ta,
	struct GfxBase * GfxBase)
{
    struct ETextFont * tf;

    if (!ta->ta_Name)
	return NULL;

    if (!(tf = AllocMem (sizeof (struct ETextFont), MEMF_ANY)) )
	return (NULL);

    return (struct TextFont *)tf;
}

void driver_CloseFont (struct TextFont * tf, struct GfxBase * GfxBase)
{
}

int driver_InitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{

    if (!rp->BitMap)
    {
	rp->BitMap = AllocMem (sizeof (struct BitMap), MEMF_CLEAR|MEMF_ANY);

	if (!rp->BitMap)
	{
	    return FALSE;
	}
    }

    if(!GetDriverData(rp))
	InitDriverData (rp, GfxBase);
    else
	CorrectDriverData(rp, GfxBase);

    rp->Flags |= 0x8000;

    return TRUE;
}

int driver_CloneRastPort (struct RastPort * newRP, struct RastPort * oldRP,
			struct GfxBase * GfxBase)
{
    CorrectDriverData (newRP, GfxBase);
    
    return FALSE;
}

void driver_DeinitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{

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

    CorrectDriverData (rp, GfxBase);

    return FALSE;
}

void driver_WaitTOF (struct GfxBase * GfxBase)
{
}

void driver_LoadRGB4 (struct ViewPort * vp, UWORD * colors, LONG count,
	    struct GfxBase * GfxBase)
{

} /* driver_LoadRGB4 */

void driver_LoadRGB32 (struct ViewPort * vp, ULONG * table,
	    struct GfxBase * GfxBase)
{


} /* driver_LoadRGB32 */

struct BitMap * driver_AllocBitMap (ULONG sizex, ULONG sizey, ULONG depth,
	ULONG flags, struct BitMap * friend, struct GfxBase * GfxBase)
{
    struct BitMap * nbm;

    nbm = AllocMem (sizeof (struct BitMap), MEMF_ANY|MEMF_CLEAR);

    if (nbm)
    {
    
    }

    return nbm;
}


LONG driver_BltBitMap (struct BitMap * srcBitMap, LONG xSrc,
	LONG ySrc, struct BitMap * destBitMap, LONG xDest,
	LONG yDest, LONG xSize, LONG ySize, ULONG minterm,
	ULONG mask, PLANEPTR tempA, struct GfxBase * GfxBase)
{
    LONG planecnt = 0;

    return planecnt;
}

void driver_BltClear (void * memBlock, ULONG bytecount, ULONG flags,
    struct GfxBase * GfxBase)
{
}

void driver_FreeBitMap (struct BitMap * bm, struct GfxBase * GfxBase)
{

}


void driver_SetRGB32 (struct ViewPort * vp, ULONG color,
	    ULONG red, ULONG green, ULONG blue,
	    struct GfxBase * GfxBase)
{

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


LONG driver_WritePixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    return 0;
} /* driver_WritePixelArray8 */


