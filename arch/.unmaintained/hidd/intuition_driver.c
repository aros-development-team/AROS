/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/

#define AROS_USE_OOP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <devices/keymap.h>
#include <devices/input.h>

#include <proto/exec.h>
#include <proto/layers.h>

#include <proto/graphics.h>
#include <proto/arossupport.h>
/* #include <proto/alib.h> */
#include "intuition_intern.h"
#include "gadgets.h"

#undef GfxBase
#undef LayersBase

#include "graphics_internal.h"

#include <proto/intuition.h>

#undef DEBUG
#undef SDEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


static BOOL createsysgads(struct Window *w, struct IntuitionBase *IntuitionBase);
static VOID disposesysgads(struct Window *w, struct IntuitionBase *IntuitionBase);



enum {
    DRAGBAR = 0,
    CLOSEGAD,
    DEPTHGAD,
    SIZEGAD,
    ZOOMGAD,
    NUM_SYSGADS
};
    
	
struct HiddIntWindow
{
    struct IntWindow window;
    
    /* Some direct-pointers to the window system gadgets 
       (They are put in windows glist too)
    */
    Object * sysgads[NUM_SYSGADS];

};

#define HIW(x) ((struct HiddIntWindow *)x)
#define SYSGAD(w, idx) (HIW(w)->sysgads[idx])

static struct GfxBase *GfxBase = NULL;
static struct IntuitionBase * IntuiBase;
static struct Library *LayersBase = NULL;




int intui_init (struct IntuitionBase * IntuitionBase)
{
    


#warning FIXME: this is a hack
    IntuiBase = IntuitionBase;
    
    return TRUE;
}



int intui_open (struct IntuitionBase * IntuitionBase)
{
    
    /* Hack */
    if (!GfxBase)
    {
    	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    	if (!GfxBase)
    	    return FALSE;
    }	    
    
    if (!LayersBase)
    {
    	LayersBase = OpenLibrary("layers.library", 0);
    	if (!LayersBase)
    	    return FALSE;
    }	    


    return TRUE;
}

void intui_close (struct IntuitionBase * IntuitionBase)
{
    return;
}

void intui_expunge (struct IntuitionBase * IntuitionBase)
{
    return;
}

void intui_SetWindowTitles (struct Window * win, UBYTE * text, UBYTE * screen)
{
}

int intui_GetWindowSize (void)
{
    return sizeof (struct HiddIntWindow);
}


/* Georg Steger: changed TITLEBAR_HEIGHT */
/* #define TITLEBAR_HEIGHT 14 */
#define TITLEBAR_HEIGHT (w->BorderTop)

int intui_OpenWindow (struct Window * w,
	struct IntuitionBase * IntuitionBase,
	struct BitMap        * SuperBitMap)
{
    /* Create a layer for the window */
    LONG layerflags = 0;
    
    EnterFunc(bug("intui_OpenWindow(w=%p)\n", w));
    
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
    D(bug("screen: %p\n", w->WScreen));
    D(bug("bitmap: %p\n", w->WScreen->RastPort.BitMap));
    
    /* Just insert some default values, should be taken from
       w->WScreen->WBorxxxx */
    
    /* Set the layer's flags according to the flags of the
    ** window
    */
    
    /* refresh type */
    if (0 != (w->Flags & WFLG_SIMPLE_REFRESH))
      layerflags |= LAYERSIMPLE;
    else
      if (0!= (w->Flags & WFLG_SUPER_BITMAP))
        layerflags |= (LAYERSMART|LAYERSUPER);
      else
        layerflags |= LAYERSMART;
        
    if (0 != (w->Flags & WFLG_BACKDROP))   
      layerflags |= LAYERBACKDROP;

    D(bug("Window dims: (%d, %d, %d, %d)\n",
    	w->LeftEdge, w->TopEdge, w->Width, w->Height));
    	
