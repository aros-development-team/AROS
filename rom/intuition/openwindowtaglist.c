/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/10/24 15:51:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/09/21 14:20:53  digulla
    Deleted empty line

    Revision 1.1  1996/09/17 16:18:17  digulla
    A new function


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <utility/tagitem.h>
#include <clib/exec_protos.h>
#include <clib/utility_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH2(struct Window *, OpenWindowTagList,

/*  SYNOPSIS */
	AROS_LHA(struct NewWindow *, newWindow, A0),
	AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 101, Intuition)

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
    struct TagItem * tag;
    struct NewWindow nw =
    {
	0,0, /* Left, Top */
	640,200, /* Width, Height */
	0xFF, 0xFF, /* DetailPen, BlockPen */
	0L, /* IDCMPFlags */
	0L, /* Flags */
	NULL, /* FirstGadget */
	NULL, /* CheckMark */
	NULL, /* Title */
	NULL, /* Screen */
	NULL, /* BitMap */
	0,0, /* MinWidth, MinHeight */
	-1,-1, /* MaxWidth, MaxHeight */
	WBENCHSCREEN /* Type */
    };
    struct Window * window;
    UBYTE * screenTitle = NULL;
    BOOL    autoAdjust	= FALSE;
    ULONG   innerWidth	= ~0L;
    ULONG   innerHeight = ~0L;

    if (newWindow)
	CopyMem (newWindow, &nw, sizeof (struct NewWindow));

    while ((tag = NextTagItem (&tagList)))
    {
	switch (tag->ti_Tag)
	{
	case WA_Left:	    nw.LeftEdge     = tag->ti_Data; break;
	case WA_Top:	    nw.TopEdge	    = tag->ti_Data; break;
	case WA_Width:	    nw.Width	    = tag->ti_Data; break;
	case WA_Height:     nw.Height	    = tag->ti_Data; break;
	case WA_IDCMP:	    nw.IDCMPFlags   = tag->ti_Data; break;
	case WA_Flags:	    nw.Flags	    = tag->ti_Data; break;
	case WA_MinWidth:   nw.MinWidth     = tag->ti_Data; break;
	case WA_MinHeight:  nw.MinHeight    = tag->ti_Data; break;
	case WA_MaxWidth:   nw.MaxWidth     = tag->ti_Data; break;
	case WA_MaxHeight:  nw.MaxHeight    = tag->ti_Data; break;

	case WA_Gadgets:    nw.FirstGadget  = (struct Gadget *)(tag->ti_Data); break;
	case WA_Title:	    nw.Title	    =	      (UBYTE *)(tag->ti_Data); break;

	case WA_ScreenTitle: screenTitle = (UBYTE *)tag->ti_Data; break;
	case WA_AutoAdjust:  autoAdjust  = (tag->ti_Data != 0); break;
	case WA_InnerWidth:  innerWidth  = tag->ti_Data; break;
	case WA_InnerHeight: innerHeight = tag->ti_Data; break;

#define MODIFY_FLAG(name)   if (tag->ti_Data) \
				nw.Flags |= (name); else nw.Flags &= ~(name)

	case WA_SizeGadget:	MODIFY_FLAG(WFLG_SIZEGADGET);       break;
	case WA_DragBar:	MODIFY_FLAG(WFLG_DRAGBAR);          break;
	case WA_DepthGadget:	MODIFY_FLAG(WFLG_DEPTHGADGET);      break;
	case WA_CloseGadget:	MODIFY_FLAG(WFLG_CLOSEGADGET);      break;
	case WA_Backdrop:	MODIFY_FLAG(WFLG_BACKDROP);         break;
	case WA_ReportMouse:	MODIFY_FLAG(WFLG_REPORTMOUSE);      break;
	case WA_NoCareRefresh:	MODIFY_FLAG(WFLG_NOCAREREFRESH);    break;
	case WA_Borderless:	MODIFY_FLAG(WFLG_BORDERLESS);       break;
	case WA_Activate:	MODIFY_FLAG(WFLG_ACTIVATE);         break;
	case WA_RMBTrap:	MODIFY_FLAG(WFLG_RMBTRAP);          break;
	case WA_WBenchWindow:	MODIFY_FLAG(WFLG_WBENCHWINDOW);     break;
	case WA_SizeBRight:	MODIFY_FLAG(WFLG_SIZEBRIGHT);       break;
	case WA_SizeBBottom:	MODIFY_FLAG(WFLG_SIZEBBOTTOM);      break;
	case WA_GimmeZeroZero:	MODIFY_FLAG(WFLG_GIMMEZEROZERO);    break;
	case WA_NewLookMenus:	MODIFY_FLAG(WFLG_NEWLOOKMENUS);     break;

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
	case WA_PubScreen:
	case WA_PubScreenFallBack:
	case WA_WindowName:
	case WA_Colors:
	case WA_Zoom:
	case WA_MouseQueue:
	case WA_RptQueue:
	case WA_BackFill:
	case WA_MenuHelp:
	case WA_NotifyDepth:
	case WA_Checkmark:
	case WA_AmigaKey:
	case WA_Pointer:
	case WA_BusyPointer:
	case WA_PointerDelay:
	case WA_HelpGroup:
	case WA_HelpGroupWindow:
	case WA_TabletMessages:
	    /* TODO */
	    break;
	} /* switch Tag */
    }

    window = OpenWindow (&nw);

    if (screenTitle != NULL)
	SetWindowTitles (window, (UBYTE *)~0L, screenTitle);

    return window;
    AROS_LIBFUNC_EXIT
} /* OpenWindowTagList */
