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
#include <graphics/layers.h>
#include <graphics/clip.h>

#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"
#include "graphics_internal.h"


#define SDEBUG 0
#define DEBUG 0
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
    struct gfx_driverdata *dd;

    EnterFunc(bug("driver_SetAPen(rp=%p, pen=%d)\n", rp, pen));
    if (!CorrectDriverData (rp, GfxBase))
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
    ReturnVoid("driver_SetAPen");
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

    if(!CorrectDriverData (rp, GfxBase))
	return ((ULONG)-1L);

    return ((ULONG)-1L);
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
  
  EnterFunc(bug("driver_WritePixel(rp=%p, x=%d, y=%d)\n", rp, x, y));
  if(!CorrectDriverData (rp, GfxBase))
	ReturnInt("driver_WritePixel", LONG,  ((ULONG)-1L));
	
  dd = GetDriverData(rp);

  /*  nlorentz: Only rasports without layers (screen rastports) have bitmaps (for now) 
      Width and Height may now contain bogus values they won't be used for anything critical.
  */
  if (NULL == L)
  {
     Width = GetBitMapAttr(bm, BMA_WIDTH);  
     Height = GetBitMapAttr(bm, BMA_HEIGHT);
  }
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
	ReturnInt("driver_WritePixel", LONG,  ((ULONG)-1L));
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
    WORD YRel = L->bounds.MinY;
    WORD XRel = L->bounds.MinX;
    
    D(bug("Layer coords: maxx=%d, maxy=%d, minx=%d, miny=%d\n",
    	L->bounds.MaxX, L->bounds.MaxY, XRel, YRel)); 


    /* Is this pixel inside the layer ?? */
    if (x > (L->bounds.MaxX - XRel + 1) ||
        y > (L->bounds.MaxY - YRel + 1)   )
    {
      /* ah, no it is not. So we exit */
	ReturnInt("driver_WritePixel(not inside layer)", LONG,  ((ULONG)-1L));
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
          i = (y + YRel) * Width + 
             ((x + XRel) >> 3);
          Mask = 1 << (7-((x + XRel) & 0x07));

          /* and let the driver set the pixel to the X-Window also,
             but this Pixel has a relative position!! */
          if (bm->Flags & BMF_AROS_DISPLAYED)
            HIDD_GC_WritePixel (dd->dd_GC, x+XRel, y+YRel);
/*          if (bm->Flags & BMF_AROS_DISPLAYED)
            driver_WritePixel (rp, x+XRel, y+YRel, GfxBase);
*/	    
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
           
          Offset = CR->bounds.MinX & 0x0f;
          
          i = (y - (CR->bounds.MinY - YRel)) * Width + 
             ((x - (CR->bounds.MinX - XRel) + Offset) >> 3);   
                /* Offset: optimization for blitting!! */
          Mask = (1 << ( 7 - ((Offset + x - (CR->bounds.MinX - XRel) ) & 0x07)));
          /* no pixel into the X window */
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
      HIDD_GC_WritePixel(dd->dd_GC, x, y);

/*      driver_WritePixel (rp, x, y, GfxBase); */

  }

  /* nlorentz: For now don't mind writing into bitmap planes,
     as HIDD bitmap object is stored in bm->Planes[0];
  */
  
  ReturnInt("driver_WritePixel(at bottom)", LONG,  0);

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

  /* if there was a layer I have to unlock it now */
/*!!!
  if (NULL != L) 
    UnlockLayer(L);
*/

  ReturnInt("driver_WritePixel", LONG,  0);

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
	return NULL;

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
    
    EnterFunc(bug("driver_BltBitMap()\n"));

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


LONG driver_WritePixelArray8 (struct RastPort * rp, ULONG xstart,
	    ULONG ystart, ULONG xstop, ULONG ystop, UBYTE * array,
	    struct RastPort * temprp, struct GfxBase * GfxBase)
{
    CorrectDriverData (rp, GfxBase);
    return 0;
} /* driver_WritePixelArray8 */