    /* A GimmeZeroZero window??? */
    if (0 != (w->Flags & WFLG_GIMMEZEROZERO))
    {
      /* 
        A GimmeZeroZero window is to be created:
          - the outer window will be a simple refresh layer
          - the inner window will be a layer according to the flags
        What is the size of the inner/outer window supposed to be???
        I just make it that the outer window has the size of what is requested
      */
      

      /* First create outer window */
      struct Layer * L = CreateUpfrontHookLayer(
                             &w->WScreen->LayerInfo
                           , w->WScreen->RastPort.BitMap
                           , w->LeftEdge
                           , w->TopEdge
                           , w->LeftEdge + w->Width - 1
                           , w->TopEdge  + w->Height - 1
                           , LAYERSIMPLE | (layerflags & LAYERBACKDROP)
                           , LAYERS_NOBACKFILL
                           , SuperBitMap);
                           
      /* Could the layer be created. Nothing bad happened so far, so simply leave */
      if (NULL == L)
        ReturnBool("intui_OpenWindow", FALSE);
				      
      /* install it as the BorderRPort */
      w->BorderRPort = L->rp;

      /* This layer belongs to a window */
      L->Window = (APTR)w;
     
      w->GZZWidth = w->Width  - w->BorderLeft - w->BorderRight;
      w->GZZHeight= w->Height - w->BorderTop  - w->BorderBottom;

      /* Now comes the inner window */
      w->WLayer = CreateUpfrontHookLayer( 
                   &w->WScreen->LayerInfo
	  	  , w->WScreen->RastPort.BitMap
		  , w->LeftEdge + w->BorderLeft  
		  , w->TopEdge  + w->BorderTop
		  , w->LeftEdge + w->BorderLeft + w->GZZWidth - 1
		  , w->TopEdge  + w->BorderTop + w->GZZHeight - 1
		  , layerflags
		  , LAYERS_BACKFILL
		  , SuperBitMap);

      /* could this layer be created? If not then delete the outer window and exit */
      if (NULL == w->WLayer)
      {
        DeleteLayer(0, L);
        ReturnBool("intui_OpenWindow", FALSE);
      }	
      	  
      /* That should do it, I guess... */
    }
    else
    {
      w->WLayer = CreateUpfrontHookLayer( 
                   &w->WScreen->LayerInfo
	  	  , w->WScreen->RastPort.BitMap
		  , w->LeftEdge
		  , w->TopEdge
		  , w->LeftEdge + w->Width - 1
		  , w->TopEdge  + w->Height - 1
		  , layerflags
		  , LAYERS_BACKFILL
		  , SuperBitMap);

      /* Install the BorderRPort here! see GZZ window above  */
      if (NULL != w->WLayer)
      {
        /* 
           I am installing a totally new RastPort here so window and frame can
           have different fonts etc. 
        */
        w->BorderRPort = AllocMem(sizeof(struct RastPort), MEMF_CLEAR);
        if (w->BorderRPort)
        {
          InitRastPort(w->BorderRPort);
          w->BorderRPort->Layer  = w->WLayer;
          w->BorderRPort->BitMap = w->WLayer->rp->BitMap;
        }
        else
        {
          /* no memory for RastPort! Simply close the window */
          intui_CloseWindow(w, IntuitionBase);
    	  ReturnBool("intui_OpenWindow", FALSE);
        }
      }		  
    }

    D(bug("Layer created: %p\n", w->WLayer));
    D(bug("Window created: %p\n", w));
    
    /* common code for GZZ and regular windows */
    
    if (w->WLayer)
    {
        /* Layer gets pointer to the window */
        w->WLayer->Window = (APTR)w;
	/* Window needs a rastport */
	w->RPort = w->WLayer->rp;
	
	/* installation of the correct BorderRPort already happened above !! */
	 
	if (createsysgads(w, IntuitionBase))
	{

    	    ReturnBool("intui_OpenWindow", TRUE);

	}
	intui_CloseWindow(w, IntuitionBase);
	
    } /* if (layer created) */
    
    ReturnBool("intui_OpenWindow", FALSE);
}

void intui_CloseWindow (struct Window * w,
	                struct IntuitionBase * IntuitionBase)
{
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
    disposesysgads(w, IntuitionBase);
    if (0 == (w->Flags & WFLG_GIMMEZEROZERO))
    {
      /* not a GZZ window */
      if (w->WLayer)
      	DeleteLayer(0, w->WLayer);
      DeinitRastPort(w->BorderRPort);
      FreeMem(w->BorderRPort, sizeof(struct RastPort));
    }
    else
    {
      /* a GZZ window */
      /* delete inner window */
      if (NULL != w->WLayer)
        DeleteLayer(0, w->WLayer);
      
      /* delete outer window */
      if (NULL != w->BorderRPort && 
          NULL != w->BorderRPort->Layer)
        DeleteLayer(0, w->BorderRPort->Layer);      
    }
}

