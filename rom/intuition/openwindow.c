/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function OpenWindow()
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include "boopsigadgets.h"
#include <exec/ports.h>

#ifndef DEBUG_OpenWindow
#   define DEBUG_OpenWindow 0
#endif
#undef DEBUG
#define DEBUG 0
#if DEBUG_OpenWindow
#   define DEBUG 1
#endif
#	include <aros/debug.h>


/*****************************************************************************

    NAME */

	AROS_LH1(struct Window *, OpenWindow,

/*  SYNOPSIS */
	AROS_LHA(struct NewWindow *, newWindow, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 34, Intuition)

/*  FUNCTION
	Opens a new window with the characteristics specified in
	newWindow.

    INPUTS
	newWindow - How you would like your new window.

    RESULT
	A pointer to the new window or NULL if it couldn't be
	opened. Reasons for this might be lack of memory or illegal
	attributes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseWindow(), ModifyIDCMP()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct NewWindow 	nw;
    struct Window   	*w = NULL, *helpgroupwindow = NULL, *parentwin = NULL;
    struct TagItem  	*tag, *tagList;
    struct RastPort 	*rp;
    struct Hook     	*backfillhook = LAYERS_BACKFILL, *shapehook = NULL;
    struct Region   	*shape = NULL;
    struct IBox     	*zoombox = NULL;
    struct Image    	*AmigaKey = NULL;
    struct Image    	*Checkmark = NULL;
    struct Layer        *parentl = NULL;
    
    STRPTR  	    	pubScreenName = NULL;
    UBYTE   	    	*screenTitle = NULL;
    BOOL    	    	autoAdjust = FALSE, pubScreenFallBack = FALSE;
    ULONG   	    	innerWidth = ~0L;
    ULONG   	    	innerHeight = ~0L;
    WORD    	    	mousequeue = DEFAULTMOUSEQUEUE;
    WORD    	    	repeatqueue = 3; /* stegerg: test on my Amiga suggests this */
    ULONG   	    	moreFlags = 0;
    ULONG   	    	helpgroup = 0;
    
    ULONG   	    	lock;
    ULONG               windowvisible = TRUE;
    BOOL    	    	driver_init_done = FALSE, have_helpgroup = FALSE;


    ASSERT_VALID_PTR(newWindow);

    D(bug("OpenWindow (%p = { Left=%d Top=%d Width=%d Height=%d })\n"
	, newWindow
	, newWindow->LeftEdge
	, newWindow->TopEdge
	, newWindow->Width
	, newWindow->Height
    ));

    nw = *newWindow;

#define WFLG_PRIVATEFLAGS (WFLG_WINDOWREFRESH | WFLG_WBENCHWINDOW  | \
			   WFLG_WINDOWTICKED  | WFLG_VISITOR       | \
			   WFLG_ZOOMED        | WFLG_HASZOOM       )

    nw.Flags &= ~WFLG_PRIVATEFLAGS;
        
    if (newWindow->Flags & WFLG_NW_EXTENDED)
    {
    	tagList = ((struct ExtNewWindow *)newWindow)->Extension;
    }
    else
    {
    	tagList = NULL;
    }

    if (tagList)
    {
	while ((tag = NextTagItem (&tagList)))
	{
	    /* ASSERT_VALID_PTR(tag) */

	    switch (tag->ti_Tag)
	    {
	    case WA_Left:	nw.LeftEdge     = tag->ti_Data; break;
	    case WA_Top:	nw.TopEdge	= tag->ti_Data; break;
	    case WA_Width:	nw.Width	= tag->ti_Data; break;
	    case WA_Height:     nw.Height	= tag->ti_Data; break;
	    case WA_IDCMP:	nw.IDCMPFlags   = tag->ti_Data; break;
	    case WA_Flags:	nw.Flags	= (nw.Flags & WFLG_PRIVATEFLAGS) |
	    					  (tag->ti_Data & ~WFLG_PRIVATEFLAGS); break;
	    case WA_MinWidth:   nw.MinWidth     = tag->ti_Data; break;
	    case WA_MinHeight:  nw.MinHeight    = tag->ti_Data; break;
	    case WA_MaxWidth:   nw.MaxWidth     = tag->ti_Data; break;
	    case WA_MaxHeight:  nw.MaxHeight    = tag->ti_Data; break;

	    case WA_Gadgets:    nw.FirstGadget  = (struct Gadget *)(tag->ti_Data); break;
	    case WA_Title:	nw.Title	= (UBYTE *)(tag->ti_Data); break;

	    case WA_ScreenTitle: screenTitle = (UBYTE *)tag->ti_Data; break;
	    case WA_AutoAdjust:  autoAdjust  = (tag->ti_Data != 0); break;
	    case WA_InnerWidth:  innerWidth  = tag->ti_Data; break;
	    case WA_InnerHeight: innerHeight = tag->ti_Data; break;

#define MODIFY_FLAG(name)   if (tag->ti_Data) \
			    nw.Flags |= (name); else nw.Flags &= ~(name)
#define MODIFY_MFLAG(name)  if (tag->ti_Data) \
			    moreFlags |= (name); else moreFlags &= ~(name)

	    case WA_SizeGadget:	    MODIFY_FLAG(WFLG_SIZEGADGET);      break;
	    case WA_DragBar:	    MODIFY_FLAG(WFLG_DRAGBAR);         break;
	    case WA_DepthGadget:    MODIFY_FLAG(WFLG_DEPTHGADGET);     break;
	    case WA_CloseGadget:    MODIFY_FLAG(WFLG_CLOSEGADGET);     break;
	    case WA_Backdrop:	    MODIFY_FLAG(WFLG_BACKDROP);        break;
	    case WA_ReportMouse:    MODIFY_FLAG(WFLG_REPORTMOUSE);     break;
	    case WA_NoCareRefresh:  MODIFY_FLAG(WFLG_NOCAREREFRESH);   break;
	    case WA_Borderless:	    MODIFY_FLAG(WFLG_BORDERLESS);      break;
	    case WA_Activate:	    MODIFY_FLAG(WFLG_ACTIVATE);        break;
	    case WA_RMBTrap:	    MODIFY_FLAG(WFLG_RMBTRAP);         break;
	    case WA_WBenchWindow:   MODIFY_FLAG(WFLG_WBENCHWINDOW);    break;
	    case WA_SizeBRight:	    MODIFY_FLAG(WFLG_SIZEBRIGHT);      break;
	    case WA_SizeBBottom:    MODIFY_FLAG(WFLG_SIZEBBOTTOM);     break;
	    case WA_GimmeZeroZero:  MODIFY_FLAG(WFLG_GIMMEZEROZERO);   break;
	    case WA_NewLookMenus:   MODIFY_FLAG(WFLG_NEWLOOKMENUS);    break;
	    case WA_Zoom:
	    	zoombox = (struct IBox *)tag->ti_Data;
		MODIFY_FLAG(WFLG_HASZOOM);
		break;

	    case WA_DetailPen:
		if (nw.DetailPen == 0xFF)
		    nw.DetailPen = tag->ti_Data;
		break;

	    case WA_BlockPen:
		if (nw.BlockPen == 0xFF)
		    nw.BlockPen = tag->ti_Data;
		break;

	    case WA_CustomScreen:
		nw.Screen = (struct Screen *)(tag->ti_Data);
		nw.Type = CUSTOMSCREEN;
		break;

	    case WA_SuperBitMap:
		nw.Flags |= WFLG_SUPER_BITMAP;
		nw.BitMap = (struct BitMap *)(tag->ti_Data);
		break;

	    case WA_SimpleRefresh:
		if (tag->ti_Data)
		    nw.Flags |= WFLG_SIMPLE_REFRESH;
		break;

	    case WA_SmartRefresh:
		if (tag->ti_Data)
		    nw.Flags |= WFLG_SMART_REFRESH;
		break;

	    case WA_PubScreenFallBack:
	    	pubScreenFallBack = (tag->ti_Data ? TRUE : FALSE);
		break;

	    case WA_PubScreenName:
	    	nw.Type = PUBLICSCREEN;
	        pubScreenName = (STRPTR)tag->ti_Data;
		break;
		    
	    case WA_PubScreen:
	    	nw.Type = PUBLICSCREEN;
		nw.Screen = (struct Screen *)tag->ti_Data;
		break;

	    case WA_BackFill:
		backfillhook = (struct Hook *)tag->ti_Data;
		break;
	    
	    case WA_MouseQueue:
		mousequeue = tag->ti_Data;
	    	break;
		
	    /* These two are not implemented in AmigaOS */
	    case WA_WindowName:
	    case WA_Colors:
		break;

	    case WA_NotifyDepth:
		MODIFY_MFLAG(WMFLG_NOTIFYDEPTH);
		break;

	    case WA_RptQueue:
	    	repeatqueue = tag->ti_Data;
		break;

	    case WA_Checkmark:
	        Checkmark = (struct Image *)tag->ti_Data;
		break;
		
	    case WA_AmigaKey:
		AmigaKey = (struct Image *)tag->ti_Data;
		break;

	    case WA_HelpGroup:
	        helpgroup = (ULONG)tag->ti_Data;
		have_helpgroup = TRUE;
		break;
		
	    case WA_HelpGroupWindow:
		helpgroupwindow = (struct Window *)tag->ti_Data;
		break;
		
	    case WA_MenuHelp:
	        MODIFY_MFLAG(WMFLG_MENUHELP);
		break;
		
	    case WA_PointerDelay:
	        MODIFY_MFLAG(WMFLG_POINTERDELAY);
		break;

	    case WA_TabletMessages:
		MODIFY_MFLAG(WMFLG_TABLETMESSAGES);
		break;
	
	    case WA_Shape:
	    	shape = (struct Region *)tag->ti_Data;
		break;

    	    case WA_ShapeHook:
	    	shapehook = (struct Hook *)tag->ti_Data;
		break;
		
            case WA_Parent:
                parentwin = ((struct Window *)tag->ti_Data);
                parentl   = parentwin->WLayer;
                break;

            case WA_Visible:
                windowvisible = (ULONG)tag->ti_Data;
                break;

	    case WA_Pointer:
	    case WA_BusyPointer:
    #warning TODO: Missing WA_ Tags
		break;
		
	    } /* switch Tag */

	} /* while ((tag = NextTagItem (&tagList))) */
	
    } /* if (tagList) */
    
    if (nw.Flags & WFLG_SIZEGADGET)
    {
    	if (!(nw.Flags & (WFLG_SIZEBRIGHT | WFLG_SIZEBBOTTOM)))
	{
	    nw.Flags |= WFLG_SIZEBRIGHT;
	}
    }
    
    /* Find out on which Screen the window must open */
    
    switch(nw.Type)
    {
        case CUSTOMSCREEN:
	    break;
	
	case WBENCHSCREEN:
	    nw.Screen = LockPubScreen("Workbench");
	    if (nw.Screen)
	    {
	        nw.Flags |= WFLG_VISITOR;
	        moreFlags |= WMFLG_DO_UNLOCKPUBSCREEN;
	    }
	    break;
	
	case PUBLICSCREEN:
	    moreFlags |= WMFLG_DO_UNLOCKPUBSCREEN;
	    if (pubScreenName)
	    {
	        nw.Screen = LockPubScreen(pubScreenName);
		if (!nw.Screen && pubScreenFallBack)
		{
		    nw.Screen = LockPubScreen(NULL);
		}
	    }
	    else
	    {
	    	if (nw.Screen)
		{
		    /* case: {WA_PubScreen, something != NULL} */
		    
		    moreFlags &= ~WMFLG_DO_UNLOCKPUBSCREEN;
		}
		else
		{
		    nw.Screen = LockPubScreen(NULL);
		}
	    }
	    
	    if (nw.Screen) nw.Flags |= WFLG_VISITOR;
	    break;
	    
    } /* switch(nw.Type) */
    
    if (nw.Screen == NULL)
        goto failexit;
    
    w  = AllocMem (sizeof(struct IntWindow), MEMF_CLEAR);
    
