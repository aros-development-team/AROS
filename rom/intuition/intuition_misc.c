/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuitions internal structure
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/input.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/preferences.h>
#include <graphics/layers.h>

#include "intuition_preferences.h"
#include "intuition_intern.h"
#include "boopsigadgets.h"

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

/**********************************************************************************/

void LoadDefaultPreferences(struct IntuitionBase * IntuitionBase)
{
    BYTE read_preferences = FALSE;
    /*
    ** Load the intuition preferences from a file on the disk
    ** Allocate storage for the preferences, even if it's just a copy
    ** of the default preferences.
    */
    GetPrivIBase(IntuitionBase)->DefaultPreferences = 
            AllocMem(sizeof(struct Preferences),
                     MEMF_CLEAR);

                     
#warning FIXME:Try to load preferences from a file!
    

/*******************************************************************
    DOSBase = OpenLibrary("dos.library",0);
    if (NULL != DOSBase)
    {
      if (NULL != (pref_file = Open("envarc:",MODE_OLDFILE)))
      {
        *
        **  Read it and check whether the file was valid.
        *

        if (sizeof(struct Preferences) ==
            Read(pref_file, 
                 GetPrivIBase(IntuitionBase)->DefaultPreferences,
                 sizeof(struct Preferences)))
          read_preferences = TRUE;

        Close(pref_file);
      }
      CloseLibrary(DOSBase)
    }
****************************************************************/

    if (FALSE == read_preferences)
    {
      /* 
      ** no (valid) preferences file is available.
      */
      memcpy(GetPrivIBase(IntuitionBase)->DefaultPreferences,
             &IntuitionDefaultPreferences,
             sizeof(struct Preferences));
    }


    /*
    ** Activate the preferences...
    */

    GetPrivIBase(IntuitionBase)->ActivePreferences = 
            AllocMem(sizeof(struct Preferences),
                     MEMF_CLEAR);

/*    
    SetPrefs(GetPrivIBase(IntuitionBase)->DefaultPreferences,
             sizeof(struct Preferences),
             TRUE);
*/
}

/**********************************************************************************/

void CheckRectFill(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
                   struct IntuitionBase * IntuitionBase)
{

    if ((x2 >= x1) && (y2 >= y1))
    {
    	RectFill(rp, x1, y1, x2, y2);
    }
}

/**********************************************************************************/

#define TITLEBAR_HEIGHT (w->BorderTop)