void intui_RefreshWindowFrame(struct Window *w)
{
    /* Draw a frame around the window */
    struct RastPort *rp = w->BorderRPort;
    struct DrawInfo *dri;
    struct Gadget *gad;
    struct Region *old_clipregion;
    WORD  old_scroll_x, old_scroll_y;
    ULONG ilock;
    UWORD i;
    
    EnterFunc(bug("intui_RefreshWindowFrame(w=%p)\n", w));
    
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
    if (!(w->Flags & WFLG_BORDERLESS))
    {
    	
	dri = GetScreenDrawInfo(w->WScreen);
	if (dri)
	{
	    LockLayerRom(rp->Layer);

	    old_scroll_x = rp->Layer->Scroll_X;
	    old_scroll_x = rp->Layer->Scroll_Y;
	    
	    rp->Layer->Scroll_X = 0;
	    rp->Layer->Scroll_Y = 0;
	    
	    old_clipregion = InstallClipRegion(rp->Layer, NULL);
	    
	    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
	    if (w->BorderLeft > 0) RectFill(rp, 0, 0, 0, w->Height - 1);
	    if (w->BorderTop > 0)  RectFill(rp, 0, 0, w->Width - 1, 0);
	    if (w->BorderRight > 1) RectFill(rp, w->Width - w->BorderRight,w->BorderTop,
	    					 w->Width - w->BorderRight,w->Height - w->BorderBottom);
	    if (w->BorderBottom > 1) RectFill(rp,w->BorderLeft,w->Height - w->BorderBottom,
	    					 w->Width - w->BorderRight,w->Height - w->BorderBottom);
	    
	    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
	    if (w->BorderRight > 0) RectFill(rp, w->Width - 1, 1, w->Width - 1, w->Height - 1);
	    if (w->BorderBottom > 0) RectFill(rp, 1, w->Height - 1, w->Width - 1, w->Height - 1);
	    if (w->BorderLeft > 1) RectFill(rp, w->BorderLeft - 1, w->BorderTop - 1,
	    					w->BorderLeft - 1, w->Height - w->BorderBottom);
	    if (w->BorderTop > 1) RectFill(rp, w->BorderLeft - 1, w->BorderTop - 1,
	    				       w->Width - w->BorderRight, w->BorderTop - 1);
	    
	   
	    SetAPen(rp, dri->dri_Pens[(w->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);
	    if (w->BorderLeft > 2) RectFill(rp, 1, 1, w->BorderLeft - 2,w->Height - 2);
	    if (w->BorderTop > 2)  RectFill(rp, 1, 1, w->Width - 2, w->BorderTop - 2);
	    if (w->BorderRight > 2) RectFill(rp, w->Width - w->BorderRight + 1, 1,
	    					 w->Width - 2, w->Height - 2);
	    if (w->BorderBottom > 2) RectFill(rp, 1, w->Height - w->BorderBottom + 1,
	    					  w->Width - 2, w->Height - 2);
						  
	    InstallClipRegion(rp->Layer,old_clipregion);
	    
	    rp->Layer->Scroll_X = old_scroll_x;
	    rp->Layer->Scroll_Y = old_scroll_y;
	    
	    UnlockLayerRom(rp->Layer);
	    
    	    /* Refresh all the gadgets with GACT_???BORDER activation set */

            /* The layer must not be locked when calling RefreshGList,
	       otherwise a deadlock can happen:
	       
	       task a: ObtainGirPort: ObtainSem(GadgetLock)
	       
	       ** task switch **
	       
	       task b: LockLayer
	               refreshglist -> ObtainGIRPort : ObtainSem(GadgetLock)
		                                       must wait because locked by task a
						       
	       ** task switch **
	       
	       task a: ObtainGirPort: LockLayer
	                              must wait because layer locked by task b
				      
	       --------------------------------------------------------------------
	       = Deadlock: task a tries to lock layer which is locked by task b
	                   task b will never unlock the layer because it tries to
			   ObtainSem GadgetLock which is locked by task a.
	    
	    */
	    
	    ilock = LockIBase(0);
	    
	    gad = w->FirstGadget;
	    while(gad)
	    {
	    	if (gad->Activation & (GACT_TOPBORDER |
				       GACT_LEFTBORDER |
				       GACT_RIGHTBORDER |
				       GACT_BOTTOMBORDER))
		{
		    RefreshGList(gad, w, NULL, 1);
		}
		
	    	gad = gad->NextGadget;
	    }
	    
	    UnlockIBase(ilock);
#if 0	    
	    for (i = 0; i < NUM_SYSGADS; i ++)
	    {
        	if (SYSGAD(w, i))
		    RefreshGList((struct Gadget *)SYSGAD(w, i), w, NULL, 1 );
	    }
#endif

	    FreeScreenDrawInfo(w->WScreen, dri);
	    
	} /* if (dri) */
	
    } /* if (!(win->Flags & WFLG_BORDERLESS)) */
    
    ReturnVoid("intui_RefreshWindowFrame");
}

BOOL intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
    WORD width, WORD height)
{
    BOOL success;

#warning this code should be moved inside intuition (it does not contain any hardware specific code)
    
    if (0 != (window->Flags & WFLG_GIMMEZEROZERO))
    {
      success = MoveSizeLayer(window->BorderRPort->Layer,
    		              x - window->LeftEdge,
    		              y - window->TopEdge,
    		              width - window->Width,
                              height - window->Height );
      if (FALSE == success)
        return FALSE;
     
#warning Why is this necessary ? There is another RWF() below !
      RefreshWindowFrame(window);

      window->GZZWidth  += (width - window->Width);
      window->GZZHeight += (height - window->Height);                
    }
    success = MoveSizeLayer(window->WLayer,
    	                    x - window->LeftEdge,
    		            y - window->TopEdge,
                            width - window->Width,
                            height - window->Height );
    if (FALSE == success)
    {
#warning FIXME: If this fails, then the MoveSizeLayer() above must be undone.
        return FALSE;
    }

    window->LeftEdge= x;
    window->TopEdge = y;
    window->Width   = width;
    window->Height  = height;

    RefreshWindowFrame(window);

    return TRUE;
}


void intui_WindowLimits (struct Window * win,
    WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight)
{

}

void intui_ActivateWindow (struct Window * win)
{

}

struct Window *intui_FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event, struct IntuitionBase *IntuitionBase)
{
    /* The caller has checked that the input event is a IECLASS_RAWMOUSE, SELECTDOWN event */
    struct Screen *scr;
    struct Layer *l;
    struct Window *new_w = NULL;
    ULONG lock;
    