/* nlorentz: For now, creating a rastport becomes the responsibility of
   intui_OpenWindow(). This is because intui_OpenWindow() in
   config/hidd/intuition_driver.c must call CreateUpfrontLayer(),
   and that will create a rastport for the layer/window, and we don't
   want two rastports pr. window.
   Alternatively we may create a layers_driver.c driver for layers,
   and then call CreateUpfrontLayer() here from openwindow.
   For the Amiga window<-->X11 window stuff, the layers driver
   would just allocate a layer struct, a rastport and
   put the rasport into layer->RastPort, so we
   could get it inside this routine and put it into
   window->RPort;.
   
*/    

    if (NULL == w)
    	goto failexit;

    w->WScreen = nw.Screen;
    
    w->UserPort = CreateMsgPort();
    if (NULL == w->UserPort)
	goto failexit;

    if (!ModifyIDCMP (w, nw.IDCMPFlags))
	goto failexit;
	
    IW(w)->closeMessage = AllocIntuiActionMsg(AMCODE_CLOSEWINDOW, w, IntuitionBase);
    if (NULL == IW(w)->closeMessage)
    	goto failexit;

/*    w->RPort	   = rp; */

    w->FirstGadget = nw.FirstGadget;

    w->DetailPen = (nw.DetailPen != 0xFF) ? nw.DetailPen : w->WScreen->DetailPen;
    w->BlockPen  = (nw.BlockPen != 0xFF)  ? nw.BlockPen  : w->WScreen->BlockPen;

    /* Copy flags */
    w->Flags = nw.Flags;
    w->MoreFlags = moreFlags;

    if (!(w->Flags & WFLG_BORDERLESS))
    {
	w->BorderLeft   = w->WScreen->WBorLeft;
	w->BorderRight  = w->WScreen->WBorRight;
	w->BorderTop    = w->WScreen->WBorTop;
	w->BorderBottom = w->WScreen->WBorBottom;
    }
    
    if (nw.Title || (w->Flags & (WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET)))
    {
    	/* this is a hack. the correct way to "correct" (increase if necessary)
	   the w->Border??? items would be to check all GACT_???BORDER gadgets
	   (inclusive sysgadgets which are GACT_????BORDER gadgets as well) in
	   nw.FirstGadget (or WA_Gadgets tag) and all sysgadgets and then
	   make sure that each window border is big enough so that none of these
	   gadgets extends outside the window border area */
	   
    	/* Georg Steger: ??? font ??? */
    	if (w->WScreen->Font)
	    w->BorderTop += ((struct IntScreen *)(w->WScreen))->DInfo.dri_Font->tf_YSize + 1;
	else
	    w->BorderTop += GfxBase->DefaultFont->tf_YSize + 1;
    }

    if ((w->Flags & WFLG_SIZEGADGET) && 
        (w->Flags & (WFLG_SIZEBRIGHT | WFLG_SIZEBBOTTOM)))
    {
        IPTR sizewidth = 16, sizeheight = 16;
	struct Image *im;
	struct DrawInfo *dri;
	
	if ((dri = GetScreenDrawInfo(w->WScreen)))
	{
	    struct TagItem imtags[] =
	    {
		{SYSIA_DrawInfo, (STACKIPTR)dri},
		{SYSIA_Which, SIZEIMAGE},
		{TAG_DONE,0} 
	    };
	
	    if ((im = NewObjectA(NULL, SYSICLASS, imtags)))
	    {
	    	GetAttr(IA_Width, (Object *)im, &sizewidth);
		GetAttr(IA_Height, (Object *)im, &sizeheight);
		 
	    	DisposeObject(im);
	    }
	    FreeScreenDrawInfo(w->WScreen, dri);
	}

	if (w->Flags & WFLG_SIZEBRIGHT)
	{
    	    if (w->BorderRight < sizewidth) w->BorderRight = sizewidth;
	}

	if (w->Flags & WFLG_SIZEBBOTTOM)
	{
    	    if (w->BorderBottom < sizeheight) w->BorderBottom = sizeheight;
	}
	
	IW(w)->sizeimage_width = sizewidth;
	IW(w)->sizeimage_height = sizeheight;
    }

    /* look for GACT_???BORDER gadgets which increase the BorderSizes */
    
    if (nw.FirstGadget)
    {
        struct Gadget *gad;
	
	for(gad = nw.FirstGadget; gad; gad = gad->NextGadget)
	{
	    WORD gadx1, gady1, gadx2, gady2;
	    
	    if (gad->Activation & GACT_LEFTBORDER)
	    {
	        /* may never be GFLG_RELRIGHT / GFLG_RELWIDTH */
		
		gadx2 = gad->LeftEdge + gad->Width - 1;
		if (gadx2 >= w->BorderLeft) w->BorderLeft = gadx2 + 1;
	    }
	    
	    if (gad->Activation & GACT_TOPBORDER)
	    {
	        /* may never be GFLG_RELBOTTOM / GFLG_RELHEIGHT */
		
	        gady2 = gad->TopEdge + gad->Height - 1;
		if (gady2 >= w->BorderTop) w->BorderTop = gady2 + 1;
	    }
	    
	    if (gad->Activation & GACT_RIGHTBORDER)
	    {
	        /* must be GFLG_RELRIGHT but never GFLG_RELWIDTH */
		
		gadx1 = -gad->LeftEdge;
		if (gadx1 >= w->BorderRight) w->BorderRight = gadx1 + 1;
	    }
	    
	    if (gad->Activation & GACT_BOTTOMBORDER)
	    {
	        /* must be GFLG_RELBOTTOM but never GFLG_RELHEIGHT */
		
		gady1 = -gad->TopEdge;
		if (gady1 >= w->BorderBottom) w->BorderBottom = gady1 + 1;
	    }
	    	    
	} /* for(gad = nw.FirstGadget; gad; gad = gad->NextGadget) */
	
    } /* if (nw.FirstGadget) */
    
    if (innerWidth != ~0L) nw.Width = innerWidth + w->BorderLeft + w->BorderRight;
    if (innerHeight != ~0L) nw.Height = innerHeight + w->BorderTop + w->BorderBottom;

    w->RelLeftEdge = nw.LeftEdge;
    w->RelTopEdge  = nw.TopEdge;
    
    {
    	LONG parentwidth;
	LONG parentheight;
	
	parentwidth  = parentwin ? parentwin->Width : w->WScreen->Width;
	parentheight = parentwin ? parentwin->Height : w->WScreen->Height;
    
	w->Width   = (nw.Width  != ~0) ? nw.Width  : parentwidth - w->RelLeftEdge;
	w->Height  = (nw.Height != ~0) ? nw.Height : parentheight - w->RelTopEdge;
	
	if (autoAdjust)
	{

    	    if (w->Width  > parentwidth)  w->Width  = parentwidth;
	    if (w->Height > parentheight) w->Height = parentheight;

	    if (w->RelLeftEdge < 0) w->RelLeftEdge = 0;
	    if (w->RelTopEdge < 0) w->RelTopEdge = 0;

	    if ((w->RelLeftEdge + w->Width) > parentwidth)
		w->RelLeftEdge = parentwidth - w->Width;
	    if ((w->RelTopEdge + w->Height) > parentheight)
		w->RelTopEdge = parentheight - w->Height;
	}
	
	w->GZZWidth  = w->Width  - w->BorderLeft - w->BorderRight;
	w->GZZHeight = w->Height - w->BorderTop  - w->BorderBottom;
    }
    
    if (NULL == parentwin)
    {
	w->LeftEdge    = w->RelLeftEdge;
	w->TopEdge     = w->RelTopEdge;
    }
    else
    {
	w->LeftEdge    = w->RelLeftEdge + parentwin->LeftEdge;
	w->TopEdge     = w->RelTopEdge  + parentwin->TopEdge;
    }

    w->MinWidth  = (nw.MinWidth  != 0) ? nw.MinWidth  : w->Width;
    w->MinHeight = (nw.MinHeight != 0) ? nw.MinHeight : w->Height;
    w->MaxWidth  = (nw.MaxWidth  != 0) ? nw.MaxWidth  : w->Width;
    w->MaxHeight = (nw.MaxHeight != 0) ? nw.MaxHeight : w->Height;

    if (zoombox)
    {
	((struct IntWindow *)w)->ZipLeftEdge = zoombox->Left;
	((struct IntWindow *)w)->ZipTopEdge  = zoombox->Top;
	((struct IntWindow *)w)->ZipWidth    = zoombox->Width;
	((struct IntWindow *)w)->ZipHeight   = zoombox->Height;    	
    }
    else
    {
	((struct IntWindow *)w)->ZipLeftEdge = w->RelLeftEdge;
	((struct IntWindow *)w)->ZipTopEdge  = w->RelTopEdge;
	((struct IntWindow *)w)->ZipWidth    = w->Width;
	((struct IntWindow *)w)->ZipHeight   = w->Height;
    }
    
    IW(w)->mousequeue = mousequeue;
    IW(w)->repeatqueue = repeatqueue;
    
    /* Amiga and checkmark images for menus */
    
    IW(w)->Checkmark = Checkmark ? Checkmark :
				   ((struct IntScreen *)(w->WScreen))->DInfo.dri_CheckMark;

    IW(w)->AmigaKey  = AmigaKey  ? AmigaKey  :
				   ((struct IntScreen *)(w->WScreen))->DInfo.dri_AmigaKey;
    
    /* child support */
    if (NULL != parentwin)
    {
	if (parentwin->firstchild)
            parentwin->firstchild->prevchild = w;

	w->nextchild = parentwin->firstchild;
	parentwin->firstchild = w;
	w->parent = parentwin;
    }
    
    /* Help stuff */
   
    if (!have_helpgroup && helpgroupwindow)
    {
        if (IW(helpgroupwindow)->helpflags & HELPF_ISHELPGROUP)
	{
	    helpgroup = IW(helpgroupwindow)->helpgroup;
	    have_helpgroup = TRUE;
	}
    }
    
    if (have_helpgroup)
    {
        IW(w)->helpflags |= HELPF_ISHELPGROUP;
	IW(w)->helpgroup = helpgroup;
    }	
	
    if (!intui_OpenWindow (w, 
                           IntuitionBase, 
                           nw.BitMap, 
                           backfillhook, 
                           shape,
			   shapehook,
                           parentl,
                           windowvisible))
	goto failexit;