BOOL CreateWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase)
{

    struct DrawInfo *dri;
    BOOL is_gzz;
    
    EnterFunc(bug("CreateWinSysGadgets(w=%p)\n", w));

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
	    WORD width  = ((struct IntWindow *)w)->sizeimage_width;
	    WORD height = ((struct IntWindow *)w)->sizeimage_height;
	    
	    struct TagItem size_tags[] =
	    {
	    	{GA_Image   	, 0 	    	},
	        {GA_RelRight	, -width + 1	},
		{GA_RelBottom	, -height + 1 	},
		{GA_Width	, width		},
		{GA_Height	, height	},
		{GA_SysGadget	, TRUE		},
		{GA_SysGType	, GTYP_SIZING 	},
		{GA_BottomBorder, TRUE		},
		{GA_RightBorder	, TRUE		},
		{GA_GZZGadget	, is_gzz	},
		{TAG_DONE			}
	    };
	    struct TagItem image_tags[] =
	    {
	    	{IA_Width   	, width     },
		{IA_Height  	, height    },
		{SYSIA_Which	, SIZEIMAGE },
		{SYSIA_DrawInfo , (IPTR)dri },
		{TAG_DONE   	    	    }		
	    };
	    Object *im;
	    
	    im = NewObjectA(NULL, SYSICLASS, image_tags);
	    if (!im)
	    {
	    	sysgads_ok = FALSE;
	    }
	    else
	    {
	    	size_tags[0].ti_Data = (IPTR)im;
		
		SYSGAD(w, SIZEGAD) = NewObjectA(NULL, BUTTONGCLASS, size_tags);

		if (!SYSGAD(w, SIZEGAD))
		{
		    DisposeObject(im);
		    sysgads_ok = FALSE;
		}
	    }
	    
	}  
	
	if (w->Flags & WFLG_DEPTHGADGET)
	{
	    struct TagItem depth_tags[] =
	    {
	    	{GA_Image   	, 0 	    	    	},
	        {GA_RelRight	, relright		},
		{GA_Top		, 0  			},
		{GA_Width	, TITLEBAR_HEIGHT	},
		{GA_Height	, TITLEBAR_HEIGHT	},
		{GA_SysGadget	, TRUE			},
		{GA_SysGType	, GTYP_WDEPTH 		},
		{GA_TopBorder	, TRUE			},
		{GA_GZZGadget	, is_gzz		},
		{TAG_DONE	 			}
	    };
	    struct TagItem image_tags[] =
	    {
	    	{IA_Left    	, -1	    	    	},
	    	{IA_Width   	, TITLEBAR_HEIGHT + 1 	},
		{IA_Height  	, TITLEBAR_HEIGHT   	},
		{SYSIA_Which	, DEPTHIMAGE 	    	},
		{SYSIA_DrawInfo , (IPTR)dri 	    	},
		{TAG_DONE   	    	    	    	}		
	    };
	    Object *im;
		
	    relright -= TITLEBAR_HEIGHT;
	    db_width -= TITLEBAR_HEIGHT;
		
	    im = NewObjectA(NULL, SYSICLASS, image_tags);
	    if (!im)
	    {
	    	sysgads_ok = FALSE;
	    }
	    else
	    {
    	    	depth_tags[0].ti_Data = (IPTR)im;
	    
		SYSGAD(w, DEPTHGAD) = NewObjectA(NULL, BUTTONGCLASS, depth_tags);

		if (!SYSGAD(w, DEPTHGAD))
		{
		    DisposeObject(im);
		    sysgads_ok = FALSE;
		}
	    }
	}  

	if (w->Flags & WFLG_HASZOOM)
	{
	    struct TagItem zoom_tags[] = 
	    {
	    	{GA_Image   	, 0 	    	    	},
	        {GA_RelRight	, relright		},
		{GA_Top		, 0  			},
		{GA_Width	, TITLEBAR_HEIGHT	},
		{GA_Height	, TITLEBAR_HEIGHT	},
		{GA_SysGadget	, TRUE			},
		{GA_SysGType	, GTYP_WZOOM 		},
		{GA_TopBorder	, TRUE			},
		{GA_GZZGadget	, is_gzz		},
		{TAG_DONE	 			}
	    };
	    struct TagItem image_tags[] =
	    {
	    	{IA_Left    	, -1	    	    	},
	    	{IA_Width   	, TITLEBAR_HEIGHT + 1   },
		{IA_Height  	, TITLEBAR_HEIGHT   	},
		{SYSIA_Which	, ZOOMIMAGE 	    	},
		{SYSIA_DrawInfo , (IPTR)dri 	    	},
		{TAG_DONE   	    	    	    	}		
	    };
	    Object *im;
		
	    relright -= TITLEBAR_HEIGHT;
	    db_width -= TITLEBAR_HEIGHT;
	    
	    im = NewObjectA(NULL, SYSICLASS, image_tags);
	    if (!im)
	    {
	    	sysgads_ok = FALSE;
	    }
	    else
	    {
    	    	zoom_tags[0].ti_Data = (IPTR)im;

		SYSGAD(w, ZOOMGAD) = NewObjectA(NULL, BUTTONGCLASS, zoom_tags);

		if (!SYSGAD(w, ZOOMGAD))
		{
		    DisposeObject(im);
		    sysgads_ok = FALSE;
		}
	    }
	}  

	if (w->Flags & WFLG_CLOSEGADGET)
	{
	    struct TagItem close_tags[] =
	    {
	    	{GA_Image   	, 0 	    	    	},
	        {GA_Left	, 0			},
		{GA_Top		, 0  			},
		{GA_Width	, TITLEBAR_HEIGHT	},
		{GA_Height	, TITLEBAR_HEIGHT	},
		{GA_SysGadget	, TRUE			},
		{GA_SysGType	, GTYP_CLOSE 		},
		{GA_TopBorder	, TRUE			},
		{GA_GZZGadget	, is_gzz		},
		{TAG_DONE				}
	    };
	    struct TagItem image_tags[] =
	    {
	    	{IA_Width   	, TITLEBAR_HEIGHT + 1	},
		{IA_Height  	, TITLEBAR_HEIGHT   	},
		{SYSIA_Which	, CLOSEIMAGE 	    	},
		{SYSIA_DrawInfo , (IPTR)dri 	    	},
		{TAG_DONE   	    	    	    	}		
	    };
	    Object *im;
		
	    db_left  += TITLEBAR_HEIGHT;
	    db_width -= TITLEBAR_HEIGHT;

	    im = NewObjectA(NULL, SYSICLASS, image_tags);
	    if (!im)
	    {
	    	sysgads_ok = FALSE;
	    }
	    else
	    {
 	    	close_tags[0].ti_Data = (IPTR)im;
		
	    	SYSGAD(w, CLOSEGAD) = NewObjectA(NULL, BUTTONGCLASS, close_tags);

		if (!SYSGAD(w, CLOSEGAD))
		{
		    DisposeObject(im);
		    sysgads_ok = FALSE;
		}
	    }
	}  

	/* Now try to create the various gadgets */
	if (w->Flags & WFLG_DRAGBAR)
	{

	    struct TagItem dragbar_tags[] =
	    {
		{GA_Left	, db_left		},
		{GA_Top		, 0			},
		{GA_RelWidth	, db_width 		},
		{GA_Height	, TITLEBAR_HEIGHT 	},
		{GA_SysGType	, GTYP_WDRAGGING	},
		{GA_TopBorder	, TRUE			},
		{GA_GZZGadget	, is_gzz		},
		{TAG_DONE				}
	    };
	    SYSGAD(w, DRAGBAR) = NewObjectA(NULL, BUTTONGCLASS, dragbar_tags);
				
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
	    
	    ReturnBool("CreateWinSysGadgets", TRUE);
	    
	} /* if (sysgads created) */
	
	KillWinSysGadgets(w, IntuitionBase);
	
    } /* if (got DrawInfo) */
    ReturnBool("CreateWinSysGadgets", FALSE);

}

