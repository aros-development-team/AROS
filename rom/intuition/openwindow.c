/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function OpenWindow()
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include "boopsigadgets.h"
#include <exec/ports.h>

#ifndef DEBUG_OpenWindow
#   define DEBUG_OpenWindow 0
#endif
#undef DEBUG
#ifdef DEBUG_OpenWindow
#   define DEBUG 1
#endif
#	include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

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

    struct NewWindow nw;
    struct Window * w;
    struct TagItem *tag, *tagList;
    struct RastPort *rp;
    struct Hook *backfillhook = LAYERS_BACKFILL;
    struct Rectangle *zoomrectangle = NULL;
    
    UBYTE * screenTitle = NULL;
    BOOL    autoAdjust	= FALSE;
    ULONG   innerWidth	= ~0L;
    ULONG   innerHeight = ~0L;
    WORD    mousequeue = DEFAULTMOUSEQUEUE;
    ULONG   moreFlags = 0;
    
    ULONG lock;
    BOOL driver_init_done = FALSE;

    D(bug("OpenWindow (%p = { Left=%d Top=%d Width=%d Height=%d })\n"
	, newWindow
	, newWindow->LeftEdge
	, newWindow->TopEdge
	, newWindow->Width
	, newWindow->Height
    ));

    nw = *newWindow;
    
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
	    switch (tag->ti_Tag)
	    {
	    case WA_Left:	nw.LeftEdge     = tag->ti_Data; break;
	    case WA_Top:	nw.TopEdge	= tag->ti_Data; break;
	    case WA_Width:	nw.Width	= tag->ti_Data; break;
	    case WA_Height:     nw.Height	= tag->ti_Data; break;
	    case WA_IDCMP:	nw.IDCMPFlags   = tag->ti_Data; break;
	    case WA_Flags:	nw.Flags	= tag->ti_Data; break;
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
	    	zoomrectangle = (struct Rectangle *)tag->ti_Data;
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

	    case WA_PubScreenName:
		{
		    char buffer[MAXPUBSCREENNAME + 1];
		    
		    if(tag->ti_Data == NULL)
		    {
			GetDefaultPubScreen(buffer);
			nw.Screen = LockPubScreen(buffer);
		    }
		    else
			nw.Screen = LockPubScreen((UBYTE *)tag->ti_Data);
		    
		    if(nw.Screen == NULL)
		    {
			BOOL fallback = (BOOL)GetTagData(WA_PubScreenFallBack,
							 FALSE, tagList);
			
			if(fallback)
			{
			    nw.Screen = GetPrivIBase(IntuitionBase)->DefaultPubScreen;

			    if(nw.Screen != NULL)
			    {
				GetDefaultPubScreen(buffer);
				nw.Screen = LockPubScreen(buffer);
			    }
			}
		    }
		    
		    break;
		}

	    case WA_PubScreen:
		if(tag->ti_Data == NULL)
		{
		    nw.Screen = GetPrivIBase(IntuitionBase)->DefaultPubScreen;
		    /* We may do this as the user must already have an
		       exclusive lock on the public screen. */
		    GetPrivScreen(GetPrivIBase(IntuitionBase)->DefaultPubScreen)->pubScrNode->psn_VisitorCount++;
		}
		else
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
	    case WA_MenuHelp:
	    case WA_Checkmark:
	    case WA_AmigaKey:
	    case WA_Pointer:
	    case WA_BusyPointer:
	    case WA_PointerDelay:
	    case WA_HelpGroup:
	    case WA_HelpGroupWindow:
	    case WA_TabletMessages:
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
    
#define IW(x) ((struct IntWindow *)x)

    w  = AllocMem (intui_GetWindowSize (), MEMF_CLEAR);
    
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

    w->UserPort = CreateMsgPort();
    if (NULL == w->UserPort)
	goto failexit;

    if (!ModifyIDCMP (w, nw.IDCMPFlags))
	goto failexit;
	
    IW(w)->closeMessage = AllocMem(sizeof (struct closeMessage), MEMF_PUBLIC);
    if (NULL == IW(w)->closeMessage)
    	goto failexit;

    D(bug("modified IDCMP\n"));

/*    w->RPort	   = rp; */

    w->FirstGadget = nw.FirstGadget;


    if (nw.DetailPen == 0xFF) nw.DetailPen = 1;
    if (nw.BlockPen  == 0xFF) nw.BlockPen = 0;

    w->MinWidth  = nw.MinWidth;
    w->MinHeight = nw.MinHeight;
    w->MaxWidth  = nw.MaxWidth;
    w->MaxHeight = nw.MaxHeight;

    if (nw.Type == PUBLICSCREEN)
	w->WScreen = IntuitionBase->ActiveScreen;
    else if (nw.Type == CUSTOMSCREEN)
	w->WScreen = nw.Screen;
    else
	w->WScreen = GetPrivIBase(IntuitionBase)->WorkBench;

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

    if (w->Flags & (WFLG_SIZEBRIGHT | WFLG_SIZEBBOTTOM))
    {
        IPTR sizewidth = 16, sizeheight = 16;
	struct Image *im;
	struct DrawInfo *dri;
	
	if ((dri = GetScreenDrawInfo(w->WScreen)))
	{
	    struct TagItem imtags[] =
	    {
		{SYSIA_DrawInfo, dri},
		{SYSIA_Which, SIZEIMAGE},
		{TAG_DONE,0} 
	    };
	
	    if ((im = NewObjectA(NULL, SYSICLASS, imtags)))
	    {
	    	GetAttr(IA_Width, im, &sizewidth);
		GetAttr(IA_Height, im, &sizeheight);
		 
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

    if (innerWidth != ~0L) nw.Width = innerWidth + w->BorderLeft + w->BorderRight;
    if (innerHeight != ~0L) nw.Height = innerHeight + w->BorderTop + w->BorderBottom;
    
    w->LeftEdge    = nw.LeftEdge;
    w->TopEdge	   = nw.TopEdge;
    w->Width	   = nw.Width;
    w->Height	   = nw.Height;

    if (autoAdjust)
    {
    	if (w->Width  > w->WScreen->Width)  w->Width  = w->WScreen->Width;
	if (w->Height > w->WScreen->Height) w->Height = w->WScreen->Height;

	if (w->LeftEdge < 0) w->LeftEdge = 0;
	if (w->TopEdge < 0) w->TopEdge = 0;
	
	if ((w->LeftEdge + w->Width) > w->WScreen->Width)
	    w->LeftEdge = w->WScreen->Width - w->Width;
	if ((w->TopEdge + w->Height) > w->WScreen->Height)
	    w->TopEdge = w->WScreen->Height - w->Height;
    }
    if (zoomrectangle)
    {
	((struct IntWindow *)w)->ZipLeftEdge = zoomrectangle->MinX;
	((struct IntWindow *)w)->ZipTopEdge  = zoomrectangle->MinY;
	((struct IntWindow *)w)->ZipWidth    = zoomrectangle->MaxX - zoomrectangle->MinX + 1;
	((struct IntWindow *)w)->ZipHeight   = zoomrectangle->MaxY - zoomrectangle->MinY + 1;    	
    }
    else
    {
	((struct IntWindow *)w)->ZipLeftEdge = w->LeftEdge;
	((struct IntWindow *)w)->ZipTopEdge  = w->TopEdge;
	((struct IntWindow *)w)->ZipWidth    = w->Width;
	((struct IntWindow *)w)->ZipHeight   = w->Height;
    }
    
    IW(w)->mousequeue = mousequeue;
    
    if (!intui_OpenWindow (w, IntuitionBase, nw.BitMap, backfillhook))
	goto failexit;

/* nlorentz: The driver has in some way or another allocated a rastport for us,
   which now is ready for us to use. */
    driver_init_done = TRUE;
    rp = w->RPort;

    D(bug("called driver, rp=%p\n", rp));
    if (w->WScreen->Font)
	SetFont (rp, ((struct IntScreen *)(w->WScreen))->DInfo.dri_Font);
    else
	SetFont (rp, GfxBase->DefaultFont);

    D(bug("set fonts\n"));

    SetAPen (rp, nw.DetailPen);
    SetBPen (rp, nw.BlockPen);
    SetDrMd (rp, JAM2);

    D(bug("set pens\n"));
    SetWindowTitles (w, nw.Title, (STRPTR)-1);
    D(bug("set title\n"));

    lock = LockIBase (0);

    w->Parent = NULL;
    w->NextWindow = w->Descendant = w->WScreen->FirstWindow;
    if (w->Descendant)
    {
    	w->Descendant->Parent = w;
    }

    w->WScreen->FirstWindow = w;
    w->WindowPort = GetPrivIBase(IntuitionBase)->IntuiReplyPort;


    UnlockIBase (lock);
kprintf("%s: Calling DoGMLayout!\n",__FUNCTION__);
    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    DoGMLayout(w->FirstGadget, w, NULL, -1, TRUE, IntuitionBase);

kprintf("%s: Calling RefreshGadgets!\n",__FUNCTION__);
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
	SetWindowTitles (w, (UBYTE *)~0L, screenTitle);

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
	    FreeMem(IW(w)->closeMessage, sizeof (struct closeMessage));
	
	if (driver_init_done)
	    intui_CloseWindow(w, IntuitionBase);
	
	if (w->UserPort)
	    DeleteMsgPort(w->UserPort);
	
	FreeMem (w, intui_GetWindowSize ());
	
	w = NULL;
    }
    
exit:
    ReturnPtr ("OpenWindow", struct Window *, w);
    AROS_LIBFUNC_EXIT
} /* OpenWindow */