/* nlorentz: The driver has in some way or another allocated a rastport for us,
   which now is ready for us to use. */
   
    driver_init_done = TRUE;
    rp = w->RPort;

    D(bug("called driver, rp=%p\n", rp));

    /* The window RastPort always gets the font from GfxBase->DefaultFont, which
       is the system's default font. Usually topaz 8, but it can be changed with
       the Fonts prefs program to another fixed-sized font. */
    
#warning: Really hacky way of re-opening GfxBase->DefaultFont

    Forbid();
    w->IFont = GfxBase->DefaultFont;
    w->IFont->tf_Accessors++;
    Permit();
    SetFont (rp, w->IFont);
        
    D(bug("set fonts\n"));

#warning Remove workaround!
/* lbischoff: The following 4 Setxxx lines are a workaround for the InitRastPort 
   problem (Bug #75 in docs/BUGS). They ensure that at least a window's rastport
   is initialized correctly. Remove them if they are not needed any longer!
*/
    SetAPen (rp, rp->FgPen);
    SetBPen (rp, rp->BgPen);
    SetDrMd (rp, rp->DrawMode);
    SetWriteMask (rp, rp->Mask);
    D(bug("set pens\n"));

    SetWindowTitles (w, nw.Title, (CONST_STRPTR)-1);
    D(bug("set title\n"));

    lock = LockIBase (0);

    {
        /* insert new window into parent/descendant list 
	**
	** before: parent win xyz
	**            |
	**            |
	**            |
	**        descendant win abc
	**
	** after:  parent win xyz
	**              \
	** 	         \
	**	       newwindow w
	**	        /
	**	       /
	**	      /
	**	 descendant win abc 
	*/
	
        struct Window *parent, *descendant_of_parent;
	 
	parent = IntuitionBase->ActiveWindow;
	if (!parent) parent = w->WScreen->FirstWindow;
	if (parent)
	{
            descendant_of_parent = parent->Descendant;
	    parent->Descendant = w;
	    if (descendant_of_parent) descendant_of_parent->Parent = w;
	} else {
            descendant_of_parent = NULL;
	}

	w->Descendant = descendant_of_parent;
	w->Parent = parent;

	w->NextWindow = w->WScreen->FirstWindow;
	w->WScreen->FirstWindow = w;

    }
        
    w->WindowPort = GetPrivIBase(IntuitionBase)->IntuiReplyPort;


    UnlockIBase (lock);

    AddResourceToList(w, RESOURCE_WINDOW, IntuitionBase);
    
    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    DoGMLayout(w->FirstGadget, w, NULL, -1, TRUE, IntuitionBase);

    if (NULL != w->FirstGadget)
        RefreshGadgets (w->FirstGadget, w, NULL);
    
    /* !!! This does double refreshing as the system gadgets also are refreshed
       in the above RfreshGadgets() call */

    if (nw.Flags & WFLG_ACTIVATE)
    {
    	/* RefreshWindowFrame() will be called from within ActivateWindow().
	No point in doing double refreshing.
	
	!!! NOTE !!! If OpenWindow() is sometime in the future moved
	to input.device's context, one should call int_activatewindow
	instead. */
	
	ActivateWindow(w);
    }
    else
    {
	RefreshWindowFrame(w);
    }
    
    if (screenTitle != NULL)
	SetWindowTitles (w, (CONST_STRPTR)~0L, screenTitle);

    UpdateMouseCoords(w);
    
    goto exit;