/**********************************************************************************/

VOID KillWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase)
{
    /* Free system gadgets */
    UWORD i;
    
    for (i = 0; i < NUM_SYSGADS; i ++)
    {
        if (SYSGAD(w, i))
	{
	    RemoveGadget( w, (struct Gadget *)SYSGAD(w, i));
	    if (((struct Gadget *)SYSGAD(w, i))->GadgetRender)
	    {
	    	DisposeObject((Object *)((struct Gadget *)SYSGAD(w, i))->GadgetRender);
	    }
	    DisposeObject( SYSGAD(w, i) );
	}
    }
}

/**********************************************************************************/

void CreateScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    if (!scr->BarLayer)
    {
    	scr->BarLayer = CreateUpfrontHookLayer(&scr->LayerInfo,
					       scr->RastPort.BitMap,
					       0,
					       0,
					       scr->Width - 1,
					       scr->BarHeight, /* 1 pixel heigher than scr->BarHeight */
					       LAYERSIMPLE | LAYERBACKDROP,
					       LAYERS_NOBACKFILL,
					       NULL);
					       
					       
	if (scr->BarLayer)
	{
	    SetFont(scr->BarLayer->rp, ((struct IntScreen *)scr)->DInfo.dri_Font);
	    RenderScreenBar(scr, FALSE, IntuitionBase);
	}
    }
}

/**********************************************************************************/

void KillScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    if (scr->BarLayer)
    {
        DeleteLayer(0, scr->BarLayer);
	scr->BarLayer = FALSE;
    }
    
}

/**********************************************************************************/