    *swallow_event = FALSE;

#warning this code should be moved inside intuition (it does not contain any hardware specific code)
#warning Fixme: Find out what screen the click was in.

    lock = LockIBase(0UL);
    scr = IntuitionBase->ActiveScreen;
    UnlockIBase(lock);
    
    if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTDOWN)
    {
	/* What layer ? */
	D(bug("Click at (%d,%d)\n",ie->ie_X,ie->ie_Y));
	LockLayerInfo(&scr->LayerInfo);
	
	l = WhichLayer(&scr->LayerInfo, ie->ie_X, ie->ie_Y);
	
	UnlockLayerInfo(&scr->LayerInfo);
	
	if (NULL == l)
	{
	    D(bug("iih: Click not inside layer\n"));
	}
	else
	{
	    new_w = (struct Window *)l->Window;
	    if (!new_w)
	    {
		D(bug("iih: Selected layer is not a window\n"));
	    }
    
    D(bug("Found layer %p\n", l));
    
        }
    }
    return new_w;
}

LONG intui_RawKeyConvert (struct InputEvent * ie, STRPTR buf,
	LONG size, struct KeyMap * km)
{

    return 0;
} /* intui_RawKeyConvert */

void intui_BeginRefresh (struct Window * win,
	struct IntuitionBase * IntuitionBase)
{
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
  /* lock all necessary layers */
  LockLayerRom(win->WLayer);
  /* Find out whether it's a GimmeZeroZero window with an extra layer to lock */
  if (0 != (win->Flags & WFLG_GIMMEZEROZERO))
    LockLayerRom(win->BorderRPort->Layer);

