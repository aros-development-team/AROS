/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <clib/macros.h>

#include <proto/exec.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/gfxmacros.h>

#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <aros/asmcall.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"
#include "graphics_internal.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/* Default font for the HIDD driver */
#include "default_font.c"

#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)


#define PRIV_GFXBASE(base) ((struct GfxBase_intern *)base)

#define SDD(base)  ((struct shared_driverdata *)PRIV_GFXBASE(base)->shared_driverdata)

#define OOPBase (SDD(GfxBase)->oopbase)

/* Storage for bitmap object */
#define BM_OBJ(bitmap)	  ((APTR)(bitmap)->Planes[0])

/* Rastport flag that tells whether or not the driver has been inited */

#define RPF_DRIVER_INITED (1L << 15)

#warning All set drawing attribute/Render code should be semaphore protected to avoid race conditions on the drawing attrs

struct shared_driverdata
{
    Object *gfxhidd;
    struct Library *oopbase;
};

static VOID setbitmapfast(struct BitMap *bm, LONG x_start, LONG y_start, LONG xsize, LONG ysize, ULONG pen);
/* Attrbases */
static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddGCAttrBase = 0;


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
    struct gfx_driverdata * dd;
    EnterFunc(bug("InitDriverData(rp=%p)\n", rp));

    /* Does this rastport have a bitmap ? */
    if (rp->BitMap)
    {
        D(bug("Got bitmap\n"));
        /* Displayable ? (== rastport of a screen) */
	if (rp->BitMap->Flags & BMF_AROS_DISPLAYED)
	{
            D(bug("Has HIDD bitmap (displayable)\n"));

	    /* We can use this rastport. Allocate driverdata */
    	    dd = AllocMem (sizeof(struct gfx_driverdata), MEMF_CLEAR);
    	    if (dd)
    	    {
	        struct shared_driverdata *sdd;
		struct TagItem gc_tags[] = {
		    { aHidd_GC_BitMap, 	0UL},
		    { TAG_DONE, 	0UL} 
		};
		
		
		D(bug("Got driverdata\n"));
		sdd = SDD(GfxBase);
		
		/* Create a GC for it */
		gc_tags[0].ti_Data = (IPTR)BM_OBJ(rp->BitMap);
		
		dd->dd_GC = HIDD_Gfx_NewGC(sdd->gfxhidd, vHIDD_Gfx_GCType_Quick, gc_tags);
		if (dd->dd_GC)
		{
	
		    D(bug("Got GC HIDD object\n"));
    		    dd->dd_RastPort = rp;
    		    SetDriverData(rp, dd);
    		    rp->Flags |= RPF_DRIVER_INITED;
		    
		    ReturnPtr("InitDriverData", struct gfx_driverdata *, dd);
	        }
		
		FreeMem(dd, sizeof (struct gfx_driverdata));
	
    	    }
	}
    }

    ReturnPtr("InitDriverData", struct gfx_driverdata *, NULL);
}

void DeinitDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata * dd;
    struct shared_driverdata *sdd;
    
    EnterFunc(bug("DeInitDriverData(rp=%p)\n", rp));
		
    sdd = SDD(GfxBase);


    dd = (struct gfx_driverdata *) rp->longreserved[0];
    
    HIDD_Gfx_DisposeGC(sdd->gfxhidd, dd->dd_GC);

    FreeMem (dd, sizeof(struct gfx_driverdata));
    ReturnVoid("DeinitDriverData");
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
	    if (old)
	    	retval = TRUE;
	}
	else if (rp != old->dd_RastPort)
	{
	    dd = InitDriverData(rp, GfxBase);
	    if (dd)
	   	 retval = TRUE;

	}
    }
    return retval;
}

BOOL init_romfonts(struct GfxBase *GfxBase)
{
    struct TextFont *tf;
    
    
    tf = AllocMem( sizeof (struct TextFont), MEMF_ANY);
    if (tf)
    {
    	/* Copy the const font struct into allocated mem */
	CopyMem((APTR)&topaz8_tf, tf, sizeof (struct TextFont));
	
	AddFont(tf);
	GfxBase->DefaultFont = tf;
	
	return TRUE;
    }
    return FALSE;
}

int driver_init (struct GfxBase * GfxBase)
{

    EnterFunc(bug("driver_init()\n"));
    /* Allocate memory for driver data */
    SDD(GfxBase) = (struct shared_driverdata *)AllocMem(sizeof (struct shared_driverdata), MEMF_ANY|MEMF_CLEAR);
    if ( SDD(GfxBase) )
    {
        /* Open the OOP library */
	SDD(GfxBase)->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if ( SDD(GfxBase)->oopbase )
	{
	    /* Init the needed attrbases */
	    HiddBitMapAttrBase	= ObtainAttrBase(IID_Hidd_BitMap);
	    if (HiddBitMapAttrBase)
	    {
	    	HiddGCAttrBase 	= ObtainAttrBase(IID_Hidd_GC);
	    	if (HiddGCAttrBase)
		{
	           /* Init the driver's defaultfont */
		   if (init_romfonts(GfxBase))
	    	   	ReturnInt("driver_init", int, TRUE);
			
		   ReleaseAttrBase(IID_Hidd_GC);
		}
		
		ReleaseAttrBase(IID_Hidd_BitMap);
	    }
	    CloseLibrary( SDD(GfxBase)->oopbase );
	}
	FreeMem( SDD(GfxBase), sizeof (struct shared_driverdata));
	
    }
    ReturnInt("driver_init", int, FALSE);
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
    if ( SDD(GfxBase) )
    {
        if (HiddBitMapAttrBase)
	    ReleaseAttrBase(IID_Hidd_BitMap);
	    
        if (HiddGCAttrBase)
	    ReleaseAttrBase(IID_Hidd_GC);
	    
        if ( SDD(GfxBase)->oopbase )
	     CloseLibrary( SDD(GfxBase)->oopbase );
	     
	FreeMem( SDD(GfxBase), sizeof (struct shared_driverdata) );
    }
    return;
}

/* Called after DOS is up & running */
BOOL driver_LateGfxInit (APTR data, struct GfxBase *GfxBase)
{

    Class *cl;
    
    /* Supplied data is really the librarybase of a HIDD */
    struct Library *hiddbase = (struct Library *)data;
    
    EnterFunc(bug("driver_LateGfxInit(hiddbase=%p)\n", hiddbase));
    
    /* Get HIDD class from library base (allways vector 5) */
    cl = AROS_LVO_CALL0(Class *, struct Library *, hiddbase, 5, );
    D(bug("driver_LateGfxInit: class=%p\n", cl));
    if (cl)
    {
        Object *gfxhidd;
    	/* Create a new GfxHidd object */
	
	struct TagItem tags[] = { {TAG_DONE, 0UL} };
	
	gfxhidd = NewObject(cl, NULL, tags);
    	D(bug("driver_LateGfxInit: gfxhidd=%p\n", gfxhidd));
	
	if (gfxhidd)
	{
	    /* Store it in GfxBase so that we can find it later */
	    SDD(GfxBase)->gfxhidd = (APTR)gfxhidd;
	    ReturnBool("driver_LateGfxInit", TRUE);
	    
	}
	
	
    }
    
    ReturnBool("driver_LateGfxInit", FALSE);

}


static VOID clipagainstbitmap(struct BitMap *bm, LONG *x1, LONG *y1, LONG *x2, LONG *y2, struct GfxBase *GfxBase)
{
    ULONG width  = GetBitMapAttr(bm, BMA_WIDTH);
    ULONG height = GetBitMapAttr(bm, BMA_HEIGHT);
    
    /* Clip against bitmap bounds  */
	    
    if (*x1 < 0)  *x1 = 0;
    if (*y1 < 0)  *y1 = 0;

    if (*x2 >= width)  *x2 = width  - 1;
    if (*y2 >= height) *y2 = height - 1; 
    
    return;
}

void driver_SetABPenDrMd (struct RastPort * rp, ULONG apen, ULONG bpen,
	ULONG drmd, struct GfxBase * GfxBase)
{
    struct gfx_driverdata *dd;
    if (!CorrectDriverData (rp, GfxBase))
    	return;
	
	
    dd = GetDriverData(rp);
    if (dd)
    {
#warning Does not set DrMd yet.
    	struct TagItem gc_tags[] = {
    		{ aHidd_GC_Foreground,	apen & PEN_MASK},
    		{ aHidd_GC_Background,	bpen & PEN_MASK},
		{ aHidd_GC_ColorExpansionMode, 0UL},
		{ TAG_DONE,	0}
    	};

	if (drmd & JAM1)
    	    gc_tags[2].ti_Data = vHidd_GC_ColExp_Transparent;
	if (drmd & JAM2)
    	    gc_tags[2].ti_Data = vHidd_GC_ColExp_Opaque;
	
    	SetAttrs(dd->dd_GC, gc_tags);
	
    }
    return;
	
    
}

void driver_SetAPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    struct gfx_driverdata *dd;

/*    EnterFunc(bug("driver_SetAPen(rp=%p, pen=%d)\n", rp, pen));
*/    if (!CorrectDriverData (rp, GfxBase))
    	return;

    dd = GetDriverData(rp);
    if (dd)
    {
        struct TagItem col_tags[]= {
		{ aHidd_GC_Foreground, pen & PEN_MASK},
		{ TAG_DONE,	0UL}
	};

	SetAttrs( dd->dd_GC, col_tags );
	
    }
    return;
/*    ReturnVoid("driver_SetAPen");
*/
}

void driver_SetBPen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
    if (CorrectDriverData (rp, GfxBase))
    {
    	
        struct TagItem col_tags[]= {
		{ aHidd_GC_Background, pen & PEN_MASK},
		{ TAG_DONE,	0UL}
	};
	
	SetAttrs( GetDriverData(rp)->dd_GC, col_tags );
    }
}

void driver_SetOutlinePen (struct RastPort * rp, ULONG pen,
		    struct GfxBase * GfxBase)
{
}

void driver_SetDrMd (struct RastPort * rp, ULONG mode,
		    struct GfxBase * GfxBase)
{
    struct TagItem drmd_tags[] =
    {
	{ aHidd_GC_ColorExpansionMode, 0UL },
	{ TAG_DONE, 0UL }
    };
    
    if (!CorrectDriverData (rp, GfxBase))
    	return;
	
    if (mode & JAM1)
    	drmd_tags[0].ti_Data = vHidd_GC_ColExp_Transparent;
    if (mode & JAM2)
    	drmd_tags[0].ti_Data = vHidd_GC_ColExp_Opaque;
	
    SetAttrs( GetDriverData(rp)->dd_GC, drmd_tags);
	
    return;
    	
}


/* Return the intersection area of a and b in intersect.
 * Return value is TRUE if a and b have such an area,  
 * else FALSE - the coordinates in intersect are not     
 * changed in this case.
 */
