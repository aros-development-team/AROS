/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Open a new screen
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>


#ifndef DEBUG_OpenScreen
#   define DEBUG_OpenScreen 0
#endif
#undef DEBUG
#if DEBUG_OpenScreen
#   define DEBUG 1
#endif
#	include <aros/debug.h>

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
#include <intuition/screens.h>
#include <proto/intuition.h>

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
    ULONG            *errorPtr;	  /* Store error at user specified location */

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
	    case SA_Type:	ns.Type	     = tag->ti_Data; break;

	    case SA_Title:
		ns.DefaultTitle = (UBYTE *)tag->ti_Data;
		break;

	    case SA_Font:
		ns.Font = (struct TextAttr *)tag->ti_Data;
		break;

	    case SA_Colors:
	    case SA_SysFont:
	    case SA_BitMap:
		break;


		/* Name of this public screen. */
	    case SA_PubName:
		if(tag->ti_Data == NULL)
		    break;

		{
		    struct Screen *old;
		    
		    LockPubScreenList();
		    
		    old = LockPubScreen((STRPTR)tag->ti_Data);
		    
		    if(old != NULL)
		    {
			UnlockPubScreen(NULL, old);
			SetError(OSERR_PUBNOTUNIQUE);
			UnlockPubScreenList();
			return NULL;
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
		
	    case SA_DisplayID:
	    case SA_DClip:
	    case SA_Overscan:
	    case SA_ShowTitle:
	    case SA_Behind:
	    case SA_Quiet:
	    case SA_AutoScroll:
	    case SA_Pens:
	    case SA_FullPalette:
	    case SA_ColorMapEntries:
	    case SA_Parent:
	    case SA_Draggable:
	    case SA_Exclusive:
	    case SA_SharePens:
	    case SA_BackFill:
	    case SA_Interleaved:
	    case SA_Colors32:
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
    success = InitRastPort (&screen->Screen.RastPort);      
    screen->Screen.RastPort.BitMap = AllocBitMap(ns.Width, 
						 ns.Height, 
						 ns.Depth, 
						 BMF_CLEAR | BMF_DISPLAYABLE , 
						 NULL);
    D(bug("got bitmap\n"));	    
    
    /* Init screens viewport (probably not necessary, but I'll do it anyway */
    InitVPort(&screen->Screen.ViewPort);
    
    /* Allocate a RasInfo struct in which we have  a pointer
       to the struct BitMap, into which the driver can
       store its stuff. (Eg. pointer to a BitMap HIDD object) 
    */
    screen->Screen.ViewPort.RasInfo = AllocMem(sizeof(struct RasInfo), MEMF_ANY | MEMF_CLEAR);
    
    
    if(!success || (NULL == screen->Screen.RastPort.BitMap) || (NULL == screen->Screen.ViewPort.RasInfo))
    {
	if (screen->Screen.RastPort.BitMap)
            FreeBitMap(screen->Screen.RastPort.BitMap);
	
	if (screen->Screen.ViewPort.RasInfo)
	    FreeMem(screen->Screen.ViewPort.RasInfo, sizeof (struct RasInfo));
	
	FreeMem (screen, sizeof (struct IntScreen));
	screen = NULL;
	return NULL;
    }
    else
    {
	/* Store pointer to bitmap, so we can get hold of it
	   from withing LoadRGBxx() functions
	*/
        D(bug("got allocated stuff\n"));	    
	screen->Screen.ViewPort.RasInfo->BitMap = screen->Screen.RastPort.BitMap;
    }
    
    if (screen)
    {
        D(bug("Loading colors\n"));
        /* Load some default colors for the screen */
	LoadRGB32(&screen->Screen.ViewPort, (ULONG *)coltab);
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
	screen->Screen.RastPort.BitMap->Flags |= BMF_AROS_DISPLAYED;
        /* 
           Copy the data from the rastport's bitmap 
           to the screen's bitmap structure 
        */
	screen->Screen.BitMap = *screen->Screen.RastPort.BitMap;

	screen->Screen.BarHeight   = 0;
	screen->Screen.BarVBorder  = 0;
	screen->Screen.BarHBorder  = 0;
	screen->Screen.MenuVBorder = 0;
	screen->Screen.MenuHBorder = 0;

	screen->Screen.WBorTop    = 7;  /* Amiga default is 2 */
	screen->Screen.WBorLeft   = 4;
	screen->Screen.WBorRight  = 4;
	screen->Screen.WBorBottom = 4;  /* Amiga default is 2 */


	screen->Screen.Title = ns.DefaultTitle;

	screen->Screen.NextScreen = IntuitionBase->FirstScreen;
	IntuitionBase->FirstScreen =
	    IntuitionBase->ActiveScreen = &screen->Screen;

        D(bug("set active screen\n"));	    
	InitLayers(&screen->Screen.LayerInfo);

        D(bug("layers intited screen\n"));	    

	screen->DInfo.dri_Version = DRI_VERSION;
	screen->DInfo.dri_NumPens = NUMDRIPENS;
	screen->DInfo.dri_Pens = screen->Pens;
	screen->DInfo.dri_Depth = ns.Depth;
	screen->DInfo.dri_Resolution.X = 44;
	screen->DInfo.dri_Resolution.Y = 44;
	screen->DInfo.dri_Flags = 0;

	if (screen->Screen.Font)
	    screen->DInfo.dri_Font = OpenFont(screen->Screen.Font);

	if (!screen->DInfo.dri_Font)
	    screen->DInfo.dri_Font = GfxBase->DefaultFont;

	SetFont (&screen->Screen.RastPort, screen->DInfo.dri_Font);

        D(bug("fonts set\n"));	    

	screen->Pens[DETAILPEN] = screen->Screen.DetailPen;
	screen->Pens[BLOCKPEN]	= screen->Screen.BlockPen;
	screen->Pens[TEXTPEN] = 1;
	screen->Pens[SHINEPEN] = 2;
	screen->Pens[SHADOWPEN] = 1;
	screen->Pens[FILLPEN] = 3;
	screen->Pens[FILLTEXTPEN] = 2;
	screen->Pens[BACKGROUNDPEN] = 0;
	screen->Pens[HIGHLIGHTTEXTPEN] = 1;
	screen->Pens[BARDETAILPEN] = 1;
	screen->Pens[BARBLOCKPEN] = 2;
	screen->Pens[BARTRIMPEN] = 1;
	

        D(bug("callling SetRast()\n"));	    
	/* Set screen to background color */
	SetRast(&screen->Screen.RastPort, screen->Pens[BACKGROUNDPEN]);

        D(bug("SetRast() called\n"));	    

	/* If this is a public screen, we link it into the intuition global
	   public screen list */
	if(screen->pubScrNode != NULL)
	{
	    AddTail((struct List *)&GetPrivIBase(IntuitionBase)->PubScreenList,
		    (struct Node *)GetPrivScreen(screen)->pubScrNode);
	}

    }

    ReturnPtr ("OpenScreen", struct Screen *, &screen->Screen);
    AROS_LIBFUNC_EXIT
} /* OpenScreen */