  /* I don't think I ever have to update the BorderRPort's layer */
  if (FALSE == BeginUpdate(win->WLayer))
  {
    EndUpdate(win->WLayer, FALSE);
    return;
  }
  
  /* let the user know that we're currently doing a refresh */
  win->Flags |= WFLG_WINDOWREFRESH;
  
} /* intui_BeginRefresh */

void intui_EndRefresh (struct Window * win, BOOL free,
	struct IntuitionBase * IntuitionBase)
{
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
  EndUpdate(win->WLayer, free);
  
  /* reset all bits indicating a necessary or ongoing refresh */
  win->Flags &= ~WFLG_WINDOWREFRESH;
  
  /* I reset this one only if Complete is TRUE!?! */
  if (TRUE == free)
    win->WLayer->Flags &= ~LAYERREFRESH;

  /* Unlock the layers. */
  if (0 != (win->Flags & WFLG_GIMMEZEROZERO))
    UnlockLayerRom(win->BorderRPort->Layer);
  
  UnlockLayerRom(win->WLayer);
 
} /* intui_EndRefresh */



static BOOL createsysgads(struct Window *w, struct IntuitionBase *IntuitionBase)
{

    struct DrawInfo *dri;
    BOOL is_gzz;
    
    EnterFunc(bug("createsysgads(w=%p)\n", w));

#warning this code should be moved inside intuition (it does not contain any hardware specific code)

    is_gzz = (w->Flags & WFLG_GIMMEZEROZERO) ? TRUE : FALSE;

    dri = GetScreenDrawInfo(w->WScreen);
    if (dri)
    {
	LONG db_left, db_width, relright; /* dragbar sizes */
	BOOL sysgads_ok = TRUE;
	
	    
	db_left = 0;
	db_width = 0; /* Georg Steger: was w->Width; */
	
	
	
	
	/* Now find out what gadgets the window wants */
	if (    w->Flags & WFLG_CLOSEGADGET 
	     || w->Flags & WFLG_DEPTHGADGET
	     || w->Flags & WFLG_HASZOOM
	     || w->Flags & WFLG_DRAGBAR /* To assure w->BorderTop being set correctly */
             || w->Flags & WFLG_SIZEGADGET
	)
	{
	/* If any of titlebar gadgets are present, me might just as well
	insert a dragbar too */
	       
	    w->Flags |= WFLG_DRAGBAR;

            /* Georg Steger: bordertop is set by rom/intuition/openwindow.c
	                     to scr->WBorTop + FontHeight + 1 */
			     
	    /* w->BorderTop = TITLEBAR_HEIGHT; */
	}
	
	/* Relright of rightmost button */
	relright = - (TITLEBAR_HEIGHT - 1);



	if (w->Flags & WFLG_SIZEGADGET)
	{
	    /* this code must not change the 'relright' variable */
#warning The sizegadget size should depend on sysiclass depth image which itself depends on drawinfo

	    struct TagItem size_tags[] = {
	            {GA_RelRight,	-16 + 1		},
		    {GA_RelBottom,	-16 + 1 	},
		    {GA_Width,		16		},
		    {GA_Height,		16		},
		    {GA_DrawInfo,	(IPTR)dri 	},	/* required	*/
		    {GA_SysGadget,	TRUE		},
		    {GA_SysGType,	GTYP_SIZING 	},
		    {GA_BottomBorder,	TRUE		},
		    {GA_RightBorder,	TRUE		},
		    {GA_GZZGadget,	is_gzz		},
		    {TAG_DONE,		0UL }
	    };
	    SYSGAD(w, SIZEGAD) = NewObjectA(
			GetPrivIBase(IntuitionBase)->sizebuttonclass
			, NULL
			, size_tags );

	    if (!SYSGAD(w, SIZEGAD))
		sysgads_ok = FALSE;
	    
	    
	}  
	
	if (w->Flags & WFLG_DEPTHGADGET)
	{
	    struct TagItem depth_tags[] = {
	            {GA_RelRight,	relright	},
		    {GA_Top,		0  		},
		    {GA_Width,		TITLEBAR_HEIGHT	},
		    {GA_Height,		TITLEBAR_HEIGHT	},
		    {GA_DrawInfo,	(IPTR)dri 	},	/* required	*/
		    {GA_SysGadget,	TRUE		},
		    {GA_SysGType,	GTYP_WDEPTH 	},
		    {GA_TopBorder,	TRUE		},
		    {GA_GZZGadget,	is_gzz		},
		    {TAG_DONE,		0UL }
	    };
		
	    relright -= TITLEBAR_HEIGHT;
		
	    db_width -= TITLEBAR_HEIGHT;
	    
	    SYSGAD(w, DEPTHGAD) = NewObjectA(
			GetPrivIBase(IntuitionBase)->tbbclass
			, NULL
			, depth_tags );

	    if (!SYSGAD(w, DEPTHGAD))
		sysgads_ok = FALSE;
	    
	    
	}  

	if (w->Flags & WFLG_HASZOOM)
	{
	    struct TagItem zoom_tags[] = {
	            {GA_RelRight,	relright	},
		    {GA_Top,		0  		},
		    {GA_Width,		TITLEBAR_HEIGHT	},
		    {GA_Height,		TITLEBAR_HEIGHT	},
		    {GA_DrawInfo,	(IPTR)dri 	},	/* required	*/
		    {GA_SysGadget,	TRUE		},
		    {GA_SysGType,	GTYP_WZOOM 	},
		    {GA_TopBorder,	TRUE		},
		    {GA_GZZGadget,	is_gzz		},
		    {TAG_DONE,		0UL }
	    };
		
	    relright -= TITLEBAR_HEIGHT;
	    db_width -= TITLEBAR_HEIGHT;
	    
	    SYSGAD(w, ZOOMGAD) = NewObjectA(
			GetPrivIBase(IntuitionBase)->tbbclass
			, NULL
			, zoom_tags );

	    if (!SYSGAD(w, ZOOMGAD))
		sysgads_ok = FALSE;
	}  

	if (w->Flags & WFLG_CLOSEGADGET)
	{
	    struct TagItem close_tags[] = {
	            {GA_Left,		0		},
		    {GA_Top,		0  		},
		    {GA_Width,		TITLEBAR_HEIGHT	},
		    {GA_Height,		TITLEBAR_HEIGHT	},
		    {GA_DrawInfo,	(IPTR)dri 	},	/* required	*/
		    {GA_SysGadget,	TRUE		},
		    {GA_SysGType,	GTYP_CLOSE 	},
		    {GA_TopBorder,	TRUE		},
		    {GA_GZZGadget,	is_gzz		},
		    {TAG_DONE,		0UL }
	    };
		
	    db_left  += TITLEBAR_HEIGHT;
	    db_width -= TITLEBAR_HEIGHT;
	    
	    SYSGAD(w, CLOSEGAD) = NewObjectA(
			GetPrivIBase(IntuitionBase)->tbbclass
			, NULL
			, close_tags );

	    if (!SYSGAD(w, CLOSEGAD))
		sysgads_ok = FALSE;
	}  

	/* Now try to create the various gadgets */
	if (w->Flags & WFLG_DRAGBAR)
	{

	    struct TagItem dragbar_tags[] = {
			{GA_Left,	db_left		},
			{GA_Top,	0		},
			{GA_RelWidth,	db_width 	},
			{GA_Height,	TITLEBAR_HEIGHT },
			{GA_SysGType,	GTYP_WDRAGGING	},
			{GA_TopBorder,	TRUE		},
		    	{GA_GZZGadget, 	is_gzz		},
			{TAG_DONE,	0UL		}
	    };
	    SYSGAD(w, DRAGBAR) = NewObjectA(
			GetPrivIBase(IntuitionBase)->dragbarclass
			, NULL
			, dragbar_tags );
				
	    if (!SYSGAD(w, DRAGBAR))
		sysgads_ok = FALSE;
				
	}
	    

	D(bug("Dragbar:  %p\n", SYSGAD(w, DRAGBAR ) ));
	D(bug("Depthgad: %p\n", SYSGAD(w, DEPTHGAD) ));
	D(bug("Zoomgad:  %p\n", SYSGAD(w, ZOOMGAD ) ));
	D(bug("Closegad: %p\n", SYSGAD(w, CLOSEGAD) ));
	D(bug("Sizegad:  %p\n", SYSGAD(w, SIZEGAD ) ));
	    
	/* Don't need drawinfo anymore */
	FreeScreenDrawInfo(w->WScreen, dri);
	
	if (sysgads_ok)
	{
	    UWORD i;
	
	
	    D(bug("Adding gadgets\n"));
	    for (i = 0; i < NUM_SYSGADS; i ++)
	    {
		if (SYSGAD(w, i))
		    AddGList(w, (struct Gadget *)SYSGAD(w, i), 0, 1, NULL);
	    }
	    
	    D(bug("Refreshing frame\n"));


	    ReturnBool("createsysgads", TRUE);
	    
	} /* if (sysgads created) */
	
	disposesysgads(w, IntuitionBase);
	
    } /* if (got DrawInfo) */
    ReturnBool("createsysgads", FALSE);

}