static BOOL andrectrect(struct Rectangle* a, struct Rectangle* b, struct Rectangle* intersect)
{
    if (a->MinX <= b->MaxX) {
	if (a->MinY <= b->MaxY) {
	  if (a->MaxX >= b->MinX) {
		if (a->MaxY >= b->MinY) {
		    intersect->MinX = MAX(a->MinX, b->MinX);
		    intersect->MinY = MAX(a->MinY, b->MinY);
		    intersect->MaxX = MIN(a->MaxX, b->MaxX);
		    intersect->MaxY = MIN(a->MaxY, b->MaxY);
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
} /* andrectrect() */

void driver_RectFill (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{
    struct gfx_driverdata *dd;
    struct Layer *L = rp->Layer;
    struct BitMap *bm = rp->BitMap;
    
    struct TagItem bm_tags[] = 
    {
    	{aHidd_BitMap_GC,	0UL},
	{TAG_DONE,	0UL}
    };
    
    EnterFunc(bug("driver_RectFill(%d, %d, %d, %d)\n", x1, y1, x2, y2));
    if (!CorrectDriverData(rp, GfxBase))
    	return;
	
    dd = GetDriverData(rp);
    
    /* Get ready for setting bitmap's gc, but don't set unless neccesary */
    bm_tags[0].ti_Data = (IPTR)dd->dd_GC;

    
    if (NULL == L)
    {
        /* No layer, probably a screen */
	
	clipagainstbitmap(bm, &x1, &y1, &x2, &y2, GfxBase);
	SetAttrs(BM_OBJ(bm), bm_tags);	/* Set GC */
	HIDD_BM_FillRect(BM_OBJ(bm) , x1, y1, x2, y2 );
	
	
    }
    else
    {
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle tofill, intersect;
	
	tofill.MinX = x1 + xrel;
	tofill.MinY = y1 + yrel;
	tofill.MaxX = x2 + xrel;
	tofill.MaxY = y2 + yrel;
	
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to rectfill ? */
	    if (andrectrect(&CR->bounds, &tofill, &intersect))
	    {
	        if (NULL == CR->lobs)
		{
		    D(bug("non-obscure cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    SetAttrs(BM_OBJ(bm), bm_tags);
		    
		    /* Cliprect not obscured, so we may render directly into the display */
		    HIDD_BM_FillRect(BM_OBJ(bm)
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    );
		}
		else
		{
		    /* Render into offscreen cliprect bitmap */
#warning setbitmapfast should handle drawmodes (JAM1, JAM2,..)
		    setbitmapfast(CR->BitMap
		    	, intersect.MinX, intersect.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, GetAPen(rp)
		    );
		    
		}
	    }
	    CR = CR->Next;
	}
	

    }
    
	
	
    ReturnVoid("driver_RectFill");
}

void driver_BltBitMapRastPort (struct BitMap   * srcBitMap,
	LONG xSrc, LONG ySrc,
	struct RastPort * destRP,
	LONG xDest, LONG yDest,
	LONG xSize, LONG ySize,
	ULONG minterm, struct GfxBase *GfxBase
)
{
    struct gfx_driverdata *dd;
    ULONG width, height;
    struct Layer *L = destRP->Layer;
    struct BitMap *bm = destRP->BitMap;
    
    EnterFunc(bug("driver_BltBitMapRastPort(%d %d %d, %d, %d, %d)\n"
    	, xSrc, ySrc, xDest, yDest, xSize, ySize));
	
    if (!CorrectDriverData(destRP, GfxBase))
    	return;
	
    dd = GetDriverData(destRP);

    width  = GetBitMapAttr(bm, BMA_WIDTH);
    height = GetBitMapAttr(bm, BMA_HEIGHT);
    
    if (NULL == L)
    {
        /* No layer, probably a screen */
	
	/* BltBitMap() will do clipping against bitmap bounds for us  */

	BltBitMap(srcBitMap
		, xSrc, ySrc
		, bm
		, xDest, yDest
		, xSize, ySize
		, minterm, 0xFF	, NULL
	);
	
	
    }
    else
    {
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle toblit, intersect;
	
	toblit.MinX = xDest + xrel;
	toblit.MinY = yDest + yrel;
	toblit.MaxX = (xDest + xSize - 1) + xrel;
	toblit.MaxY = (yDest + ySize - 1) + yrel;
	
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to blit ? */
	    if (andrectrect(&CR->bounds, &toblit, &intersect))
	    {
	        if (NULL == CR->lobs)
		{
		    D(bug("non-obscured cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    /* Cliprect not obscured, so we may render directly into the display */
		    BltBitMap(srcBitMap
		    	, xSrc, ySrc
			, bm
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, minterm, destRP->Mask, NULL
		    );
		}
		else
		{
		    /* Render into offscreen cliprect bitmap */
		    BltBitMap(srcBitMap
		    	, xSrc, ySrc
			, CR->BitMap
		    	, intersect.MinX - CR->bounds.MinX
			, intersect.MinY - CR->bounds.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, minterm, destRP->Mask, NULL
		    );
		    
		    
		}
	    }
	    CR = CR->Next;
	}
	

    }
	
	
    ReturnVoid("driver_BltBitMapRastPort");
}

#define SWAP(a,b)       { a ^= b; b ^= a; a ^= b; }

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

#define NUMCHARS(tf) ((tf->tf_HiChar - tf->tf_LoChar) + 2)
#define CTF(x) ((struct ColorTextFont *)x)

void driver_Text (struct RastPort * rp, STRPTR string, LONG len,
		struct GfxBase * GfxBase)
{

#warning Does not handle color textfonts nor drawingmodes
    WORD  render_y;
    struct TextFont *tf;
    WORD current_x;
    
    if (!CorrectDriverData (rp, GfxBase))
    	return;
	
    
    tf = rp->Font;

    /* Render along font's baseline */
    render_y = rp->cp_y - tf->tf_Baseline;
    current_x = rp->cp_x;
    
    
    
    while ( len -- )
    {
	ULONG charloc;
	WORD render_x;
	ULONG idx;
	
	idx = *string - tf->tf_LoChar;
	charloc = ((ULONG *)tf->tf_CharLoc)[idx];

	
	if (tf->tf_Flags & FPF_PROPORTIONAL)
	{
	    render_x = current_x + ((WORD *)tf->tf_CharKern)[idx];
	}
	else
	    render_x = current_x;	/* Monospace */
	    
	if (tf->tf_Style & FSF_COLORFONT)
	{
	}
	else
	{
	
	    UWORD *src;
	    WORD xoffset;
	    
	    xoffset = charloc >> 16;
	    
	    src = ((UWORD *)tf->tf_CharData) + (xoffset >> 4);
	    xoffset &= 0x0F;

	    /* Get pointer to start of chardata */
	    D(bug("Blitting char at idx %d: %c\n", idx, *string));
	    
	    BltTemplate((PLANEPTR) src
	    	, xoffset
		, tf->tf_Modulo
		, rp
		, render_x, render_y
		, charloc & 0xFFFF 
		, tf->tf_YSize
	    );
	    
	}
	
	if (tf->tf_Flags & FPF_PROPORTIONAL)
	    current_x += ((WORD *)tf->tf_CharSpace)[idx];
	else
	    current_x += tf->tf_XSize; /* Add glyph width */
	
	string ++;
    } /* for (each character to render) */
    
    Move(rp, current_x, rp->cp_y);
    return;

}

WORD driver_TextLength (struct RastPort * rp, STRPTR string, ULONG len,
		    struct GfxBase * GfxBase)
{
    struct TextFont *tf = rp->Font;
    WORD strlen = 0;
    
    while (len --)
    {
	
	if (tf->tf_Flags & FPF_PROPORTIONAL)
	{
	    WORD idx;
	
	    if ( *string < tf->tf_LoChar || *string > tf->tf_HiChar)
	    {
		idx = NUMCHARS(tf) - 1; /* Last glyph is the default glyph */
	    }
	    else
	    {
		idx = *string - tf->tf_LoChar;
	    }
	    strlen += ((WORD *)tf->tf_CharSpace)[idx];
	}
	else
	{
	    strlen += tf->tf_XSize;
	}
	
	string ++;
	
    }
    return strlen;
}

void driver_Move (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
    return;
}

void driver_Draw (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
  /* Let's draw the line by using WritePixel() */
  
  LONG x_end = x;
  LONG y_end = y;
  LONG x_step = 0, y_step = 0;
  LONG dx = 1, dy = 1;
  LONG _x, _y;
  LONG steps, counter;
  
  EnterFunc(bug("driver_Draw(rp=%p, x=%d, y=%d)\n", rp, x, y));

    if (!CorrectDriverData (rp, GfxBase))
    	return;

  if (rp->cp_x != x)
    if (rp->cp_x > x)
    {
      x_step = -1;
      dx = rp->cp_x - x;
    }
    else
    {
      x_step = 1;
      dx = x - rp->cp_x;
    }

  if (rp->cp_y != y)
    if (rp->cp_y > y)
    {
      y_step = -1;
      dy = rp->cp_y - y;
    }
    else
    {
      y_step = 1;
      dy = y - rp->cp_y;
    }
  
  _x = 0;
  _y = 0;
  x = rp->cp_x;
  y = rp->cp_y;
  rp->cp_x = x_end;
  rp->cp_y = y_end;

  if (dx > dy)
    steps = dx;
  else
    steps = dy;
    
  counter = 0;  
  while (counter <= steps)
  {
    counter++;
    WritePixel(rp, x, y);

    if (dx > dy)
    {
      x += x_step;
      /* _x += dx; unnecessary in this case */
      _y += dy;
      if (_y >= dx)
      {
        _y -= dx;
        y += y_step;
      }
    }
    else
    {
      y += y_step;
      _x += dx;
      /* _y += dy; unnecessary in this case */
      if (_x >= dy)
      {
        _x -= dy;
        x += x_step;
      }
    }
  }
   ReturnVoid("driver_Draw");
}


ULONG driver_ReadPixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{
 
  struct gfx_driverdata *dd;

  struct Layer * L = rp -> Layer;
  BYTE Mask, pen_Mask;
  LONG count;
  struct BitMap * bm = rp->BitMap;
  ULONG Width, Height;
  ULONG i;
  BYTE * Plane;
  LONG penno;
  
  if(!CorrectDriverData (rp, GfxBase))
	return ((ULONG)-1L);
  dd = GetDriverData(rp);
  
  Width = GetBitMapAttr(bm, BMA_WIDTH);  
  Height = GetBitMapAttr(bm, BMA_HEIGHT);

  /* do we have a layer?? */
  if (NULL == L)
  {  
    /* is this pixel inside the rastport? 
       all pixels with coordinate less than zero are useless */
    if (x < 0 || x  > Width || 
        y < 0 || y  > Height)
    {
      /* no, it's not ! I refuse to do anything :-))  */
      return -1;
    }
  }

  if (0 != (Width & 0x07))
    Width = (Width >> 3) + 1;
  else
    Width = (Width >> 3);

  /* does this rastport have a layer? */
  if (NULL != L)
  {
    /* 
       more difficult part here as the layer might be partially 
       hidden.
       The coordinate (x,y) is relative to the layer.
    */
    struct ClipRect * CR = L -> ClipRect;
    WORD XRel = L->bounds.MinX;
    WORD YRel = L->bounds.MinY;
    
    /* Is this pixel inside the layer at all ?? */
    if (x > (L->bounds.MaxX - XRel + 1) ||
        y > (L->bounds.MaxY - YRel + 1)   )
    {
      /* ah, no it is not. So we exit. */
      return -1;
    }
    /* No one may interrupt me while I'm working with this layer */
/*!!!
    LockLayer(L);
*/
    /* search the list of ClipRects. If the cliprect where the pixel
       goes into does not have an entry in lobs, we can directly
       draw it to the bitmap, otherwise we have to draw it into the
       bitmap of the cliprect. 
    */
    while (NULL != CR)
    {
      if (x >= (CR->bounds.MinX - XRel) &&
          x <= (CR->bounds.MaxX - XRel) &&
          y >= (CR->bounds.MinY - YRel) &&  
          y <= (CR->bounds.MaxY - YRel)    )
      {
        /* that's our cliprect!! */
        /* if it is not hidden, then we treat it as if we were
           directly drawing to the BitMap  
        */
        LONG Offset;
        if (NULL == CR->lobs)
        {
	  
          i = (y + YRel) * Width + 
             ((x + XRel) >> 3);
          Mask = (1 << (7-((x + XRel) & 0x07)));
	  
	  if (bm->Flags & BMF_AROS_DISPLAYED)
	  {
	    return HIDD_BM_GetPixel(dd->dd_GC, x + XRel, y + YRel);
	  }
        } 
        else
        {
          /* we have to draw into the BitMap of the ClipRect, which
             will be shown once the layer moves... 
           */
           
          bm = CR -> BitMap;
           
          Width = GetBitMapAttr(bm, BMA_WIDTH);
          /* Calculate the Width of the bitplane in bytes */
          if (Width & 0x07)
            Width = (Width >> 3) + 1;
          else
            Width = (Width >> 3);
           
          if (0 == (L->Flags & LAYERSUPER))
          { 
            /* no superbitmap */
            Offset = CR->bounds.MinX & 0x0f;
          
            i = (y - (CR->bounds.MinY - YRel)) * Width + 
               ((x - (CR->bounds.MinX - XRel) + Offset) >> 3);   
                /* Offset: optimization for blitting!! */
            Mask = (1 << ( 7 - ((Offset + x - (CR->bounds.MinX - XRel) ) & 0x07)));
          }
          else
          {
            /* with superbitmap */
            i =  (y + L->Scroll_Y) * Width +
                ((x + L->Scroll_X) >> 3);
            Mask = 1 << (7 - ((x + L->Scroll_X) & 0x07));                 
          }
          
        }       
        break;
        
      } /* if */      
      /* examine the next cliprect */
      CR = CR->Next;
    } /* while */   
  } /* if */
  else
  { /* this is probably a screen */

    if (bm->Flags & BMF_AROS_DISPLAYED)
        return HIDD_BM_GetPixel(dd->dd_GC, x, y);
    
    i = y * Width + (x >> 3);
    Mask = (1 << (7-(x & 0x07)));
  }

 /* get the pen for this rastport */

  pen_Mask = 1;
  penno = 0;

  /* read the pixel from all bitplanes */
  for (count = 0; count < GetBitMapAttr(bm, BMA_DEPTH); count++)
  {
    Plane = bm->Planes[count];
    /* are we supposed to clear this pixel or set it in this bitplane */
    if (0 != (Plane[i] & Mask))
    {
      /* in this bitplane the pixel is  set */
      penno |= pen_Mask;                
    } /* if */

    pen_Mask = pen_Mask << 1;
  } /* for */
  
  /* if there was a layer I have to unlock it now */

/*!!!
  if (NULL != L) 
    UnlockLayer(L);
*/

  return penno;

}

LONG driver_WritePixel (struct RastPort * rp, LONG x, LONG y,
		    struct GfxBase * GfxBase)
{

  struct Layer * L = rp -> Layer;
  ULONG pen;
  BYTE Mask, pen_Mask, CLR_Mask;
  LONG count;
  struct BitMap * bm = rp->BitMap;
  ULONG Width, Height;
  ULONG i;
  BYTE * Plane; 
  struct gfx_driverdata *dd;

  struct TagItem bm_tags[] = 
  {
    	{aHidd_BitMap_GC,	0UL},
	{TAG_DONE,	0UL}
  };
  
  if(!CorrectDriverData (rp, GfxBase))
	return  -1L;
	
  dd = GetDriverData(rp);
  bm_tags[0].ti_Data = (IPTR) dd->dd_GC;

  Width = GetBitMapAttr(bm, BMA_WIDTH);  
  Height = GetBitMapAttr(bm, BMA_HEIGHT);
  
  /* 
     Is there a layer. If not then it cannot be a regular window!!
  */
  if (NULL == L)
  {  
    /* is this pixel inside the rastport? */
    if (x < 0 || x  > Width || 
        y < 0 || y  > Height)
    {
      /* no, it's not ! I refuse to do anything :-))  */
	return -1L;
    }
  }

  if (0 != (Width & 0x07))
    Width = (Width >> 3) + 1;
  else
    Width = (Width >> 3);

  /* does this rastport have a layer? */
  if (NULL != L)
  {
    /* more difficult part here as the layer might be partially 
       hidden.
       The coordinate x,y is relative to the layer.
    */
    struct ClipRect * CR = L -> ClipRect;
    WORD YRel = L->bounds.MinY + L->Scroll_Y;
    WORD XRel = L->bounds.MinX + L->Scroll_Y;

    /* Is this pixel inside the layer ?? */
    if (x > (L->bounds.MaxX - XRel + 1) ||
        y > (L->bounds.MaxY - YRel + 1)   )
    {
      /* ah, no it is not. So we exit */
	return -1L;
    }
    
    /* No one may interrupt me while I'm working with this layer */
    /* But there is a problem: if this is called from a routine that 
       already  locked  the layer is already I am stuck. 
    */
/* !!!
    LockLayer(L);
*/
    /* search the list of ClipRects. If the cliprect where the pixel
       goes into does not have an entry in lobs, we can directly
       draw it to the bitmap, otherwise we have to draw it into the
       bitmap of the cliprect. 
    */
    while (NULL != CR)
    {
      if (x >= (CR->bounds.MinX - XRel) &&
          x <= (CR->bounds.MaxX - XRel) &&
          y >= (CR->bounds.MinY - YRel) &&  
          y <= (CR->bounds.MaxY - YRel)    )
      {
        /* that's our cliprect!! */
        /* if it is not hidden, then we treat it as if we were
           directly drawing to the BitMap  
        */
        LONG Offset;
        if (NULL == CR->lobs)
        {
          /* this ClipRect is not hidden! */
          i = (y + YRel) * Width + 
             ((x + XRel) >> 3);
          Mask = 1 << (7-((x + XRel) & 0x07));

          /* and let the driver set the pixel to the X-Window also,
             but this Pixel has a relative position!! */
          if (bm->Flags & BMF_AROS_DISPLAYED)
	  {
	    SetAttrs( BM_OBJ(bm), bm_tags);
            HIDD_BM_DrawPixel ( BM_OBJ(bm), x+XRel, y+YRel);
	   } 
        } 
        else
        {
          /* we have to draw into the BitMap of the ClipRect, which
             will be shown once the layer moves... 
           */
          bm = CR -> BitMap;
           
          Width = GetBitMapAttr(bm, BMA_WIDTH);
          /* Calculate the Width of the bitplane in bytes */
          if (Width & 0x07)
            Width = (Width >> 3) + 1;
          else
            Width = (Width >> 3);
           
          if (0 == (L->Flags & LAYERSUPER))
          { 
            /* no superbitmap */
            Offset = CR->bounds.MinX & 0x0f;
          
            i = (y - (CR->bounds.MinY - YRel)) * Width + 
               ((x - (CR->bounds.MinX - XRel) + Offset) >> 3);   
                /* Offset: optimization for blitting!! */
            Mask = (1 << ( 7 - ((Offset + x - (CR->bounds.MinX - XRel) ) & 0x07)));
          }
          else
          {
            /* with superbitmap */
            i = ((y + L->Scroll_Y) * Width) +
                ((x + L->Scroll_X) >> 3);
            Mask = 1 << (7 - ((x + L->Scroll_X) & 0x07));                 
          }
          
        }       
        break;
        
      } /* if */      
      /* examine the next cliprect */
      CR = CR->Next;
    } /* while */
       
  } /* if */
  else
  { /* this is probably something like a screen */
  

    /* if it is an old window... */
/*    if (bm->Flags & BMF_AROS_OLDWINDOW)
         return driver_WritePixel (rp, x, y, GfxBase);
*/

    i = y * Width + (x >> 3);
    Mask = (1 << (7-(x & 0x07)));

    /* and let the driver set the pixel to the X-Window also */
    if (bm->Flags & BMF_AROS_DISPLAYED)
    {
      SetAttrs( BM_OBJ(bm), bm_tags );
      HIDD_BM_DrawPixel( BM_OBJ(bm), x, y);
    }

  }

  /* nlorentz: before writing to the bitmap manually,
     check that it is not a HIDD bitmap. 
     HIDD bitmaps have no planes in them
  */
  if ((bm->Flags & BMF_AROS_DISPLAYED) == 0)
  {

    /* get the pen for this rastport */
    pen = GetAPen(rp);

    pen_Mask = 1;
    CLR_Mask = ~Mask;
  
    /* we use brute force and write the pixel to
       all bitplane, setting the bitplanes where the pen is
       '1' and clearing the other ones */
    for (count = 0; count < GetBitMapAttr(bm, BMA_DEPTH); count++)
    {
      Plane = bm->Planes[count];

      /* are we supposed to clear this pixel or set it in this bitplane */
      if (0 != (pen_Mask & pen))
      {
        /* in this bitplane we're setting it */
        Plane[i] |= Mask;          
      } 
      else
      {
        /* and here we clear it */
        Plane[i] &= CLR_Mask;
      }
      pen_Mask = pen_Mask << 1;
    } /* for */
  
  } /* if (not a hidd bitmap) */


  /* if there was a layer I have to unlock it now */
/*!!!
  if (NULL != L) 
    UnlockLayer(L);
*/

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
    struct gfx_driverdata *dd;

    
    /* We have to use layers to perform clipping */
    struct Layer *L = rp->Layer;
    struct BitMap *bm = rp->BitMap;

    EnterFunc(bug("driver_SetRast(rp=%p, color=%u)\n", rp, color));
    
    if (!CorrectDriverData(rp, GfxBase))
    	return;
    
    dd = GetDriverData(rp);
    
    if (NULL != L)
    {
        /* Layered rastport, we have to clip this operation. */
    	/* Window rastport, we need to clip th operation */
	
        struct ClipRect *CR = L->ClipRect;
	struct Rectangle intersect;
	
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to blit ?
	    
	    Theoretically this test shouldn't be necessary because
	    we are setting the whole rastport, and all cliprects
	    should be inside that rastport, but I do not know
	    the internals of layers, so I'll do it anyway
	    
	    */
	    if (andrectrect(&CR->bounds, &L->bounds, &intersect))
	    {
		
	        if (NULL == CR->lobs)
		{
		    struct TagItem bm_tags[] =
		    {
			{ aHidd_BitMap_DrawMode, vHIDD_GC_DrawMode_Copy},
			{ aHidd_BitMap_Foreground, color },
			{ TAG_DONE, 0}
		    };
		    
		    D(bug("non-obscured cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    /* Write to the HIDD */
	
		    SetAttrs(BM_OBJ(bm), bm_tags);
		    HIDD_BM_FillRect(BM_OBJ(bm)
		    	, intersect.MinX, intersect.MinY
			, intersect.MaxX, intersect.MaxY
		    );
	
		}
		else
		{
		    setbitmapfast(CR->BitMap
		    	, intersect.MinX, intersect.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, color
		    );
		    
		    
		} /* if (intersecton inside hidden cliprect) */

		
	    } /* if (cliprect intersects with area we want to draw to) */
	    
	    CR = CR->Next;
	} /* while (cliprects to examine) */
	
	
    }
    else
    {
        /* Nonlayered (screen) */
	
	struct TagItem tags[] = {
    	    	{ aHidd_BitMap_Background, color },
		{ aHidd_BitMap_DrawMode, vHIDD_GC_DrawMode_Copy },
	    	{ TAG_DONE, 0UL }
	};
	
	D(bug("Calling SetAttrs\n"));
	SetAttrs( BM_OBJ(bm), tags);
	D(bug("Calling HIDD_BM_Clear\n"));
	HIDD_BM_Clear( BM_OBJ(bm) );
	    


    }
    ReturnVoid("driver_SetRast");

}

void driver_SetFont (struct RastPort * rp, struct TextFont * font,
		    struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
}

struct TextFont * driver_OpenFont (struct TextAttr * ta,
	struct GfxBase * GfxBase)
{
    struct TextFont *tf, *best_so_far = NULL;
    WORD bestmatch = 0;
   
    
    if (!ta->ta_Name)
	return NULL;
	
    /* Search for font in the fontlist */
    Forbid();
    ForeachNode(&GfxBase->TextFonts, tf)
    {
	if (0 == strcmp(tf->tf_Message.mn_Node.ln_Name, ta->ta_Name))
	{
	    UWORD match;
	    struct TagItem *tags = NULL;
	    struct TextAttr match_ta =
	    {
	    	tf->tf_Message.mn_Node.ln_Name,
		tf->tf_YSize,
		tf->tf_Style,
		tf->tf_Flags
	    };
	    
	    if (ExtendFont(tf, NULL))
	    {
	        tags = ((struct TextFontExtension *)tf->tf_Extension)->tfe_Tags;
	    }
	    else
	    	tags = NULL;
	    
	    match = WeighTAMatch(ta, &match_ta, tags);
	    if (match > bestmatch)
	    {
	    	bestmatch = match;
		best_so_far = tf;
	    }
	}
    }
    Permit();
	

    return best_so_far;
}

void driver_CloseFont (struct TextFont * tf, struct GfxBase * GfxBase)
{
    /* None using the fint anymore ? */
    if (    tf->tf_Accessors == 0
         && (tf->tf_Flags & FPF_ROMFONT) == 0) /* Don't free ROM fonts */
    {
        Forbid();
	
	Remove((struct Node *)tf);
	
	Permit();
	
	/* Free font data */
	
	/* !!! NOTE. FreeXXX functions has to match AllocXXX in
	   workbench/libs/diskfont/diskfont_io.c
	*/
	if (tf->tf_Style & FSF_COLORFONT)
	{
	    UWORD i;
	    struct ColorFontColors *cfc;
			
	    for (i = 0; i < 8; i ++)
	    {
		if (CTF(tf)->ctf_CharData[i])
		    FreeVec(CTF(tf)->ctf_CharData[i]);
	    }
	    
	    cfc = CTF(tf)->ctf_ColorFontColors;
	    if (cfc)
	    {
		if (cfc->cfc_ColorTable)
		    FreeVec(cfc->cfc_ColorTable);
				
		FreeVec(cfc);
	    }

	}
	else
	{
	    /* Not a colortextfont, only one plane */
	    FreeVec(tf->tf_CharData);
	}

	StripFont(tf);
	
	if (tf->tf_CharSpace)
	    FreeVec(tf->tf_CharSpace);
	    
	if (tf->tf_CharKern)
	    FreeVec(tf->tf_CharKern);
	    
	/* All fonts have a tf_CharLoc allocated */    
	FreeVec(tf->tf_CharLoc); 
	
	FreeVec(tf->tf_Message.mn_Node.ln_Name);
	FreeVec(tf);
	
    }
    return;
}


int driver_InitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{

   /* Do nothing */
   
/*    if (!rp->BitMap)
    {
	rp->BitMap = AllocMem (sizeof (struct BitMap), MEMF_CLEAR|MEMF_ANY);
	
	if (!rp->BitMap)
	{
	    return FALSE;
	}
    }

*/
/*    if(!GetDriverData(rp))
	InitDriverData (rp, GfxBase);
    else
	CorrectDriverData(rp, GfxBase);

*/

    return TRUE;
}

int driver_CloneRastPort (struct RastPort * newRP, struct RastPort * oldRP,
			struct GfxBase * GfxBase)
{
    /* Let CorrectDriverData() have a bitmap to use for the GC */
    newRP->BitMap = oldRP->BitMap;
    
    /* Creates a new GC. Hmmm, a general Copy method would've been nice */
    
    if (!CorrectDriverData (newRP, GfxBase))
    	return FALSE;
	
    /* copy rastports attributes */
    SetFont(newRP, oldRP->Font);
    SetABPenDrMd(newRP, GetAPen(oldRP), GetBPen(oldRP), GetDrMd(oldRP));
    Move(newRP, oldRP->cp_x, oldRP->cp_y);
    
#warning Some attributes not copied    
    return TRUE;
}

void driver_DeinitRastPort (struct RastPort * rp, struct GfxBase * GfxBase)
{
    D(bug("driver_DeInitRP()\n"));

    if ( rp->Flags & RPF_DRIVER_INITED )
    {
    	D(bug("RP inited, rp=%p, %flags=%d=\n", rp, rp->Flags));
		 
        if (GetDriverData(rp)->dd_RastPort == rp) 
	{
	    D(bug("Calling DeInitDriverData\n"));
	    DeinitDriverData (rp, GfxBase);
	}
    }
    return;
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

#warning TODO

    /* For now we do not support bit masking */
    return FALSE;
    
}

void driver_WaitTOF (struct GfxBase * GfxBase)
{
}

void driver_LoadRGB4 (struct ViewPort * vp, UWORD * colors, LONG count,
	    struct GfxBase * GfxBase)
{
    LONG t;

    for (t = 0; t < count; t ++ )
    {
	driver_SetRGB32 (vp, t
	    , (colors[t] & 0x0F00) << 20
	    , (colors[t] & 0x00F0) << 24
	    , (colors[t] & 0x000F) << 28
	    , GfxBase
	);
        
    }
} /* driver_LoadRGB4 */

void driver_LoadRGB32 (struct ViewPort * vp, ULONG * table,
	    struct GfxBase * GfxBase)
{
    LONG t;
    
    EnterFunc(bug("driver_LoadRGB32(vp=%p, table=%p)\n"));
    
    
    while (*table)
    {
        ULONG count, first;
	
	count = (*table) >> 16;
	first = *table & 0xFFFF;

	table ++;

	for (t=0; t<count; t++)
	{
	    driver_SetRGB32 (vp, t + first
		, table[0]
		, table[1]
		, table[2]
		, GfxBase
	    );

	    table += 3;
	}

    } /* while (*table) */
    ReturnVoid("driver_LoadRGB32");

} /* driver_LoadRGB32 */

struct BitMap * driver_AllocBitMap (ULONG sizex, ULONG sizey, ULONG depth,
	ULONG flags, struct BitMap * friend, struct GfxBase * GfxBase)
{
    struct BitMap * nbm;
    
    
    EnterFunc(bug("driver_AllocBitMap(sizex=%d, sizey=%d, depth=%d, flags=%d, friend=%p)\n",
    	sizex, sizey, depth, flags, friend));

    nbm = AllocMem (sizeof (struct BitMap), MEMF_ANY|MEMF_CLEAR);

    if (nbm)
    {
        Object *bm_obj;
        Object *gfxhidd;
	
	struct TagItem bm_tags[] =
	{
	    {aHidd_BitMap_Width,	0	},
	    {aHidd_BitMap_Height,	0	},
	    {aHidd_BitMap_Depth,	0	},
	    {aHidd_BitMap_Displayable,	0	},
	    {TAG_DONE,	0	}
	};
	
	D(bug("BitMap struct allocated\n"));
	
	/* Insert supplied values */
	bm_tags[0].ti_Data = sizex;
	bm_tags[1].ti_Data = sizey;
	bm_tags[2].ti_Data = depth;
	bm_tags[3].ti_Data = ((flags & BMF_DISPLAYABLE) ? TRUE : FALSE);

	
	gfxhidd  = SDD(GfxBase)->gfxhidd;
	D(bug("Gfxhidd: %p\n", gfxhidd));

    	/* Create HIDD bitmap object */
	if (gfxhidd)
	{
	    bm_obj = HIDD_Gfx_NewBitMap(gfxhidd, bm_tags);
	    D(bug("bitmap object: %p\n", bm_obj));

	    if (bm_obj)
	    {
	    
		/* Store it in plane array */
		BM_OBJ(nbm) = bm_obj;
		nbm->Rows   = sizey;
		nbm->BytesPerRow = ((sizex - 1) >> 3) + 1;
		nbm->Depth  = depth;
		nbm->Flags  = flags;
	    
	    
		ReturnPtr("driver_AllocBitMap", struct BitMap *, nbm);
	    }
	    
	} /* if (gfxhidd) */
	
	FreeMem(nbm, sizeof (struct BitMap));
	
    }

    ReturnPtr("driver_AllocBitMap", struct BitMap *, NULL);
}

static VOID setbitmapfast(struct BitMap *bm, LONG x_start, LONG y_start, LONG xsize, LONG ysize, ULONG pen)
{
    LONG modulo;
    LONG num_whole;
    UBYTE plane;
    UBYTE i;
    UBYTE pre_pixels_to_set,
    	  post_pixels_to_set; /* pixels to clear in pre and post byte */
 
    UBYTE prebyte_mask, postbyte_mask;
    
    pre_pixels_to_set  = 7 - (x_start & 0x07);
    post_pixels_to_set = (x_start + xsize - 1) & 0x07;
    num_whole = xsize - pre_pixels_to_set - post_pixels_to_set;

    num_whole >>= 3; /* number of bytes */

    modulo = bm->BytesPerRow - num_whole;
    
    if (pre_pixels_to_set == 0)
    {
        num_whole --;
	prebyte_mask = 0xFF;
    }
    else
    {
	/* Say we want to clear two pixels in last byte. We want the mask
	MSB 00000011 LSB
	*/
	prebyte_mask = 0;
	for (i = 0; i < pre_pixels_to_set; i ++ )
	{
	    prebyte_mask <<= 1;
    	    prebyte_mask |=  1;
    	}
	modulo ++;
    }
    
    if (post_pixels_to_set == 0)
    {
        num_whole --;
	prebyte_mask = 0xFF;
    }
    else
    {
	/* Say we want to set two pixels in last byte. We want the mask
	MSB 11000000 LSB
	*/
	postbyte_mask = 0;
	for (i = 0; i < post_pixels_to_set; i ++ )
	{
	    postbyte_mask <<= 1;
    	    postbyte_mask |=  1;
	}
    	postbyte_mask <<= (7 - post_pixels_to_set);
	modulo ++;
    }
    
    
    for (plane = 0; plane < GetBitMapAttr(bm, BMA_DEPTH); plane ++)
    {
    
        LONG y;
	UBYTE pixvals;
    	UBYTE *curbyte = ((UBYTE *)bm->Planes[plane]) + (x_start >> 3);
	
	/* Set or clear current bit of val ? */
	if (pen & (1L << plane))
	    pixvals = 0xFF;
	else
	    pixvals = 0x00;
	
	/* Set the pre and postmask */
	prebyte_mask  = (pixvals & prebyte_mask)  | (~prebyte_mask);  
	postbyte_mask = (pixvals & postbyte_mask) | (~prebyte_mask);
	
	for (y = y_start; y < ysize; y ++)
	{
	    LONG x;
	    /* Clear the first nonwhole byte */
	    *curbyte ++ &= prebyte_mask;
	    
	    for (x = 0; x < num_whole; x ++)
	    {
	        *curbyte ++ = pixvals;
	    }
	    /* Clear the last nonwhole byte */
	    *curbyte++ &= postbyte_mask;
	    
	    curbyte += modulo;
	}
	
    }
    return;
    
}



/* Minterms and GC drawmodes are in opposite order */
#define MINTERM_TO_GCDRMD(minterm) 	\
((  	  ((minterm & 0x80) >> 3)	\
	| ((minterm & 0x40) >> 1)	\
	| ((minterm & 0x20) << 1)	\
	| ((minterm & 0x10) << 3) )  >> 4 )

#define WIDTH_TO_BYTES(width) ((( (width) - 1) >> 3) + 1)
#define COORD_TO_BYTEIDX(x, y, bytes_per_row)	\
	( ((y) * (bytes_per_row)) + ((x) >> 3) )

#define XCOORD_TO_MASK(x) (1L << (7 - ((x) & 0x07)))

static VOID setbitmappixel(struct BitMap *bm
	, LONG x, LONG y
	, ULONG pen
	, UBYTE depth
	, UBYTE plane_mask)
{
    UBYTE i;
    ULONG idx;
    UBYTE mask, clr_mask;
    ULONG penmask;

    idx = COORD_TO_BYTEIDX(x, y, bm->BytesPerRow);

    mask = XCOORD_TO_MASK( x );
    clr_mask = ~mask;
    
    penmask = 1;
    for (i = 0; i < depth; i ++)
    {

	if ((1L << i) & plane_mask)
	{
            UBYTE *plane = bm->Planes[i];
	
	    if ((penmask & pen) != 0)
		plane[idx] |=  mask;
	    else
		plane[idx] &=  clr_mask;

	}
	penmask <<= 1;
	
    }
    return;
}

static ULONG getbitmappixel(struct BitMap *bm
	, LONG x
	, LONG y
	, UBYTE depth
	, UBYTE plane_mask)
{
    UBYTE i;
    ULONG idx;

    ULONG mask;
    ULONG pen = 0L;
    
    idx = COORD_TO_BYTEIDX(x, y, bm->BytesPerRow);
    mask = XCOORD_TO_MASK( x );
    
    for (i = depth - 1; depth ; i -- , depth -- )
    {
        if ((1L << i) & plane_mask)
	{
	    UBYTE *plane = bm->Planes[i];
	    pen <<= 1;
	
	    if ((plane[idx] & mask) != 0)
		pen |= 1;
	}
    }
    return pen;
}


struct blit_info
{
    struct BitMap *bitmap;
    ULONG minterm;
    ULONG planemask;
    UBYTE bmdepth;
    ULONG bmwidth;
    
};

#define BI(x) ((struct blit_info *)x)
static VOID bitmap_to_buf(APTR src_info
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, LONG width, LONG height
	, ULONG *bufptr)
{

    LONG y;
    
    /* Fill buffer with pixels from bitmap */
    for (y = 0; y < height; y ++)
    {
	LONG x;
	    
	for (x = 0; x < width; x ++)
	{
	    
	    *bufptr ++ = getbitmappixel(BI(src_info)->bitmap
		, x + x_src
		, y + y_src
		, BI(src_info)->bmdepth
		, BI(src_info)->planemask);
			

	}
	
    }

}


static VOID buf_to_bitmap(APTR dest_info
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, ULONG width, ULONG height
	, ULONG *bufptr)
{

    /*  Write pixels to the bitmap */
	
    /* Optimize bitmap copy */
	
    if (BI(dest_info)->minterm ==  0x00C0)
    {
	LONG y;
	for (y = 0; y < height; y ++)
	{
	    LONG x;
	    for (x = 0; x < width; x ++)
	    {

		setbitmappixel(BI(dest_info)->bitmap
		    	, x + x_dest
			, y + y_dest
			, *bufptr ++, BI(dest_info)->bmdepth, BI(dest_info)->planemask
		);


	    }
		
	}

    }
    else
    {
	LONG y;
	    
	for (y = 0; y < height; y ++)
	{
	    LONG x;
		
	    for (x = 0; x < width; x ++)
	    {
		ULONG src = *bufptr ++ , dest = 0;
		ULONG minterm = BI(dest_info)->minterm;

		/* Set the pixel using correct minterm */

		dest = getbitmappixel(BI(dest_info)->bitmap
			, x + x_dest
			, y + y_dest
			, BI(dest_info)->bmdepth
			, BI(dest_info)->planemask
		);

		if (minterm & 0x0010) dest  = ~src & ~dest;
		if (minterm & 0x0020) dest |= ~src & dest;
		if (minterm & 0x0040) dest |=  src & ~dest;
		if (minterm & 0x0080) dest |= src & dest;
		    
		setbitmappixel(BI(dest_info)->bitmap
			, x + x_dest
			, y + y_dest
			, dest, BI(dest_info)->bmdepth
			, BI(dest_info)->planemask
		);

	    }
		
	}
	    
    }
    return;

}

/* General functions for mving blocks of data to or from HIDDs, be it pixelarrays
  or bitmaps. They use a callback-function to get data from amiga/put data to amiga bitmaps/pixelarrays
  
*/	
static VOID amiga2hidd_fast(APTR src_info
	, LONG x_src , LONG	y_src
	, Object 	*hidd_bm
	, LONG x_dest, LONG y_dest
	, ULONG xsize, ULONG ysize
	, VOID (*fillbuf_hook)()
)
{
#define NUMPIX 2000 /* We can chunky2bitmap-convert 1000 pixels before writing pixarray */
#define DEPTH 8
    
    
    ULONG temp_buf[NUMPIX];
    ULONG tocopy_w,
    	  tocopy_h;
	  
    LONG pixels_left_to_process = xsize * ysize;
	  
    LONG current_x, current_y, next_x, next_y;

    
    next_x = 0;
    next_y = 0;
    
    while (pixels_left_to_process)
    {

	/* Get some more pixels from the HIDD */

	current_x = next_x;
	current_y = next_y;
	
	if (NUMPIX < xsize)
	{
	   /* buffer cant hold a single horizontal line, and must 
	      divide each line into several copy-operations */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMPIX;
		next_x += NUMPIX;
	    }
	    else
	    {	/* Start at a new line */
	    	next_x = 0;
		next_y ++;
	    }
	    tocopy_h = 1;
	    
    	}
	else /* We can copy one or several whole horizontal lines at a time */
	{
	    tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);

	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
	    
	
	}


	/* Get data */
	fillbuf_hook(src_info
		, current_x + x_src
		, current_y + y_src
		, current_x + x_dest
		, current_y + y_dest
		, tocopy_w, tocopy_h
		, temp_buf
	);
	
	/* Put it to the HIDD */
	D(bug("Putting box\n"));
	HIDD_BM_PutImage(hidd_bm
		, temp_buf
		, x_dest + current_x
		, y_dest + current_y
		, tocopy_w, tocopy_h);

	D(bug("Box put\n"));

	pixels_left_to_process -= (tocopy_w * tocopy_h);
	
	
    } /* while (pixels left to copy) */
    
    return;
    
}
	

static VOID hidd2amiga_fast(Object *hidd_bm
	, LONG x_src , LONG y_src
	, APTR dest_info
	, LONG x_dest, LONG y_dest
	, ULONG xsize, ULONG ysize
	, VOID (*putbuf_hook)()
)
{

    ULONG temp_buf[NUMPIX];
    ULONG tocopy_w, tocopy_h;
    
    LONG pixels_left_to_process = xsize * ysize;
    ULONG current_x, current_y, next_x, next_y;
    
    next_x = 0;
    next_y = 0;

    while (pixels_left_to_process)
    {
	
	current_x = next_x;
	current_y = next_y;
	
	if (NUMPIX < xsize)
	{
	   /* buffer cant hold a single horizontal line, and must 
	      divide each line into copies */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMPIX;
		next_x += NUMPIX;
	    }
	    else
	    {	/* Start at a new line */
	    
	    	next_x = 0;
		next_y ++;
	    }
	    tocopy_h = 1;
	    
    	}
    	else
    	{
	    tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);
	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
	    
    	}
	
	
	/* Get some more pixels from the HIDD */
	HIDD_BM_GetImage(hidd_bm
		, temp_buf
		, x_src + current_x
		, y_src + current_y
		, tocopy_w, tocopy_h);


	/*  Write pixels to the destination */
	putbuf_hook(dest_info
		, current_x + x_src
		, current_y + y_src
		, current_x + x_dest
		, current_y + y_dest
		, tocopy_w, tocopy_h
		, temp_buf
	);
	
	pixels_left_to_process -= (tocopy_w * tocopy_h);

    }
    
    return;
    
}


static VOID clearbitmapfast(struct BitMap *bm, LONG x_start, LONG y_start, LONG xsize, LONG ysize)
{
    LONG modulo;
    LONG num_whole;
    UBYTE plane;
    UBYTE i;
    UBYTE pre_pixels_to_clear,
    	  post_pixels_to_clear; /* pixels to clear in pre and post byte */
 
    UBYTE prebyte_mask, postbyte_mask;
    
    pre_pixels_to_clear  = 7 - (x_start & 0x07);
    post_pixels_to_clear = (x_start + xsize - 1) & 0x07;
    num_whole = xsize - pre_pixels_to_clear - post_pixels_to_clear;

    num_whole >>= 3; /* number of bytes */

    modulo = bm->BytesPerRow - num_whole;
    
    if (pre_pixels_to_clear == 0)
    {
        num_whole --;
	prebyte_mask = 0xFF;
    }
    else
    {
	/* Say we want to clear two pixels in last byte. We want the mask
	MSB 11111100 LSB
	*/
	prebyte_mask = 0;
	for (i = 0; i < pre_pixels_to_clear; i ++ )
	{
	    prebyte_mask <<= 1;
    	    prebyte_mask |=  1;
    	}
	modulo ++;
    }
    prebyte_mask = ~prebyte_mask; /* We want to clear  */
    
    if (post_pixels_to_clear == 0)
    {
        num_whole --;
	prebyte_mask = 0xFF;
    }
    else
    {
	/* Say we want to clear two pixels in last byte. We want the mask
	MSB 00111111 LSB
	*/
	postbyte_mask = 0;
	for (i = 0; i < post_pixels_to_clear; i ++ )
	{
	    postbyte_mask <<= 1;
    	    postbyte_mask |=  1;
	}
    	postbyte_mask <<= (7 - post_pixels_to_clear);
	modulo ++;
    }
    
    postbyte_mask = ~postbyte_mask; /* We want to clear */
    
    for (plane = 0; plane < GetBitMapAttr(bm, BMA_DEPTH); plane ++)
    {
        LONG y;
    	UBYTE *curbyte = ((UBYTE *)bm->Planes[plane]) + (x_start >> 3);
	
	for (y = y_start; y < ysize; y ++)
	{
	    LONG x;
	    /* Clear the first nonwhole byte */
	    *curbyte ++ &= prebyte_mask;
	    
	    for (x = 0; x < num_whole; x ++)
	    {
	        *curbyte ++ = 0;
	    }
	    /* Clear the last nonwhole byte */
	    *curbyte++ &= postbyte_mask;
	    
	    curbyte += modulo;
	}
	
    }
    return;
    
}


LONG driver_BltBitMap (struct BitMap * srcBitMap, LONG xSrc,
	LONG ySrc, struct BitMap * destBitMap, LONG xDest,
	LONG yDest, LONG xSize, LONG ySize, ULONG minterm,
	ULONG mask, PLANEPTR tempA, struct GfxBase * GfxBase)
{
    LONG planecnt = 0;
    
    ULONG wSrc, wDest;
    ULONG x;
    ULONG depth;
	

    EnterFunc(bug("driver_BltBitMap()\n"));
	
/*
kprintf("BltBitMap(%p, %d, %d, %p, %d, %d, %d, %d)\n"
		,srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize);
		
kprintf("Amiga to Amiga, wSrc=%d, wDest=%d\n",
		wSrc, wDest);
*/	
    wSrc  = GetBitMapAttr( srcBitMap, BMA_WIDTH);
    wDest = GetBitMapAttr(destBitMap, BMA_WIDTH);

    /* Clip all blits */

    depth = GetBitMapAttr ( srcBitMap, BMA_DEPTH);
    x     = GetBitMapAttr (destBitMap, BMA_DEPTH);
    if (x < depth)
	depth = x;

    /* Clip X and Y */
    if (xSrc < 0)
    {
	xDest += -xSrc;
	xSize -= -xSrc;
	xSrc = 0;
    }

    if (ySrc < 0)
    {
	yDest += -ySrc;
	ySize -= -ySrc;
	ySrc = 0;
    }

    /* Clip width and height for source and dest */
    if (ySrc + ySize > srcBitMap->Rows)
    {
	ySize = srcBitMap->Rows - ySrc;
    }

    if (yDest + ySize > destBitMap->Rows)
    {
	ySize = destBitMap->Rows - yDest;
    }

    if (xSrc + xSize >= wSrc)
    {
	xSize = wSrc - xSrc;
    }
        
    if (xDest + xSize >= wDest)
    {
    	xSize = wDest - xDest;
    }

    /* If the size is illegal or we need not copy anything, return */
    if (ySize <= 0 || xSize <= 0 || !mask)
	return 0;
    
    planecnt = depth;
    

/*    kprintf("driver_BltBitMap(%p, %d, %d, %p, %d, %d, %d, %d, %d, %d)\n",
    	srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, minterm, mask);
*/    
    /* The posibble cases:
	1) both src and dest is HIDD bitmaps
	2) src is HIDD bitmap, dest is amigabitmap.
     	3) srcBitMap is amiga bitmap, dest is HIDD bitmap.
	
	The case where both src & dest is amiga bitmap is handled
	by BltBitMap() itself.
    */
    	
    
    if (srcBitMap->Flags & BMF_AROS_DISPLAYED)
    {
        Object *src_bm = (Object *)BM_OBJ(srcBitMap);
	
    	if (destBitMap->Flags & BMF_AROS_DISPLAYED)
	{
	    Object *dst_bm = (Object *)BM_OBJ(destBitMap);
	    
	    /* Case 1. */
	    switch (minterm)
	    {
	    	case 0x00:  { /* Clear dest */
 		    struct TagItem tags[] =
		    {
		    	{aHidd_BitMap_Foreground,	0UL},
			{aHidd_BitMap_DrawMode,	vHIDD_GC_DrawMode_Copy},
			{TAG_DONE, 0UL}
		    };

		    SetAttrs(dst_bm, tags);
/* kprintf("clear HIDD bitmap\n");
*/		    
		    HIDD_BM_FillRect(dst_bm
		    	, xDest, yDest
			, xDest + xSize - 1
			, yDest + ySize - 1
		    );
			
		    break; }
		

		default: {
		    struct TagItem drmd_tags[] =
		    {
		    	{ aHidd_BitMap_DrawMode,	0UL},
			{ TAG_DONE, 0UL}
		    };
		    
		    drmd_tags[0].ti_Data = MINTERM_TO_GCDRMD(minterm);
		    SetAttrs(dst_bm, drmd_tags);

/* kprintf("copy HIDD to HIDD\n");
*/		    HIDD_BM_CopyBox( src_bm
		    	, xSrc, ySrc
			, dst_bm
			, xDest, yDest
			, xSize, ySize
		    );
		    break;  }
		    
	    } /* switch */
	    
	}
	else
	{
	    /* Case 2. */
	    switch (minterm)
	    {
	        case 0: /* Clear Amiga bitmap */
/* kprintf("clear amiga bitmap\n");
*/		    clearbitmapfast(destBitMap, xDest, yDest, xSize, ySize);
		    
		    break;
		    
		default: {
/* kprintf("copy HIDD to Amiga:\n");
*/
		    struct blit_info bi;
		    bi.bitmap	 = destBitMap;
		    bi.minterm	 = minterm;
		    bi.planemask = mask;
		    
		    bi.bmdepth	= GetBitMapAttr(destBitMap, BMA_DEPTH);
		    bi.bmwidth	= GetBitMapAttr(destBitMap, BMA_WIDTH);
		    
		    hidd2amiga_fast( BM_OBJ(srcBitMap)
		    	, xSrc, ySrc
			, (APTR) &bi
			, xDest, yDest
			, xSize, ySize
			, buf_to_bitmap
		    );
			
		    break; }
		
	    } /* switch (minterm) */
	    
	    
	}
    }
    else
    {
	Object *dst_bm = (Object *)BM_OBJ(destBitMap);
        
	/* Case 3. */
	switch (minterm)
	{

	    case 0: {/* Clear the destination */

 		    struct TagItem tags[] =
		    {
		    	{aHidd_BitMap_Foreground,	0UL},
			{aHidd_BitMap_DrawMode,	vHIDD_GC_DrawMode_Copy},
			{TAG_DONE, 0UL}
		    };

		    SetAttrs(dst_bm, tags);

/* kprintf("clear HIDD bitmap\n"); 
*/		    
		HIDD_BM_FillRect(dst_bm
		    , xDest, yDest
		    , xDest + xSize - 1
		    , yDest + ySize - 1
		);

	    
	    break; }
	    

	    default: {
		struct TagItem drmd_tags[] =
		{
		    { aHidd_BitMap_DrawMode,	0UL},
		    { TAG_DONE, 0UL}
		};
		
		struct blit_info bi;
		drmd_tags[0].ti_Data = MINTERM_TO_GCDRMD(minterm);
		SetAttrs(dst_bm, drmd_tags);

/* kprintf("copy Amiga to HIDD\n"); 
*/
		bi.bitmap	= srcBitMap;
		bi.minterm	= minterm;
		bi.planemask	= mask;
		
		bi.bmdepth	= GetBitMapAttr(srcBitMap, BMA_DEPTH);
		bi.bmwidth	= GetBitMapAttr(srcBitMap, BMA_WIDTH);

	    	amiga2hidd_fast( (APTR) &bi
			, xSrc, ySrc
			, dst_bm
			, xDest, yDest
			, xSize, ySize
			, bitmap_to_buf
		);
			
			
		break; }
	}
    
    }

    ReturnInt("driver_BltBitMap", LONG, planecnt);
}

void driver_BltClear (void * memBlock, ULONG bytecount, ULONG flags,
    struct GfxBase * GfxBase)
{
    
}

void driver_FreeBitMap (struct BitMap * bm, struct GfxBase * GfxBase)
{
    Object *gfxhidd = SDD(GfxBase)->gfxhidd;
    
    HIDD_Gfx_DisposeBitMap(gfxhidd, (Object *)BM_OBJ(bm));
    
    FreeMem(bm, sizeof (struct BitMap));
}


void driver_SetRGB32 (struct ViewPort * vp, ULONG color,
	    ULONG red, ULONG green, ULONG blue,
	    struct GfxBase * GfxBase)
{
   Object *bm_obj;
   HIDDT_Color hidd_col;
   
   EnterFunc(bug("driver_SetRGB32(vp=%p, color=%d, r=%x, g=%x, b=%x)\n",
   		vp, color, red, green, blue));
		
   /* Get bitmap object */
   
   bm_obj = BM_OBJ(vp->RasInfo->BitMap);
   
   D(bug("Bitmap obj: %p\n", bm_obj));
   hidd_col.red   = red   >> 16;
   hidd_col.green = green >> 16 ;
   hidd_col.blue  = blue  >> 16;
   
   HIDD_BM_SetColors(bm_obj, &hidd_col, color, 1);
   ReturnVoid("driver_SetRGB32");
   

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


/* Hook for moving data from pixelarray to chunky format buffer */
static VOID pixarray_to_buf(APTR src_info
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, ULONG width, ULONG height
	, ULONG *bufptr)
{
    UBYTE * pixarray = (UBYTE *)src_info;
    ULONG numpix = width * height;
    ULONG i;
    
    /* Get the pointer to the element we're currently working on */
    pixarray = &(pixarray[(y_src * width) + x_src]);
    
    for (i = 0; i < numpix; i ++)
    {
    	*bufptr ++ = (ULONG)*pixarray ++;
    }
    return;
}

static VOID buf_to_pixarray(APTR src_info
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, ULONG width, ULONG height
	, ULONG *bufptr)
{
    UBYTE * pixarray = (UBYTE *)src_info;
    ULONG numpix = width * height;
    ULONG i;
    
    /* Get the pointer to the element we're currently working on */
    pixarray = &(pixarray[(y_dest * width) + x_dest]);
    
    for (i = 0; i < numpix; i ++)
    {
    	*pixarray ++ = (UBYTE)*bufptr ++;
    }
    return;
}


		
LONG driver_WritePixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata *dd;
    struct BitMap *bm = rp->BitMap;
    struct Layer *L = rp->Layer;
    ULONG array_width, array_height;
    
    LONG pixwritten = 0;
    
    EnterFunc(bug("driver_WritePixelArray8(%p, %d, %d, %d, %d)\n",
    	rp, xstart, ystart, xstop, ystop));
    
    if (!CorrectDriverData (rp, GfxBase))
	return 0;
	
    dd = GetDriverData(rp);
    
    array_width  = xstop - xstart + 1;
    array_height = ystop - ystart + 1;
    
    
    if (NULL == L)
    {
    	struct TagItem bm_tags[] =
	{
	    { aHidd_BitMap_DrawMode, vHIDD_GC_DrawMode_Copy},
	    { TAG_DONE, 0}
	};
        /* No layer, probably a screen */
	
	/* Just put the pixelarray directly onto the HIDD */
	
	SetAttrs(BM_OBJ(bm), bm_tags);
	
	amiga2hidd_fast((APTR)array
		, 0, 0
		, BM_OBJ(bm)
		, xstart, ystart
		, array_width, array_height
		, pixarray_to_buf
	);
	pixwritten += array_width * array_height;
	
    }
    else
    {
    	/* Window rastport, we need to clip th operation */
	
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle towrite, intersect;
	
	towrite.MinX = xstart + xrel;
	towrite.MinY = ystart + yrel;
	towrite.MaxX = xstop  + xrel;
	towrite.MaxY = ystop  + yrel;
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to blit ? */
	    if (andrectrect(&CR->bounds, &towrite, &intersect))
	    {
	        ULONG inter_width  = intersect.MaxX - intersect.MinX + 1;
		ULONG inter_height = intersect.MaxY - intersect.MinY + 1;
		LONG array_rel_x = intersect.MinX - towrite.MinX;
		LONG array_rel_y = intersect.MinY - towrite.MinY;
		
	        if (NULL == CR->lobs)
		{
		    struct TagItem bm_tags[] =
		    {
			{ aHidd_BitMap_DrawMode, vHIDD_GC_DrawMode_Copy},
			{ TAG_DONE, 0}
		    };
		    
		    D(bug("non-obscured cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    /* Write to the HIDD */
	
		    SetAttrs(BM_OBJ(bm), bm_tags);
	
		    amiga2hidd_fast((APTR)array
			, array_rel_x
			, array_rel_y
			, BM_OBJ(bm)
			, intersect.MinX, intersect.MinY
			, inter_width, inter_height
			, pixarray_to_buf
		    );
		}
		else
		{
		    /* This is the tricky one: render into offscreen cliprect bitmap */
		    UBYTE depth = GetBitMapAttr(CR->BitMap, depth);
		    
		    LONG cr_rel_x	= intersect.MinX - CR->bounds.MinX;
		    LONG cr_rel_y	= intersect.MinY - CR->bounds.MinY;
		    
		    UBYTE *array_ptr	= array + (array_rel_y * array_width) + array_rel_x;
		    
		    ULONG modulo = array_width - inter_width;
		    
		    LONG y;
		    
		    for (y = 0; y < inter_height; y ++)
		    {
		    	LONG x;
		    	for (x = 0; x < inter_width; x ++)
			{
			    setbitmappixel(CR->BitMap
			    	, cr_rel_x + x
				, cr_rel_y + y
				
				, *array_ptr ++
				, depth
				, 0xFF /* All planes */
			    );
			}
			array_ptr += modulo;
		    }
		    
		} /* if (intersecton inside hidden cliprect) */
		
	        pixwritten += inter_width * inter_height;

		
	    } /* if (cliprect intersects with area we want to draw to) */
	    
	    CR = CR->Next;
	} /* while (cliprects to examine) */
	
    } /* if (not screen rastport) */
	
    ReturnInt("driver_WritePixelArray8", LONG, pixwritten);
    
} /* driver_WritePixelArray8 */


LONG driver_ReadPixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata *dd;
    struct BitMap *bm = rp->BitMap;
    struct Layer *L = rp->Layer;
    ULONG array_width, array_height;
    
    LONG pixread = 0;
    
    EnterFunc(bug("driver_ReadPixelArray8(%p, %d, %d, %d, %d)\n",
    	rp, xstart, ystart, xstop, ystop));
    
    if (!CorrectDriverData (rp, GfxBase))
	return 0;
	
    dd = GetDriverData(rp);
    
    array_width  = xstop - xstart + 1;
    array_height = ystop - ystart + 1;
    
    
    if (NULL == L)
    {
        /* No layer, probably a screen */
	
	/* Just get the pixelarray directly from the HIDD */
	
	hidd2amiga_fast(BM_OBJ(bm)
		, xstart, ystart
		, (APTR)array
		, 0, 0
		, array_width, array_height
		, buf_to_pixarray
	);
	pixread += array_width * array_height;
	
    }
    else
    {
    	/* Window rastport, we need to clip th operation */
	
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle toread, intersect;
	
	toread.MinX = xstart + xrel;
	toread.MinY = ystart + yrel;
	toread.MaxX = xstop  + xrel;
	toread.MaxY = ystop  + yrel;
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to blit ? */
	    if (andrectrect(&CR->bounds, &toread, &intersect))
	    {
	        ULONG inter_width  = intersect.MaxX - intersect.MinX + 1;
		ULONG inter_height = intersect.MaxY - intersect.MinY + 1;
		LONG array_rel_x = intersect.MinX - toread.MinX;
		LONG array_rel_y = intersect.MinY - toread.MinY;
		
	        if (NULL == CR->lobs)
		{
		    
		    D(bug("non-obscured cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    /* Read from the HIDD */
	
		    amiga2hidd_fast(BM_OBJ(bm)
			, intersect.MinX, intersect.MinY
			, (APTR)array
			, array_rel_x
			, array_rel_y
			, inter_width, inter_height
			, pixarray_to_buf
		    );
		}
		else
		{
		    /* This is the tricky one: render into offscreen cliprect bitmap */
		    UBYTE depth = GetBitMapAttr(CR->BitMap, depth);
		    LONG cr_rel_x = intersect.MinX - CR->bounds.MinX;
		    LONG cr_rel_y = intersect.MinY - CR->bounds.MinY;
		    
		    LONG y;
		    UBYTE *array_ptr = array + (array_rel_y * array_width) + array_rel_x;
		    ULONG modulo = array_width - inter_width;
		    
		    for (y = 0; y < inter_height; y ++)
		    {
		    	LONG x;
		    	for (x = 0; x < inter_width; x ++)
			{
			    *array_ptr ++ = getbitmappixel(CR->BitMap
			    	, cr_rel_x + x
				, cr_rel_y + y
				, depth
				, 0xFF /* All planes */
			    );
			}
			array_ptr += modulo;
			
		    }
		    
		} /* if (intersecton inside hidden cliprect) */
		
	        pixread += inter_width * inter_height;

		
	    } /* if (cliprect intersects with area we want to draw to) */
	    
	    CR = CR->Next;
	} /* while (cliprects to examine) */
	
    } /* if (not screen rastport) */
	
    ReturnInt("driver_ReadPixelArray8", LONG, pixread);
    
} /* driver_WritePixelArray8 */


static VOID blttemplate_amiga(PLANEPTR source, LONG x_src, LONG modulo, struct BitMap *dest
	, LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, struct RastPort *rp, struct GfxBase *GfxBase)
{
    UBYTE *srcptr;
    UBYTE dest_depth = GetBitMapAttr(dest, BMA_DEPTH);
    UWORD drmd = GetDrMd(rp);
    UBYTE apen = GetAPen(rp);
    UBYTE bpen = GetBPen(rp);
    LONG x, y;

    /* Find the exact startbyte */
    srcptr = source + (x_src >> 3);
    
    /* Find the exact startbit */
    x_src &= 0x07;
    

    for (y = 0; y < ysize; y ++)
    {
    	for (x = 0; x < xsize; x ++)
	{
	    UBYTE pen;
	    UBYTE mask = 1 << (7 - ((x + x_src) & 0x07));
	    BOOL is_set = ((*srcptr & mask) ? TRUE : FALSE);
	    BOOL set_pixel = FALSE;
	    
	    if (drmd & INVERSVID)
	    {
	    	is_set = ((is_set == TRUE) ? FALSE : TRUE);
	    }
	    if (drmd & JAM1)
	    {
	    	/* Only use apen if pixel is set */
		if (is_set)
		{
		    pen = apen;
		    set_pixel = TRUE;
		}
		    
	    }
	    else if (drmd & JAM2)
	    {
	    	/* Use apen if pixel is et, bpen otherwise */
		if (is_set)
		    pen = apen;
		else
		    pen = bpen;
		    
		set_pixel = TRUE;
		
	    }
	    else if (drmd & COMPLEMENT)
	    {
		
	    	pen = getbitmappixel(dest
			, x + x_dest
			, y + y_dest, dest_depth
			, 0xFF
		);
		
		pen = ~pen;

		
	    }
	    if (set_pixel)
	    {
		setbitmappixel(dest
			, x + x_dest
			, y + y_dest
			, dest_depth, 0xFF
			, pen
		);
	    }

	
	    /* Last pixel in this byte ? */
	    if (((x + x_src) & 0x07) == 0x07)
	    	srcptr ++;
		
	}
	srcptr += modulo;
    }
    return;
}	


struct template_info
{
    PLANEPTR source;
    LONG x_src;
    LONG modulo;
    BOOL invertsrc;
    
    
};

VOID template_to_buf(struct template_info *ti
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, ULONG xsize, ULONG ysize
	, ULONG *buf)
{
    UBYTE *srcptr;
    LONG x, y;
    
    EnterFunc(bug("template_to_buf(%p, %d, %d, %d, %d, %p)\n"
    			, ti, x_src, y_src, xsize, ysize, buf));
    /* Find the exact startbyte */
    srcptr = ti->source + (x_src >> 3) + (ti->modulo * y_src);
    
    D(bug("offset (in bytes): %d, modulo=%d, y_src=%d, x_src=%d\n"
    	, (x_src >> 3) + (ti->modulo * y_src), ti->modulo, y_src, x_src ));
    /* Find the exact startbit */
    
    x_src &= 0x07;
    
    
    D(bug("new x_src: %d, srcptr=%p\n", x_src, srcptr));

    for (y = 0; y < ysize; y ++)
    {
	UBYTE *byteptr = srcptr;
    	for (x = 0; x < xsize; x ++)
	{
	    UBYTE mask = 1 << (7 - ((x + x_src) & 0x07));
	    BOOL is_set = ((*byteptr & mask) ? TRUE : FALSE);
	    
	    if (ti->invertsrc)
	    {
	    	is_set = ((is_set == TRUE) ? FALSE : TRUE);
	    }
	    
	    if (is_set)
		*buf = 1UL;
	    else
		*buf = 0UL;
 D(bug("%d", *buf));
	    buf ++;

	    /* Last pixel in this byte ? */
	    if (((x + x_src) & 0x07) == 0x07)
	    {
	    	byteptr ++;
	    }
		
	}
	D(bug("\n"));
	srcptr += ti->modulo;
    }
    
    D(bug("srcptr is %p\n", srcptr));
    ReturnVoid("template_to_buf");
}

VOID driver_BltTemplate(PLANEPTR source, LONG xSrc, LONG srcMod, struct RastPort * destRP,
	LONG xDest, LONG yDest, LONG xSize, LONG ySize, struct GfxBase *GfxBase)
{
    struct gfx_driverdata *dd;
    struct Layer *L = destRP->Layer;
    struct BitMap *bm = destRP->BitMap;
    ULONG width, height;
    Object *template_bm;
    struct template_info ti;
    
    struct TagItem setgc_tags[] =
    {
    	{aHidd_BitMap_GC, 0UL},
	{TAG_DONE, 0UL}
    };
    struct TagItem bm_tags[] = 
    {
    	{ aHidd_BitMap_Width,	xSize },
	{ aHidd_BitMap_Height,	ySize },
	{ aHidd_BitMap_Depth,	1 },
	{ aHidd_BitMap_Displayable, FALSE },
	{ TAG_DONE, 0UL }
    };

    
    EnterFunc(bug("driver_BltTemplate(%d, %d, %d, %d, %d, %d)\n"
    	, xSrc, srcMod, xDest, yDest, xSize, ySize));
	
    if (!CorrectDriverData(destRP, GfxBase))
    	ReturnVoid("driver_BltTemplate");	
	
    dd = GetDriverData(destRP);
    
    /* Prepare for setting bitmap GC */
    setgc_tags[0].ti_Data = (IPTR)dd->dd_GC;
    SetAttrs( BM_OBJ(bm), setgc_tags );

    width  = GetBitMapAttr(bm, BMA_WIDTH);
    height = GetBitMapAttr(bm, BMA_HEIGHT);
    
    /* Create an offscreen HIDD bitmap of depth 1 to use in color expansion */
    template_bm = HIDD_Gfx_NewBitMap(SDD(GfxBase)->gfxhidd, bm_tags);
    if (!template_bm)
    	ReturnVoid("driver_BltTemplate");
	
    /* Copy contents from Amiga bitmap to the offscreen HIDD bitmap */
    ti.source	 = source;
    ti.modulo	 = srcMod;
    ti.invertsrc = ((GetDrMd(destRP) & INVERSVID) ? TRUE : FALSE);

D(bug("Copying template to HIDD offscreen bitmap\n"));    

    amiga2hidd_fast( (APTR)&ti
    	, xSrc, 0
	, template_bm
	, 0, 0
	, xSize, ySize
	, template_to_buf
    );


D(bug("Done Copying template to HIDD offscreen bitmap\n"));    

    if (NULL == L)
    {
        /* No layer, probably a screen */
	
	/* Blit with color expansion */
	HIDD_BM_BlitColorExpansion( BM_OBJ(bm)
		, template_bm
		, 0, 0
		, xDest, yDest
		, xSize, ySize);
	
    }
    else
    {
    	/* Window rastport, we need to clip the operation */
	
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle toblit, intersect;
	
	toblit.MinX = xDest + xrel;
	toblit.MinY = yDest + yrel;
	toblit.MaxX = (xDest + xSize - 1) + xrel;
	toblit.MaxY = (yDest + ySize - 1) + yrel;
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to blit ? */
	    if (andrectrect(&CR->bounds, &toblit, &intersect))
	    {
		    
		
	        if (NULL == CR->lobs)
		{
		    LONG  clipped_xsrc, clipped_ysrc;
		    clipped_xsrc = /* xsrc = 0 + */ intersect.MinX - toblit.MinX;
		    clipped_ysrc = /* ysrc = 0 + */ intersect.MinY - toblit.MinY;
		    D(bug("non-obscured cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    HIDD_BM_BlitColorExpansion( BM_OBJ(bm)
			, template_bm
			, clipped_xsrc, clipped_ysrc
			, intersect.MinX
			, intersect.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
		    );
		    
		}
		else
		{
		    UBYTE *clipped_source;
		    LONG clipped_xsrc;
		    
		    /* This is the tricky one: render into offscreen cliprect bitmap */

		    clipped_xsrc = xSrc + (intersect.MinX - toblit.MinX);
		    clipped_source = clipped_source + (((clipped_xsrc - 1) >> 4) + 1);
		    clipped_xsrc &= 0x0F;

		    blttemplate_amiga(clipped_source
		    	, clipped_xsrc
			, srcMod
			, CR->BitMap
			, intersect.MinX - CR->bounds.MinX
			, intersect.MinY - CR->bounds.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, destRP, GfxBase
		    );
		    
		    	

		}
		
	    } /* if (cliprect intersects with area we want to draw to) */
	    
	    CR = CR->Next;
	} /* while (cliprects to examine) */
	
    } /* if (not screen rastport) */
    
    HIDD_Gfx_DisposeBitMap(SDD(GfxBase)->gfxhidd, template_bm);
	
    ReturnVoid("driver_BltTemplate");
}


struct pattern_info
{
    PLANEPTR mask;
    struct RastPort *rp;
    LONG mask_xmin;
    LONG mask_ymin;
    LONG mask_bpr; /* Butes per row */
    UBYTE dest_depth;
    struct GfxBase * gfxbase;
    Object *destbm;
    
};

#define GfxBase (pi->gfxbase)
static VOID pattern_to_buf(struct pattern_info *pi
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, ULONG xsize, ULONG ysize
	, ULONG *buf)
{

    /* x_src, y_src is the coordinates int the layer. */
    LONG y;
    struct RastPort *rp = pi->rp;
    
    ULONG drmd = GetDrMd(rp);
    ULONG apen = GetAPen(rp);
    ULONG bpen = GetBPen(rp);
    UBYTE *apt = (UBYTE *)rp->AreaPtrn;
    
    ULONG pattern_height = 1L << ABS(rp->AreaPtSz);

    EnterFunc(bug("pattern_to_buf(%p, %d, %d, %d, %d, %p)\n"
    			, pi, x_src, y_src, xsize, ysize, buf ));
			

    if ((drmd & JAM2) == 0)
    {
    	/* We must get the data from the destination bitmap */
	HIDD_BM_GetImage(pi->destbm, buf, x_dest, y_dest, xsize, ysize);
    }
			
    
    for (y = 0; y < ysize; y ++)
    {
        LONG x;
	
	for (x = 0; x < xsize; x ++)
	{
	    ULONG set_pixel;
	    ULONG pixval;
	    
	    /* Mask supplied ? */
	    if (pi->mask)
	    {
		ULONG idx, mask;
		idx = COORD_TO_BYTEIDX(x + pi->mask_xmin, y + pi->mask_ymin, pi->mask_bpr);
		mask = XCOORD_TO_MASK(x + pi->mask_xmin);
		 
		 
		set_pixel = pi->mask[idx] & mask;
		 
	    }
	    else
	        set_pixel = 1UL;
		
		
	    if (set_pixel)
	    {
	   
	
		if (apt)
		{
		    ULONG idx, mask;
		    
		    /* This rp has area pattern. Get pattern offset.
		       Patterns are allways 16 bits wide.
		    */

		    
		    idx = COORD_TO_BYTEIDX((x + x_src) & 0x0F, (y + y_src) & (pattern_height - 1), 2);
		    mask = XCOORD_TO_MASK(x + x_src);
		    
/*		    D(bug("idx: %d, mask: %d\n", idx, mask));
*/		    
		    /* Mono- or multicolor ? */
		    if (rp->AreaPtSz > 0)
		    {
		    	/* mono */
			set_pixel = apt[idx] & mask;
			if (drmd & INVERSVID)
			    set_pixel = ((set_pixel != 0) ? 0UL : 1UL );
			
			if (set_pixel)
			{
			    /* Use FGPen to render */
			    pixval = apen;
			}
			else
			{
			    if (drmd & JAM2)
			    {
			    	pixval = bpen;
				set_pixel = 1UL;
			    }
			    else
			    {   
			        /* Do not set pixel */
			    	set_pixel = 0UL;
			    }
			
			}

			
		    }
		    else
		    {
		        UBYTE i, depth;
			ULONG plane_size, pen_mask;
			UBYTE *plane;
			
			plane_size = (/* bytesperrow = */ 2 ) * pattern_height;
			depth = pi->dest_depth;
			plane = apt;
			
		    	/* multicolored pattern, get pixel from all planes */
			for (i = 0; i < depth; i ++)
			{

			    pen_mask <<= 1;
	
			    if ((plane[idx] & mask) != 0)
				pixval |= pen_mask;
			}
			
			set_pixel = TRUE;
		   }
		    
		   if (set_pixel)
		   {
		   	D(bug(" s"));
		    	*buf = pixval;
		   }
		   else
/*		   	*buf ++ =  Keep old value */

			
		   D(bug("(%d, %d): %d", x, y, *buf));
		   buf ++;
		}
	    
	    } /* if (pixel should be set */
	    
	    
	} /* for (each column) */
	
    } /* for (each row) */

    
    ReturnVoid("pattern_to_buf");
}


#undef GfxBase

static VOID bltpattern_amiga(struct pattern_info *pi
		, struct BitMap *dest_bm
		, LONG x_src, LONG y_src	/* offset into layer */
		, LONG x_dest, LONG y_dest	/* offset into bitmap */
		, ULONG xsize, LONG ysize
		, struct GfxBase *GfxBase
)
{

    /* x_src, y_src is the coordinates int the layer. */
    LONG y;
    struct RastPort *rp = pi->rp;
    
    ULONG drmd = GetDrMd(rp);
    ULONG apen = GetAPen(rp);
    ULONG bpen = GetBPen(rp);
    UBYTE *apt = (UBYTE *)rp->AreaPtrn;
    
    ULONG pattern_height = 1L << ABS(rp->AreaPtSz);
    UBYTE dest_depth = GetBitMapAttr(dest_bm, BMA_DEPTH);
    
    for (y = 0; y < ysize; y ++)
    {
        LONG x;
	
	for (x = 0; x < xsize; x ++)
	{
	    ULONG set_pixel;
	    ULONG pixval;
	    
	    
	    /* Mask supplied ? */
	    if (pi->mask)
	    {
		ULONG idx, mask;
		idx = COORD_TO_BYTEIDX(x + pi->mask_xmin, y + pi->mask_ymin, pi->mask_bpr);
		mask = XCOORD_TO_MASK(x + pi->mask_xmin);
		 
		set_pixel = pi->mask[idx] & mask;
		 
	    }
	    else
	        set_pixel = 1UL;
		
		
	    if (set_pixel)
	    {
	   
	
		if (apt)
		{
		    ULONG idx, mask;
		    
		    /* This rp has area pattern. Get pattern offset.
		       Patterns are allways 16 bits wide.
		    */

		    idx = COORD_TO_BYTEIDX((x + x_src) & 0x0F, (y + y_src) & (pattern_height - 1), 2);
		    mask = XCOORD_TO_MASK(x + x_src);
		    
/*		    D(bug("idx: %d, mask: %d\n", idx, mask));
*/		    
		    /* Mono- or multicolor ? */
		    if (rp->AreaPtSz > 0)
		    {
		    	/* mono */
			set_pixel = apt[idx] & mask;
			if (drmd & INVERSVID)
			    set_pixel = ((set_pixel != 0) ? 0UL : 1UL );
			
			if (set_pixel)
			{
			    /* Use FGPen to render */
			    pixval = apen;
			}
			else
			{
			    if (drmd & JAM2)
			    {
			    	pixval = bpen;
				set_pixel = 1UL;
			    }
			    else
			    {   
			        /* Do not set pixel */
			    	set_pixel = 0UL;
			    }
			
			}

			
		    }
		    else
		    {
		        UBYTE i, depth;
			ULONG plane_size, pen_mask;
			UBYTE *plane;
			
			plane_size = (/* bytesperrow = */ 2 ) * pattern_height;
			depth = pi->dest_depth;
			plane = apt;
			
		    	/* multicolored pattern, get pixel from all planes */
			for (i = 0; i < depth; i ++)
			{

			    pen_mask <<= 1;
	
			    if ((plane[idx] & mask) != 0)
				pixval |= pen_mask;
			}
			
			set_pixel = TRUE;
		   }
		    
		   if (set_pixel)
		   {
		    	setbitmappixel(dest_bm, x + x_dest, y + y_dest, pixval, dest_depth, 0xFF);
		   }
		   
		}
	    
	    } /* if (pixel should be set */
	    
	    
	} /* for (each column) */
	
    } /* for (each row) */
    
    return;

}    

VOID driver_BltPattern(struct RastPort *rp, PLANEPTR mask, LONG xMin, LONG yMin,
		LONG xMax, LONG yMax, ULONG byteCnt, struct GfxBase *GfxBase)
{

    struct gfx_driverdata *dd;
    struct Layer *L = rp->Layer;
    struct BitMap *bm = rp->BitMap;
    ULONG width, height;
    
    struct pattern_info pi;
    
    struct TagItem setgc_tags[] =
    {
    	{aHidd_BitMap_GC, 0UL},
	{TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("driver_BltPattern(%d, %d, %d, %d, %d)\n"
    	, xMin, yMin, xMax, yMax, byteCnt));
	
	
    if (!CorrectDriverData(rp, GfxBase))
    	ReturnVoid("driver_BltPattern");

    pi.mask	= mask;
    pi.rp	= rp;
    pi.gfxbase	= GfxBase;
    pi.mask_bpr = byteCnt;
    pi.dest_depth	= GetBitMapAttr(rp->BitMap, BMA_DEPTH);
    pi.destbm	= BM_OBJ(bm);
	
    dd = GetDriverData(rp);
    
    width  = xMax - xMin + 1;
    height = yMax - yMin + 1;
    
    /* Prepare for setting bitmap GC */
    setgc_tags[0].ti_Data = (IPTR)dd->dd_GC;
    SetAttrs( BM_OBJ(bm), setgc_tags );
    
    
    if (NULL == L)
    {
        /* No layer, probably a screen */
	
	pi.mask_xmin = 0;
	pi.mask_ymin = 0;
	
	amiga2hidd_fast( (APTR) &pi
		, 0, 0
		, BM_OBJ(bm)
		, xMin, yMin
		, width
		, height
		, pattern_to_buf
	);
	
	
    }
    else
    {
    	/* Window rastport, we need to clip the operation */
	
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle toblit, intersect;
	
	toblit.MinX = xMin + xrel;
	toblit.MinY = yMin + yrel;
	toblit.MaxX = xMax + xrel;
	toblit.MaxY = yMax + yrel;
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to blit ? */
	    if (andrectrect(&CR->bounds, &toblit, &intersect))
	    {
		    
		
	        if (NULL == CR->lobs)
		{
		
		    
		    pi.mask_xmin = intersect.MinX - toblit.MinX;
		    pi.mask_ymin = intersect.MinY - toblit.MinY;
		    
D(bug("amiga2hidd_fast(xmin=%d, ymin=%d, destx=%d, desty=%d, w=%d, h=%d)\n"
			, xMin, yMin 
			, intersect.MinX, intersect.MaxX
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
		    ));
		    amiga2hidd_fast( (APTR) &pi
			, xMin, yMin 
			, BM_OBJ(bm)
			, intersect.MinX, intersect.MaxX
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, pattern_to_buf
		    );
		    
		}
		else
		{
		    bltpattern_amiga( &pi
		    	, CR->BitMap
			, xMin, yMin
			, intersect.MinX - CR->bounds.MinX
			, intersect.MinY - CR->bounds.MinY
			, intersect.MaxX - intersect.MinX + 1
			, intersect.MaxY - intersect.MinY + 1
			, GfxBase
		    );
		    
		    	

		}
		
	    } /* if (cliprect intersects with area we want to draw to) */
	    
	    CR = CR->Next;
	} /* while (cliprects to examine) */
	
    } /* if (not screen rastport) */
	
    ReturnVoid("driver_BltPattern");
}



VOID driver_WriteChunkyPixels(struct RastPort * rp, ULONG xstart, ULONG ystart,
		ULONG xstop, ULONG ystop, UBYTE * array,
		LONG bytesperrow, struct GfxBase *GfxBase)
{
    driver_WritePixelArray8(rp 
		, xstart, ystart
		, xstop, ystop
		, array, NULL
		, GfxBase
    );

}

LONG driver_ReadPixelLine8 (struct RastPort * rp, ULONG xstart,
			    ULONG ystart, ULONG width,
			    UBYTE * array, struct RastPort * temprp,
			    struct GfxBase *GfxBase)
{
    /* We are lazy, and waste som cycles to be able to reuse what we've
       allready done
     */
    
    return driver_ReadPixelArray8(rp
    	, xstart, ystart
	, xstart + width - 1, 1
	, array, temprp
	, GfxBase
    );
}		    


LONG driver_WritePixelLine8 (struct RastPort * rp, ULONG xstart,
			    ULONG ystart, ULONG width,
			    UBYTE * array, struct RastPort * temprp,
			    struct GfxBase *GfxBase)
{
    /* We are lazy, and waste som cycles to be able to reuse what we've
       allready done
     */
    return driver_WritePixelArray8(rp
    	, xstart, ystart
	, xstart + width - 1, 1
	, array, temprp
	, GfxBase
    );
}


struct layerhookmsg
{
    struct Layer *Layer;
/*  struct Rectangle rect; (replaced by the next line!) */
    WORD MinX, MinY, MaxX, MaxY;
    LONG OffsetX, OffsetY;
};


VOID calllayerhook(struct Hook *h, struct RastPort *rp, struct layerhookmsg *msg)
{
    struct BitMap *bm = rp->BitMap;

    if(h == LAYERS_BACKFILL)
    {
        /* Use default backfill */
	if (bm->Flags & BMF_AROS_DISPLAYED)
	{
	     struct TagItem bm_tags[] =
	     {
	     	{aHidd_BitMap_Foreground, 0UL},
		{aHidd_BitMap_DrawMode,	  vHIDD_GC_DrawMode_Copy},
		{TAG_DONE, 0UL}
	     };

	     SetAttrs(BM_OBJ(bm), bm_tags);
		    
		    /* Cliprect not obscured, so we may render directly into the display */
	     HIDD_BM_FillRect(BM_OBJ(bm)
		, msg->MinX, msg->MinY
		, msg->MaxX, msg->MaxY
	     );
	}
	else
	{
	    clearbitmapfast(rp->BitMap
	    	, msg->MinX, msg->MinY
		, msg->MaxX - msg->MinX + 1
		, msg->MaxY - msg->MinY + 1
	    );
	}
    }

    else if(h != LAYERS_NOBACKFILL)
    {
	/* Call user specified hook */
	AROS_UFC3(void, h->h_Entry,
	    AROS_UFCA(struct Hook *,         h,    A0),
	    AROS_UFCA(struct RastPort *,     rp,   A2),
	    AROS_UFCA(struct layerhookmsg *, msg, A1)
	);
    }
}

void driver_EraseRect (struct RastPort * rp, LONG x1, LONG y1, LONG x2, LONG y2,
		    struct GfxBase * GfxBase)
{

    ULONG width, height;
    struct Layer *L = rp->Layer;
    struct BitMap *bm = rp->BitMap;
    
    EnterFunc(bug("driver_EraseRect(%d, %d, %d, %d)\n", x1, y1, x2, y2));
    if (!CorrectDriverData(rp, GfxBase))
    	ReturnVoid("driver_EraseRect(No driverdata)");

    width  = GetBitMapAttr(bm, BMA_WIDTH);
    height = GetBitMapAttr(bm, BMA_HEIGHT);
    
    if (NULL == L)
    {
	struct layerhookmsg msg;
	
	clipagainstbitmap(bm, &x1, &y1, &x2, &y2, GfxBase);
	
	
	/* Use the layerinfo hook */
	msg.Layer = NULL;
	msg.MinX = x1;
	msg.MinY = y1;
	msg.MaxX = x2;
	msg.MaxY = y2;
	
	/* Hook should not use these */
	msg.OffsetX = 0;
	msg.OffsetY = 0;
	
#warning TODO
/*  From where do I get the layerinfo->BlankHook ?
    Maybe put it into rp->LongReserved[1] in
    OpenScreen() ? (Or really inside intui_OpenScreen() )
	calllayerhook(struct Hook *???, rp, &msg);
*/
	
    }
    else
    {
        struct ClipRect *CR = L->ClipRect;
	WORD xrel = L->bounds.MinX;
        WORD yrel = L->bounds.MinY;
	struct Rectangle toerase, intersect;
	struct layerhookmsg msg;
	
	toerase.MinX = x1 + xrel;
	toerase.MinY = y1 + yrel;
	toerase.MaxX = x2 + xrel;
	toerase.MaxY = y2 + yrel;
	
	msg.Layer = L;
	
#warning TODO
/* What should these be ? */	
	msg.OffsetX = 0;
	msg.OffsetY = 0;
	
	
	while (NULL != CR)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to rectfill ? */
	    if (andrectrect(&CR->bounds, &toerase, &intersect))
	    {
	    
	        if (NULL == CR->lobs)
		{
		    D(bug("non-obscured cliprect, intersect= (%d,%d,%d,%d)\n"
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    ));
		    
		    msg.MinX = intersect.MinX;
		    msg.MinY = intersect.MinY;
		    msg.MaxX = intersect.MaxX;
		    msg.MaxY = intersect.MaxY;
		    
		    calllayerhook(L->BackFill, rp, &msg);
		}
		else
		{
		    msg.MinX = intersect.MinX - CR->bounds.MinX;
		    msg.MinY = intersect.MinY - CR->bounds.MinY;
		    msg.MaxX = intersect.MaxX - CR->bounds.MaxX;
		    msg.MaxY = intersect.MaxY - CR->bounds.MaxY;
		    
		    calllayerhook(L->BackFill, rp, &msg);

		}

	    }
	    CR = CR->Next;
	}
	

    }
    ReturnVoid("driver_EraseRect");

}