void RenderScreenBar(struct Screen *scr, BOOL refresh, struct IntuitionBase *IntuitionBase)
{
    struct DrawInfo *dri = &((struct IntScreen *)scr)->DInfo;
    struct RastPort *rp;
    
    if (scr->BarLayer)
    {
        rp = scr->BarLayer->rp;
	
	/* must lock GadgetLock to avoid deadlocks with ObtainGIRPort
	   when calling refreshgadget inside layer update state */
	   
	ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
	LockLayerRom(scr->BarLayer);

	if (refresh) BeginUpdate(scr->BarLayer);
	
	/* real BarHeight = scr->BarHeight + 1 !!! */
	
	SetDrMd(rp, JAM2);
	
	SetAPen(rp, dri->dri_Pens[BARBLOCKPEN]);
	RectFill(rp, 0, 0, scr->Width - 1, scr->BarHeight - 1);
	
	SetAPen(rp, dri->dri_Pens[BARTRIMPEN]);
	RectFill(rp, 0, scr->BarHeight, scr->Width - 1, scr->BarHeight);

	if (!scr->Title)
	{
	    scr->Title = scr->DefaultTitle;
	}
	if (scr->Title)
	{
	    SetAPen(rp, dri->dri_Pens[BARDETAILPEN]);
	    SetBPen(rp, dri->dri_Pens[BARBLOCKPEN]);
	    Move(rp, scr->BarHBorder, scr->BarVBorder + rp->TxBaseline);
	    Text(rp, scr->Title, strlen(scr->Title));
	   /*   D(bug("screen title render (%d,%d,%s)\n", scr->BarHBorder, scr->BarVBorder + rp->TxBaseline, scr->Title)); */
	}

	if (scr->FirstGadget)
	{
	    RefreshBoopsiGadget(scr->FirstGadget, (struct Window *)scr, IntuitionBase);
        }
	
	if (refresh)
	{
	    scr->BarLayer->Flags &= ~LAYERREFRESH;
	    EndUpdate(scr->BarLayer, TRUE);
	}
	
	

	UnlockLayerRom(scr->BarLayer);
	ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
	
    } /* if (scr->BarLayer) */
}

/**********************************************************************************/

struct IntuiActionMessage *AllocIntuiActionMsg(UWORD code, struct Window *win,
					       struct IntuitionBase *IntuitionBase)
{
    struct IntuiActionMessage *msg;
    
    if ((msg = AllocMem(sizeof(struct IntuiActionMessage), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        msg->Code   = code;
	msg->Window = win;
	msg->Task   = FindTask(NULL);
    }
    
    return msg;
}

/**********************************************************************************/

void FreeIntuiActionMsg(struct IntuiActionMessage *msg, struct IntuitionBase *IntuitionBase)
{
    if (msg)
    {
        FreeMem(msg, sizeof(struct IntuiActionMessage));
    }
}

/**********************************************************************************/

void SendIntuiActionMsg(struct IntuiActionMessage *msg, struct IntuitionBase *IntuitionBase)
{    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);
    AddTail((struct List *)GetPrivIBase(IntuitionBase)->IntuiActionQueue, (struct Node *)msg);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->IntuiActionLock);
    
    AddNullEvent();
}

/**********************************************************************************/

BOOL AllocAndSendIntuiActionMsg(UWORD code, struct Window *win, struct IntuitionBase *IntuitionBase)
{
    struct IntuiActionMessage *msg;
    BOOL		      retval = FALSE;
    
    if ((msg = AllocIntuiActionMsg(code, win, IntuitionBase)))
    {
        SendIntuiActionMsg(msg, IntuitionBase);
	
	retval = TRUE;
    }
    
    return retval;
}


/**********************************************************************************/

void UpdateMouseCoords(struct Window *win)
{
    WORD scrmousex = win->WScreen->MouseX;
    WORD scrmousey = win->WScreen->MouseY;
    
    win->MouseX    = scrmousex - win->LeftEdge;
    win->MouseY    = scrmousey - win->TopEdge;

    /* stegerg: AmigaOS sets this even if window is not GZZ
       so we do the same as they are handy also for non-GZZ
       windows */

    win->GZZMouseX = scrmousex - (win->LeftEdge + win->BorderLeft);
    win->GZZMouseY = scrmousey - (win->TopEdge + win->BorderTop);
}

/**********************************************************************************/

/* subtract rectangle b from rectangle b. resulting rectangles will be put into
   destrectarray which must have place for at least 4 rectangles. Returns number
   of resulting rectangles */
   
#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

