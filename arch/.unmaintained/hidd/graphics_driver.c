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
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"
#include "graphics_internal.h"


#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>


#define PEN_BITS    4
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)


#define PRIV_GFXBASE(base) ((struct GfxBase_intern *)base)

#define SDD(base)  ((struct shared_driverdata *)PRIV_GFXBASE(base)->shared_driverdata)

#define OOPBase (SDD(GfxBase)->oopbase)

/* Storage for bitmap object */
#define BM_OBJ(bitmap) ((APTR)(bitmap)->Planes[0])

/* Rastport flag that tells whether or not the driver has been inited */

#define RPF_DRIVER_INITED (1L << 15)

struct shared_driverdata
{
    Object *gfxhidd;
    struct Library *oopbase;
};


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
		
    sdd = SDD(GfxBase);


    dd = (struct gfx_driverdata *) rp->longreserved[0];
    
    HIDD_Gfx_DisposeGC(sdd->gfxhidd, dd->dd_GC);

    FreeMem (dd, sizeof(struct gfx_driverdata));
}

BOOL CorrectDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    BOOL retval = TRUE;
    struct gfx_driverdata * dd, * old;
    
    EnterFunc(bug("CorrectDriverData(rp=%p)\n", rp));

    
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
    ReturnBool("CorrectDriverData", retval);
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
	    
	    	   ReturnInt("driver_init", int, TRUE);
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
    Object *gc;

    
    /* We have to use layers to perform clipping */
    struct Layer *L = rp->Layer;

    EnterFunc(bug("driver_SetRast(rp=%p, color=%u)\n", rp, color));
    
    if (NULL != L)
    {
        /* Layered rastport, we have to clip this operation. */
    }
    else
    {
        /* Nonlayered (screen) */
	
	if (CorrectDriverData(rp, GfxBase))
	{
    	    struct TagItem tags[] = {
    	    	{ aHidd_GC_Background, color },
	    	{ TAG_DONE, 0UL }
    	    };
	
            gc = GetDriverData(rp)->dd_GC;
	    D(bug("gc=%p\n", gc));
	    
	    
            SetAttrs(gc, tags);
	    HIDD_GC_Clear(gc);
        }

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
	bm_obj = HIDD_Gfx_NewBitMap(gfxhidd, bm_tags);
	D(bug("bitmap object: %p\n", bm_obj));

	if (bm_obj)
	{
	    /* Store it in plane array */
	    BM_OBJ(nbm) = bm_obj;
	
	    ReturnPtr("driver_AllocBitMap", struct BitMap *, nbm);
	    
	}
	
	
	FreeMem(nbm, sizeof (struct BitMap));
	
    }

    ReturnPtr("driver_AllocBitMap", struct BitMap *, NULL);
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


LONG driver_WritePixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    return 0;
} /* driver_WritePixelArray8 */


