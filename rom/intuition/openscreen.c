/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a new screen
    Lang: english
*/
#include "intuition_intern.h"
#include <string.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <graphics/modeid.h>


#ifndef DEBUG_OpenScreen
#   define DEBUG_OpenScreen 0
#endif
#undef DEBUG
#if DEBUG_OpenScreen
#   define DEBUG 1
#endif
#include <aros/debug.h>

#if DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif


/* Default colors for the new screen */

static const ULONG coltab[] = { 
    (16L << 16) + 0,	/* 16 colors, loaded at index 0 */
    
    					/* X11 color names	*/
    0xB3B3B3B3, 0xB3B3B3B3, 0xB3B3B3B3, /* Grey70	*/
    0x00000000, 0x00000000, 0x00000000, /* Black	*/
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, /* White	*/
    0x66666666, 0x88888888, 0xBBBBBBBB, /* AMIGA Blue   */
    
    0x00000000, 0x00000000, 0xFFFFFFFF, /* Blue		*/
    0x00000000, 0xFFFFFFFF, 0x00000000, /* Green	*/
    0xFFFFFFFF, 0x00000000, 0x00000000, /* Red		*/
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, /* Cyan		*/
    
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, /* Magenta	*/
    0xEEEEEEEE, 0x82828282, 0xEEEEEEEE, /* Violet 	*/
    0xA5A5A5A5, 0x2A2A2A2A, 0x2A2A2A2A, /* Brown	*/
    0xFFFFFFFF, 0xE4E4E4E4, 0xC4C4C4C4, /* Bisque	*/
    
    0xE6E6E6E6, 0xE6E6E6E6, 0xFAFAFAFA, /* Lavender	*/
    0x00000000, 0x00000000, 0x80808080, /* Navy		*/
    0xF0F0F0F0, 0xE6E6E6E6, 0x8C8C8C8C, /* Khaki	*/
    0xA0A0A0A0, 0x52525252, 0x2D2D2D2D, /* Sienna	*/
    0L		/* Termination */
};    
/*****************************************************************************

    NAME */

	AROS_LH1(struct Screen *, OpenScreen,

/*  SYNOPSIS */
	AROS_LHA(struct NewScreen *, newScreen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 33, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)


    struct NewScreen  ns;
    struct TagItem   *tag, *tagList;
    struct IntScreen *screen;
    int               success;
    struct Hook      *layer_info_hook = NULL;
    struct ColorSpec *colors = NULL;
    ULONG            *errorPtr;	  /* Store error at user specified location */
    UWORD	     *customdripens = NULL;
    ULONG	     *colors32 = NULL;
    WORD    	      sysfont = -1;
    UWORD	      numcolormapcols = 0;
    BOOL	      ok = TRUE, rp_inited = FALSE, li_inited = FALSE, sharepens = FALSE;
    BOOL	      frontbm_set = FALSE;
    struct BitMap    *old_front_bm = NULL;
    
    struct TagItem   modetags[] = {
    	{ BIDTAG_Depth		, 0UL 	},
	{ BIDTAG_DesiredWidth	, 0UL 	},
	{ BIDTAG_DesiredHeight	, 0UL 	},
	{ TAG_DONE			}
    };
    
    ULONG modeid = INVALID_ID;
    
    ASSERT_VALID_PTR(newScreen);


#define COPY(x)     screen->Screen.x = ns.x
#define SetError(x) if(errorPtr != NULL) *errorPtr = x;

    D(bug("OpenScreen (%p = { Left=%d Top=%d Width=%d Height=%d Depth=%d })\n"
	, newScreen
	, newScreen->LeftEdge
	, newScreen->TopEdge
	, newScreen->Width
	, newScreen->Height
	, newScreen->Depth
    ));
    
    if (IntuitionBase->FirstScreen)
        old_front_bm = IntuitionBase->FirstScreen->RastPort.BitMap;

    ns = *newScreen;

    if (newScreen->Type & NS_EXTENDED)
    {
    	tagList = ((struct ExtNewScreen *)newScreen)->Extension;
    }
    else
    {
    	tagList = NULL;
    }

    screen = AllocMem(sizeof (struct IntScreen), MEMF_ANY | MEMF_CLEAR);

    /* Do this really early to be able to report errors */
    errorPtr = (ULONG *)GetTagData(SA_ErrorCode, NULL, tagList);
       
    if(screen == NULL)
    {
	SetError(OSERR_NOMEM);
	return NULL;
    }

    /* The RefreshLock semaphore is used for BeginRefresh/EndRefresh and
    ** when executing deferred actions in Intuition's inputhandler.
    **
    ** Without this semaphore there can be refreshing trouble when app
    ** and Intuition do their refreshing at the same time: apps
    ** usually do EndRefresh(TRUE) and without this sem this could happen
    ** before Intuition has finished its part of the refreshing --> updating
    ** state is ended too early.
    */
     
#if !USE_LOCKLAYERINFO_AS_REFRESHLOCK
    InitSemaphore(&screen->RefreshLock);
#endif
    
    if(tagList)
    {
	while((tag = NextTagItem (&tagList)))
	{
	    switch(tag->ti_Tag)
	    {
	    case SA_Left:	ns.LeftEdge  = tag->ti_Data; break;
	    case SA_Top:	ns.TopEdge   = tag->ti_Data; break;
	    case SA_Width:	ns.Width     = tag->ti_Data; break;
	    case SA_Height:     ns.Height    = tag->ti_Data; break;
	    case SA_Depth:	ns.Depth     = tag->ti_Data; break;
	    case SA_DetailPen:  ns.DetailPen = tag->ti_Data; break;
	    case SA_BlockPen:   ns.BlockPen  = tag->ti_Data; break;
	    case SA_Type:
	    	ns.Type	&= ~SCREENTYPE;
		ns.Type |= tag->ti_Data;
		break;

	    case SA_Title:
		ns.DefaultTitle = (UBYTE *)tag->ti_Data;
		break;

	    case SA_Font:
		ns.Font = (struct TextAttr *)tag->ti_Data;
		break;

	    case SA_Colors32:
	        colors32 = (ULONG *)tag->ti_Data;
		break;
		
	    case SA_Colors:
	        colors = (struct ColorSpec *)tag->ti_Data;
		break;
		
	    case SA_SysFont:
	    	sysfont = (WORD)tag->ti_Data;
		break;
		
	    case SA_BitMap:
		break;


		/* Name of this public screen. */
	    case SA_PubName:
		if(tag->ti_Data == NULL)
		    break;

		{
		    struct Screen *old;
		    
		    LockPubScreenList();
		    
		    if (strcmp((char *)tag->ti_Data, "Workbench") == 0)
		    {
#warning This would still not be safe, if a normal app tried to open its own screen with SA_PubName=Workbench
		        if (GetPrivIBase(IntuitionBase)->WorkBench)
			{
			    UnlockPubScreenList();
			    return NULL;
			}
		    }
		    else
		    {
			old = LockPubScreen((STRPTR)tag->ti_Data);

			if(old != NULL)
			{
			    UnlockPubScreen(NULL, old);
			    SetError(OSERR_PUBNOTUNIQUE);
			    UnlockPubScreenList();
			    return NULL;
			}
		    }
		    
		    UnlockPubScreenList();
		}			
		
		screen->pubScrNode = AllocMem(sizeof(struct PubScreenNode), MEMF_CLEAR);
		
		if(screen->pubScrNode == NULL)
		{
		    SetError(OSERR_NOMEM);
		    return NULL;
		}		
		
		screen->pubScrNode->psn_Node.ln_Name = AllocVec(MAXPUBSCREENNAME + 1,
								MEMF_ANY);
		
		if(screen->pubScrNode->psn_Node.ln_Name == NULL)
		{
		    SetError(OSERR_NOMEM);
		    FreeMem(screen->pubScrNode, sizeof(struct PubScreenNode));
		    return NULL;
		}
		
		/* Always open public screens in private mode. */
		screen->pubScrNode->psn_Flags |= PSNF_PRIVATE;
		strcpy(screen->pubScrNode->psn_Node.ln_Name, (STRPTR)tag->ti_Data);
		break;
		
		/* Signal bit number to use when signalling public screen
		   signal task. */
	    case SA_PubSig:
		if(screen->pubScrNode == NULL)
		    return NULL;
		
		/* If no PubTask is set, we set the calling task as default */
		if(screen->pubScrNode->psn_SigTask == NULL)
		    screen->pubScrNode->psn_SigTask = FindTask(NULL);
		
		screen->pubScrNode->psn_SigBit = (UBYTE)tag->ti_Data;
		break;
		
		/* Task that should be signalled when the public screen loses
		   its last visitor window. */
	    case SA_PubTask:
		if(screen->pubScrNode == NULL)
		    return NULL;
		
		screen->pubScrNode->psn_SigTask = (struct Task *)tag->ti_Data;
		break;

	    case SA_BackFill:
	          layer_info_hook = (struct Hook *)tag->ti_Data;
	        break;
	
	    case SA_Quiet:
	    	if (tag->ti_Data)
		{
		    ns.Type |= SCREENQUIET;
		} else {
		    ns.Type &= ~SCREENQUIET;
		}
		break;
		
	    case SA_ShowTitle:
	        if (tag->ti_Data)
		{
		    ns.Type |= SHOWTITLE;
		} else {
		    ns.Type &= ~SHOWTITLE;
		}
	        break;

	    case SA_Pens:
		customdripens = (UWORD *)tag->ti_Data;
		break;
		
	    case SA_DisplayID:
	    	modeid = tag->ti_Data;
		break;

	    case SA_SharePens:
		sharepens = tag->ti_Data ? TRUE : FALSE;
		break;
		
	    case SA_DClip:
	    case SA_Overscan:
	    case SA_Behind:
	    case SA_AutoScroll:
	    case SA_FullPalette:
	    case SA_ColorMapEntries:
	    case SA_Parent:
	    case SA_Draggable:
	    case SA_Exclusive:
	    case SA_Interleaved:
	    case SA_VideoControl:
	    case SA_FrontChild:
	    case SA_BackChild:
	    case SA_LikeWorkbench:
	    case SA_MinimizeISG:
    #warning TODO: Missing SA_ Tags
		break;

	    } /* switch (tag->ti_Tag) */

	} /* while ((tag = NextTagItem (&tagList))) */

    } /* if (tagList) */

    /* First Init the RastPort then get the BitPlanes!! */
    
    modetags[0].ti_Data  = ns.Depth;
    modetags[1].ti_Data = ns.Width;
    modetags[2].ti_Data = ns.Height;
    
    if (INVALID_ID == modeid) {
	modeid = BestModeIDA(modetags);
	if (INVALID_ID == modeid) {
    	    kprintf("!!! OpenScreen(): Could not find valid modeid !!!\n");
    	   return NULL;
	}
    }
	
    
    if ((success = InitRastPort (&screen->Screen.RastPort))) rp_inited = TRUE;
        
    screen->Screen.RastPort.BitMap = AllocScreenBitMap(modeid);
    D(bug("got bitmap\n"));	    
    
    /* Init screen's viewport */
    InitVPort(&screen->Screen.ViewPort);
    
    /* Allocate a RasInfo struct in which we have  a pointer
       to the struct BitMap, into which the driver can
       store its stuff. (Eg. pointer to a BitMap HIDD object) 
    */
    screen->Screen.ViewPort.RasInfo = AllocMem(sizeof(struct RasInfo), MEMF_ANY | MEMF_CLEAR);
    
    
    if(!success || 
       (NULL == screen->Screen.RastPort.BitMap) || 
       (NULL == screen->Screen.ViewPort.RasInfo))
    {
        ok = FALSE;
    }
    else
    {
	/* Store pointer to bitmap, so we can get hold of it
	   from withing LoadRGBxx() functions
	*/
        D(bug("got allocated stuff\n"));	    
	screen->Screen.ViewPort.RasInfo->BitMap = screen->Screen.RastPort.BitMap;
	
    }

    if (ok)
    {
	WORD numcolors = (ns.Depth <= 8) ? (1L << ns.Depth) : 256;

	/* Get a color map structure. Sufficient colors?? */

	if (NULL != (screen->Screen.ViewPort.ColorMap = GetColorMap(numcolors)))
	{
	    numcolormapcols = numcolors;
	    
            /* I should probably also call AttachPalExtra */
	    screen->Screen.ViewPort.ColorMap->VPModeID = modeid;
	    
	    
            if (0 != AttachPalExtra(screen->Screen.ViewPort.ColorMap,
                                    &screen->Screen.ViewPort))
                ok = FALSE;
	}
	else
            ok = FALSE;
    }
 
    if (ok)
    {
        D(bug("Loading colors\n"));
	
	/* First load default colors for the screen */
	LoadRGB32(&screen->Screen.ViewPort, (ULONG *)coltab);

	if (colors)  /* if SA_Colors tag exists */
	{
	    for(; colors->ColorIndex != (WORD)~0; colors++)
	    {
	        SetRGB4(&screen->Screen.ViewPort,
			colors->ColorIndex,
			colors->Red,
			colors->Green,
			colors->Blue);
	    }
	}

	if (colors32)  /* if SA_Colors32 tag exists */
	{
	    LoadRGB32(&screen->Screen.ViewPort, (const ULONG *)colors32);
	}

        D(bug("Loaded colors\n"));
	
	COPY(LeftEdge);
	COPY(TopEdge);
	COPY(Width);
	COPY(Height);
	COPY(DetailPen);
	COPY(BlockPen);
	COPY(Font);
	COPY(DefaultTitle);

	screen->Screen.Flags = ns.Type;

        /* Mark the bitmap of the screen as an AROS-displayed BitMap */
	screen->Screen.RastPort.BitMap->Flags |= BMF_AROS_HIDD;
        /* 
           Copy the data from the rastport's bitmap 
           to the screen's bitmap structure 
        */
	screen->Screen.BitMap = *screen->Screen.RastPort.BitMap;

	screen->Screen.WBorTop    = 3;  /* Amiga default is 2 */
	screen->Screen.WBorLeft   = 4;
	screen->Screen.WBorRight  = 4;
	screen->Screen.WBorBottom = 2;  /* Amiga default is 2 */

	screen->Screen.Title = ns.DefaultTitle;


	InitLayers(&screen->Screen.LayerInfo);
	li_inited = TRUE;

#if 0
    	/* Root layer now installed automatically by first call
	   to CreateLayerTagList */
	   
#ifdef CreateLayerTagList
	{
		struct TagItem tags[4] = {{LA_Visible, FALSE},
		                          {LA_Priority, ROOTPRIORITY},
		                          {TAG_DONE, -1}};

		screen->rootLayer = 
			CreateLayerTagList(&screen->Screen.LayerInfo,
			                   screen->Screen.RastPort.BitMap,
					   screen->Screen.LeftEdge,
					   screen->Screen.TopEdge,
					   screen->Screen.LeftEdge + screen->Screen.Width - 1,
					   screen->Screen.TopEdge + screen->Screen.Height - 1,
			                   0,
			                   tags);
	}
#endif
#endif

	if (NULL != layer_info_hook)
	  InstallLayerInfoHook(&screen->Screen.LayerInfo, layer_info_hook);

        D(bug("layers intited screen\n"));	    

	screen->DInfo.dri_Version = DRI_VERSION;
	screen->DInfo.dri_NumPens = NUMDRIPENS;
	screen->DInfo.dri_Pens = screen->Pens;
	/* dri_Depth is 8 on hi/true color screens like in AmigaOS with picasso96/cybergraphx */
	screen->DInfo.dri_Depth = (ns.Depth <= 8) ? ns.Depth : 8; 
	screen->DInfo.dri_Resolution.X = 44;
	screen->DInfo.dri_Resolution.Y = 44;
	screen->DInfo.dri_Flags = 0;
	
	/* SA_SysFont overrides SA_Font! */
	
    	if (sysfont == 0)
	{
	    /* Is handled below */
	}
	else if (sysfont == 1)
	{

#warning: Really hacky way of re-opening ScreenFont

    	    Forbid();
	    screen->DInfo.dri_Font = GetPrivIBase(IntuitionBase)->ScreenFont;
	    screen->DInfo.dri_Font->tf_Accessors++;
	    Permit();
    	    
	}
	else if (ns.Font)
	{
	    screen->DInfo.dri_Font = OpenFont(ns.Font);
    	}
	
	if (!screen->DInfo.dri_Font)
	{
	    /* GfxBase->DefaultFont is *not* always topaz 8. It
	       can be set with the Font prefs program!! */

#warning: Really hacky way of re-opening system default font
	    
	    Forbid();
	    screen->DInfo.dri_Font = GfxBase->DefaultFont;
	    screen->DInfo.dri_Font->tf_Accessors++;
	    Permit();
	}
	
	if (!screen->DInfo.dri_Font) ok = FALSE;

    } /* if (ok) */
    
    if (ok)
    {
        struct TagItem sysi_tags[] =
	{
	    {SYSIA_Which, 	MENUCHECK		},
	    {SYSIA_DrawInfo,	(IPTR)&screen->DInfo	},
	    {TAG_DONE					}	    
	};
	
        screen->DInfo.dri_CheckMark = NewObjectA(NULL, "sysiclass", sysi_tags);
	
	sysi_tags[0].ti_Data = AMIGAKEY;
	
	screen->DInfo.dri_AmigaKey  = NewObjectA(NULL, "sysiclass", sysi_tags);
	
	if (!screen->DInfo.dri_CheckMark || !screen->DInfo.dri_AmigaKey) ok = FALSE;
    }
    
    if (ok) {

	if (!SetFrontBitMap(screen->Screen.RastPort.BitMap, TRUE))
	    ok = FALSE;
	else
	    frontbm_set = TRUE;
    }
    
    if (ok)
    {
	SetFont(&screen->Screen.RastPort, screen->DInfo.dri_Font);

	AskFont(&screen->Screen.RastPort, &screen->textattr); 
        screen->Screen.Font = &screen->textattr;

        D(bug("fonts set\n"));	    

	screen->Screen.BarVBorder  = 4; /* on the Amiga it is (usually?) 1 */
	screen->Screen.BarHBorder  = 5;
	screen->Screen.MenuVBorder = 4; /* on teh Amiga it is (usually?) 2 */
	screen->Screen.MenuHBorder = 4;
	screen->Screen.BarHeight   = screen->DInfo.dri_Font->tf_YSize +
	    			     screen->Screen.BarVBorder * 2; /* real layer will be 1 pixel higher! */

	{
	    #define SDEPTH_HEIGHT (screen->Screen.BarHeight + 1)
	    #define SDEPTH_WIDTH SDEPTH_HEIGHT

	    struct TagItem sdepth_tags[] =
	    {
	    	{GA_Image   	, 0 	    	    	},
	        {GA_RelRight	, -SDEPTH_WIDTH + 1	},
		{GA_Top     	, 0  			},
		{GA_Width   	, SDEPTH_WIDTH		},
		{GA_Height  	, SDEPTH_HEIGHT		},
		{GA_SysGadget	, TRUE			},
		{GA_SysGType	, GTYP_SDEPTH	 	},
		{TAG_DONE		    	    	}
	    };
	    struct TagItem image_tags[] =
	    {
	    	{IA_Left    	, -1    	    	},
		{IA_Width   	, SDEPTH_WIDTH + 1	},
		{IA_Height  	, SDEPTH_HEIGHT   	},
		{SYSIA_Which	, SDEPTHIMAGE   	},
		{SYSIA_DrawInfo , (IPTR)&screen->DInfo	},
		{TAG_DONE   	    	    	    	}
	    };	    
    	    struct Object *im;
	    
	    im = NewObjectA(NULL, SYSICLASS, image_tags);
	    
	    if (im)
	    {
	    	sdepth_tags[0].ti_Data = (IPTR)im;
		
		screen->depthgadget = NewObjectA(NULL, BUTTONGCLASS, sdepth_tags );

		screen->Screen.FirstGadget = (struct Gadget *)screen->depthgadget;
		if (screen->Screen.FirstGadget)
		{
		    screen->Screen.FirstGadget->GadgetType |= GTYP_SCRGADGET;
		}
	    }
	}

	/* set default values for pens */
	/* if old-look or two-color-screen */
	if ((customdripens == NULL) || (ns.Depth == 1))
	{
            screen->Pens[DETAILPEN] = screen->Screen.DetailPen;
       	    screen->Pens[BLOCKPEN] = screen->Screen.BlockPen;
	    screen->Pens[TEXTPEN] = 1;
	    screen->Pens[SHINEPEN] = 1;
	    screen->Pens[SHADOWPEN] = 1;
	    screen->Pens[FILLPEN] = 1;
	    screen->Pens[FILLTEXTPEN] = 0;
	    screen->Pens[BACKGROUNDPEN] = 0;
	    screen->Pens[HIGHLIGHTTEXTPEN] = 1;
	    screen->Pens[BARDETAILPEN] = 0;
	    screen->Pens[BARBLOCKPEN] = 1;
	    screen->Pens[BARTRIMPEN] = 1;	
        }
	else /* if new-look and not two-color-screen */
	{
	    screen->Pens[DETAILPEN] = screen->Screen.DetailPen;
	    screen->Pens[BLOCKPEN] = screen->Screen.BlockPen;
	    screen->Pens[TEXTPEN] = 1;
	    screen->Pens[SHINEPEN] = 2;
	    screen->Pens[SHADOWPEN] = 1;
	    screen->Pens[FILLPEN] = 3;
	    screen->Pens[FILLTEXTPEN] = 1;
	    screen->Pens[BACKGROUNDPEN] = 0;
	    screen->Pens[HIGHLIGHTTEXTPEN] = 2;
	    screen->Pens[BARDETAILPEN] = 1;
	    screen->Pens[BARBLOCKPEN] = 2;
	    screen->Pens[BARTRIMPEN] = 1;

	    screen->DInfo.dri_Flags |= DRIF_NEWLOOK;
        }

	if (customdripens)
	{
	    WORD i;
	
	    for(i = 0; (i < NUMDRIPENS) && (customdripens[i] != (UWORD)~0); i++)
	    {
	        screen->Pens[i] = customdripens[i];
	    }
	}
	
	/* Allocate shared/exclusive colors */
	
	{
	    BYTE color_alloced[256];
	    
	    WORD i;
	    
	    for(i = 0; i < 256; i++) color_alloced[i] = FALSE;
	    
	    /* The Pens in the DrawInfo must be allocated as shared */
	    
	    for(i = 0; i < NUMDRIPENS; i++)
	    {
	        if (!color_alloced[screen->Pens[i]])
		{
		    ObtainPen(screen->Screen.ViewPort.ColorMap,
		    	      screen->Pens[i],
			      0,
			      0,
			      0,
			      PENF_NO_SETCOLOR);
			      
		    color_alloced[screen->Pens[i]] = TRUE;
		}
	    }
	    
	    /* If SA_SharePens is FALSE then allocate the rest of the colors
	       in the colormap as exclusive */
	    
	    if (!sharepens)
	    {   
		for(i = 0; i < numcolormapcols; i ++)
		{
	            if (!color_alloced[i])
		    {
			ObtainPen(screen->Screen.ViewPort.ColorMap,
		    		  i,
				  0,
				  0,
				  0,
				  PENF_EXCLUSIVE | PENF_NO_SETCOLOR);
				  
			color_alloced[i] = TRUE;
		    }
		}
		
	    } /* if (!sharepens) */
	    
	}
	
        D(bug("callling SetRast()\n"));	    
	/* Set screen to background color */
	SetRast(&screen->Screen.RastPort, screen->Pens[BACKGROUNDPEN]);

	if (!(screen->Screen.Flags & SCREENQUIET))
	{
	    CreateScreenBar(&screen->Screen, IntuitionBase);
	}

        D(bug("SetRast() called\n"));	    


  

	/* If this is a public screen, we link it into the intuition global
	   public screen list */
	if(screen->pubScrNode != NULL)
	{
	    /* Set the pointer to ourselves */
	    GetPrivScreen(screen)->pubScrNode->psn_Screen = &screen->Screen;
	    
	    AddTail((struct List *)&GetPrivIBase(IntuitionBase)->PubScreenList,
		    (struct Node *)GetPrivScreen(screen)->pubScrNode);
	}

	screen->Screen.NextScreen = IntuitionBase->FirstScreen;
	IntuitionBase->FirstScreen =
	    IntuitionBase->ActiveScreen = &screen->Screen;

        D(bug("set active screen\n"));

    } /* if (ok) */	    
 	
    if (!ok)
    {
    	if (frontbm_set) {
	    if (NULL != old_front_bm)
	    	SetFrontBitMap(old_front_bm, FALSE);
	}
	
	if (li_inited) ThinLayerInfo(&screen->Screen.LayerInfo);
	
        if (screen->Screen.ViewPort.ColorMap) FreeColorMap(screen->Screen.ViewPort.ColorMap);

        if (screen->Screen.BarLayer) KillScreenBar(&screen->Screen, IntuitionBase);
	
	if (screen->DInfo.dri_AmigaKey)  DisposeObject(screen->DInfo.dri_AmigaKey);
	if (screen->DInfo.dri_CheckMark) DisposeObject(screen->DInfo.dri_CheckMark);
	
        if (screen->DInfo.dri_Font) CloseFont(screen->DInfo.dri_Font);
	
	if (screen->Screen.RastPort.BitMap)
            FreeBitMap(screen->Screen.RastPort.BitMap);

	if (screen->Screen.ViewPort.RasInfo)
	    FreeMem(screen->Screen.ViewPort.RasInfo, sizeof (struct RasInfo));

	if (rp_inited) DeinitRastPort(&screen->Screen.RastPort);

	FreeMem (screen, sizeof (struct IntScreen));
    	
	screen = 0;
	
    } /* if (!ok) */
    
    ReturnPtr ("OpenScreen", struct Screen *, (struct Screen *)screen);
    
    AROS_LIBFUNC_EXIT
    
} /* OpenScreen */