static VOID disposesysgads(struct Window *w, struct IntuitionBase *IntuitionBase)
{
    /* Free system gadges */
    UWORD i;
    
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
    for (i = 0; i < NUM_SYSGADS; i ++)
    {
        if (SYSGAD(w, i))
	{
	    RemoveGadget( w, (struct Gadget *)SYSGAD(w, i));
	    DisposeObject( SYSGAD(w, i) );
	}
    }
}

void intui_ScrollWindowRaster(struct Window * win,
                              WORD dx,
                              WORD dy,
                              WORD xmin,
                              WORD ymin,
                              WORD xmax,
                              WORD ymax,
                              struct IntuitionBase * IntuitionBase)
{
#warning this code should be moved inside intuition (it does not contain any hardware specific code)
  ScrollRasterBF(win->RPort,
                 dx,
                 dy,
                 xmin,
                 ymin,
                 xmax,
                 ymax);
  /* Has there been damage to the layer? */
  if (0 != (win->RPort->Layer->Flags & LAYERREFRESH))
  {
    /* 
       Send a refresh message to the window if it doesn't already
       have one.
    */
    windowneedsrefresh(win, IntuitionBase);
  } 
}

void windowneedsrefresh(struct Window * w, 
                        struct IntuitionBase * IntuitionBase )
{
  /* Supposed to send a message to this window, saying that it needs a
     refresh. I will check whether there is no such a message queued in
     its messageport, though. It only needs one such message! 
  */
  struct IntuiMessage * IM;
  BOOL found = FALSE;

#warning this code should be moved inside intuition (it does not contain any hardware specific code)
  if (NULL == w->UserPort)
    return;
  
  
  /* Refresh the window's gadgetry ... 
     ... stegerg: and in the actual implementation
     call RefershWindowFrame first, as the border gadgets don´t
     cover the whole border area. bug: the border gadgets will
     be refreshed twice :( */

  
  if (FALSE != BeginUpdate(w->WLayer))
  {
  
    if (!(w->Flags & WFLG_GIMMEZEROZERO))
    {
      kprintf("REFRESHING NON GZZ WINDOW FRAME\n");
      RefreshWindowFrame(w);
    }
  
    kprintf("REFRESHING WINDOW GADGETS\n");

    RefreshGadgets(w->FirstGadget, w, NULL);
  }

  EndUpdate(w->WLayer, FALSE);
  
  /* Can use Forbid() for this */
  
  Forbid();
  
  IM = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

  /* reset the flag in the layer */
  w->WLayer->Flags &= ~LAYERREFRESH;

  while ((NULL != IM) && !found)
  {
    /* Does the window already have such a message? */
    if (IDCMP_REFRESHWINDOW == IM->Class)
    {
kprintf("Window %s already has a refresh message pending!!\n",w->Title);
      found = TRUE;
    }
    IM = (struct IntuiMessage *)IM->ExecMessage.mn_Node.ln_Succ;
  }
  
  Permit();

kprintf("Sending a refresh message to window %s!!\n",w->Title);
  if (!found)
  {
    IM = alloc_intuimessage(IntuitionBase);
    if (NULL != IM)
    {
      IM->Class = IDCMP_REFRESHWINDOW;
      send_intuimessage(IM, w, IntuitionBase);
    }
  }
}