WORD SubtractRectFromRect(struct Rectangle *a, struct Rectangle *b, struct Rectangle *destrectarray)
{
    struct Rectangle 	intersect;    
    BOOL 		intersecting = FALSE;
    WORD		numrects = 0;
    
    /* calc. intersection between a and b */
    
    if (a->MinX <= b->MaxX) {
	if (a->MinY <= b->MaxY) {
	  if (a->MaxX >= b->MinX) {
		if (a->MaxY >= b->MinY) {
		    intersect.MinX = MAX(a->MinX, b->MinX);
		    intersect.MinY = MAX(a->MinY, b->MinY);
		    intersect.MaxX = MIN(a->MaxX, b->MaxX);
		    intersect.MaxY = MIN(a->MaxY, b->MaxY);
		    
		    intersecting = TRUE;
		}
	    }
	}
    }

    if (!intersecting)
    {   
        destrectarray[numrects++] = *a;
	
    } /* not intersecting */
    else
    {
        if (intersect.MinY > a->MinY) /* upper */
	{
	    destrectarray->MinX = a->MinX;
	    destrectarray->MinY = a->MinY;
	    destrectarray->MaxX = a->MaxX;
	    destrectarray->MaxY = intersect.MinY - 1;
	    
	    numrects++;destrectarray++;
	}
	
	if (intersect.MaxY < a->MaxY) /* lower */
	{
	    destrectarray->MinX = a->MinX;
	    destrectarray->MinY = intersect.MaxY + 1;
	    destrectarray->MaxX = a->MaxX;
	    destrectarray->MaxY = a->MaxY;

	    numrects++;destrectarray++;
	}

        if (intersect.MinX > a->MinX) /* left */
	{
	    destrectarray->MinX = a->MinX;
	    destrectarray->MinY = intersect.MinY;
	    destrectarray->MaxX = intersect.MinX - 1;
	    destrectarray->MaxY = intersect.MaxY;
	    
	    numrects++;destrectarray++;
	}
	
	if (intersect.MaxX < a->MaxX) /* right */
	{
	    destrectarray->MinX = intersect.MaxX + 1;
	    destrectarray->MinY = intersect.MinY;
	    destrectarray->MaxX = a->MaxX;
	    destrectarray->MaxY = intersect.MaxY;

	    numrects++;destrectarray++;
	}
	
    } /* intersecting */
    
    return numrects;

}

/**********************************************************************************/

LONG CalcResourceHash(APTR resource)
{
    LONG l1, l2, l3, l4, hash;
    
    /* FIXME: Probably sucks. I have no clue about this hash stuff */
    
    l1 = ((LONG)resource) & 0xFF;
    l2 = (((LONG)resource) >> 8) & 0xFF;
    l3 = (((LONG)resource) >> 16) & 0xFF;
    l4 = (((LONG)resource) >> 24) & 0xFF;
   
    hash = (l1 + l2 + l3 + l4) % RESOURCELIST_HASHSIZE;

    return hash;
}
/**********************************************************************************/

void AddResourceToList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase)
{
    struct HashNode *hn = NULL;
    LONG    	     hash;
    ULONG   	     ilock;
        
    switch(resourcetype)
    {
    	case RESOURCE_WINDOW:
	    hn = &((struct IntWindow *)resource)->hashnode;
	    hn->type = RESOURCE_WINDOW;
	    break;
	    
	default:
	    D(bug("AddResourceToList: Unknown resource type!!!\n"));
	    return;
    }  

    hash = CalcResourceHash(resource);

    hn->resource = resource;
	    
    ilock = LockIBase(0);
    AddTail((struct List *)&GetPrivIBase(IntuitionBase)->ResourceList[hash], (struct Node *)hn);
    UnlockIBase(ilock);
}

/**********************************************************************************/

void RemoveResourceFromList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase)
{
    struct HashNode *hn = NULL;
    ULONG   	     ilock;
    
    switch(resourcetype)
    {	
    	case RESOURCE_WINDOW:
	    hn = &((struct IntWindow *)resource)->hashnode;
	    break;
	    
	default:
	    D(bug("RemoveResourceFromList: Unknown resource type!!!\n"));
	    return;
    }
    
    if (hn->type != resourcetype)
    {
    	D(bug("RemoveResourceFromList: Panic. Resource Type mismatch!!!\n"));
    }
    
    ilock = LockIBase(0);
    Remove((struct Node *)hn);
    UnlockIBase(ilock);
}

/**********************************************************************************/

BOOL ResourceExisting(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase)
{
    struct HashNode *hn = NULL;
    LONG    	     hash;
    ULONG   	     ilock;
    BOOL    	     exists = FALSE;
    
    hash = CalcResourceHash(resource);
    
    ilock = LockIBase(0);
    ForeachNode((struct List *)&GetPrivIBase(IntuitionBase)->ResourceList[hash], hn)
    {
    	if ((hn->resource == resource) && (hn->type == resourcetype))
	{
	    exists = TRUE;
	    break;
	}
    }
    UnlockIBase(ilock);
    
    return exists;
}
/**********************************************************************************/