failexit:
    D(bug("fail\n"));
        
    if (w)
    {
	ModifyIDCMP (w, 0L);
	
	/* nlorentz: Freeing the rasport is now intui_CloseWindow()'s task.

	   if (rp)
	   {
	       FreeRastPort (rp);
	   }
	*/
	
	if (IW(w)->closeMessage)
	    FreeIntuiActionMsg(IW(w)->closeMessage, IntuitionBase);
	
	if (driver_init_done)
	    intui_CloseWindow(w, IntuitionBase);
	
	if (w->IFont) CloseFont(w->IFont);
	
	if (w->UserPort)
	    DeleteMsgPort(w->UserPort);
	
	FreeMem (w, sizeof(struct IntWindow));
	
	w = NULL;
    }

    if (nw.Screen && (moreFlags & WMFLG_DO_UNLOCKPUBSCREEN))
    {
        UnlockPubScreen(NULL, nw.Screen);
    }
   
exit:
    ReturnPtr ("OpenWindow", struct Window *, w);
    AROS_LIBFUNC_EXIT
} /* OpenWindow */



/**********************************************************************************/

int intui_OpenWindow (struct Window * w,
	struct IntuitionBase * IntuitionBase,
	struct BitMap        * SuperBitMap,
	struct Hook          * backfillhook,
	struct Region	     * shape,
	struct Hook 	     * shapehook,
	struct Layer         * parent,
	ULONG                  visible)
{
    /* Create a layer for the window */
    LONG layerflags = 0;
    
    EnterFunc(bug("intui_OpenWindow(w=%p)\n", w));
    
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
          ReturnBool("intui_OpenWindow(No GimmeZeroZero layer)", FALSE);
	      			      
      /* install it as the BorderRPort */
      w->BorderRPort = L->rp;

      /* This layer belongs to a window */
      L->Window = (APTR)w;
     
      /* Now comes the inner window */
      w->WLayer = CreateUpfrontHookLayer( 
                   &w->WScreen->LayerInfo
	  	  , w->WScreen->RastPort.BitMap
		  , w->LeftEdge + w->BorderLeft  
		  , w->TopEdge  + w->BorderTop
		  , w->LeftEdge + w->BorderLeft + w->GZZWidth - 1
		  , w->TopEdge  + w->BorderTop + w->GZZHeight - 1
		  , layerflags
		  , backfillhook
		  , SuperBitMap);

      /* could this layer be created? If not then delete the outer window and exit */
      if (NULL == w->WLayer)
      {
        DeleteLayer(0, L);
        ReturnBool("intui_OpenWindow(No window layer)", FALSE);
      }	

      if ((layerflags & LAYERBACKDROP) && (w->WScreen->Flags & SHOWTITLE))
      {
          /* backdrop window was created over screen barlayer, but it must be
	     under the screen barlayer if screen has flag SHOWTITLE set */
	     
          Forbid();
	  w->WScreen->Flags &= ~SHOWTITLE;
	  Permit();
	  
	  ShowTitle(w->WScreen, TRUE);
      }
      	  
      /* That should do it, I guess... */
    }
    else
    {
#ifdef CreateLayerTagList
      struct TagItem win_tags[] =
      {
          {LA_Hook  	    , (IPTR)backfillhook	    	    	    	    	    	},
	  {LA_Priority	    , (layerflags & LAYERBACKDROP) ? BACKDROPPRIORITY : UPFRONTPRIORITY },
	  {LA_Shape 	    , (IPTR)shape   	    	    	    	    	    	    	},
    	  {LA_ShapeHook     , (IPTR)shapehook	    	    	    	    	    	    	},
	  {LA_SuperBitMap   , (IPTR)SuperBitMap							},
	  {LA_ChildOf       , (IPTR)parent							},
	  {LA_Visible       , (ULONG)visible                                                    },
	  {TAG_DONE 	    	    	    	    	    	    	    	    	    	}
      };
      
      w->WLayer = CreateLayerTagList(&w->WScreen->LayerInfo,
      	    	    	    	     w->WScreen->RastPort.BitMap,
				     w->RelLeftEdge,
				     w->RelTopEdge,
				     w->RelLeftEdge + w->Width - 1,
				     w->RelTopEdge + w->Height - 1,
				     layerflags,
				     win_tags);
				     
#else
      w->WLayer = CreateUpfrontHookLayer( 
                   &w->WScreen->LayerInfo
	  	  , w->WScreen->RastPort.BitMap
		  , w->RelLeftEdge
		  , w->RelTopEdge
		  , w->LeftEdge + w->Width - 1
		  , w->TopEdge  + w->Height - 1
		  , layerflags
		  , backfillhook
		  , SuperBitMap);
#endif

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
    	  ReturnBool("intui_OpenWindow(No BorderRPort)", FALSE);
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
	 
	if (CreateWinSysGadgets(w, IntuitionBase))
	{
    	    ReturnBool("intui_OpenWindow", TRUE);
	}
	intui_CloseWindow(w, IntuitionBase);
	
    } /* if (layer created) */
    
    ReturnBool("intui_OpenWindow(General failure)", FALSE);
}
