#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/preferences.h>
#include "intuition_preferences.h"
#include "intuition_intern.h"

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

void CheckRectFill(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2)
{
    if ((x2 >= x1) && (y2 >= y1))
    {
    	RectFill(rp, x1, y1, x2, y2);
    }
}

/**********************************************************************************/

#define TITLEBAR_HEIGHT (w->BorderTop)

BOOL createsysgads(struct Window *w, struct IntuitionBase *IntuitionBase)
{

    struct DrawInfo *dri;
    BOOL is_gzz;
    
    EnterFunc(bug("createsysgads(w=%p)\n", w));

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
	    
	    struct TagItem size_tags[] = {
	            {GA_RelRight,	-width + 1	},
		    {GA_RelBottom,	-height + 1 	},
		    {GA_Width,		width		},
		    {GA_Height,		height		},
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
	    
	    ReturnBool("createsysgads", TRUE);
	    
	} /* if (sysgads created) */
	
	disposesysgads(w, IntuitionBase);
	
    } /* if (got DrawInfo) */
    ReturnBool("createsysgads", FALSE);

}

/**********************************************************************************/

VOID disposesysgads(struct Window *w, struct IntuitionBase *IntuitionBase)
{
    /* Free system gadgets */
    UWORD i;
    
    for (i = 0; i < NUM_SYSGADS; i ++)
    {
        if (SYSGAD(w, i))
	{
	    RemoveGadget( w, (struct Gadget *)SYSGAD(w, i));
	    DisposeObject( SYSGAD(w, i) );
	}
    }
}

