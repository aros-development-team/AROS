/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <string.h>
#include <intuition/intuition.h>
#include "asl_intern.h"

struct LayoutData *AllocCommon(ULONG, struct IntReq *, APTR, struct AslBase_intern *);
VOID FreeCommon(struct LayoutData *, struct AslBase_intern *);
BOOL HandleEvents(struct LayoutData *, struct AslReqInfo *, struct AslBase_intern *);


#define SDEBUG 0
#define DEBUG 0


#  include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/asl.h>
#include <utility/tagitem.h>

	AROS_LH2(BOOL, AslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR            , requester, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct Library *, AslBase, 10, Asl)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,AslBase)

    struct ReqNode	*reqnode;
    struct IntReq	*intreq;
    struct AslReqInfo	*reqinfo;
    struct LayoutData 	*ld;
    struct Window 	*win;	


    BOOL success = FALSE;
	
    D(bug("AslRequest(requester=%p, tagList=%p)\n", requester, tagList));
    
    if (!requester)
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return (FALSE);
    }
    
    /* Find the correspondent internal structure */
    reqnode = FindReqNode(requester, ASLB(AslBase));
    if (!reqnode)
    {
	return (FALSE);
    }
		
    intreq  = reqnode->rn_IntReq;
    reqinfo = &(ASLB(AslBase)->ReqInfo[intreq->ir_ReqType]);
    
/*  stegerg: this must be done when the Requester is terminated 
**           before setting the new output variables in the Requester struct.
**
**    StripRequester(requester, intreq->ir_ReqType, ASLB(AslBase));
*/
	
    /* Parse new tags if supplied */
    if (tagList)
    {
	struct ParseTagArgs pta;
		
	ParseCommonTags(intreq, tagList, ASLB(AslBase));
		
	/* Parse requester specific tags */
	pta.pta_IntReq	= intreq;
	pta.pta_Req	= reqnode->rn_Req;
	pta.pta_Tags	= tagList;

	CallHookPkt(&(reqinfo->ParseTagsHook), &pta, ASLB(AslBase));
    }

    /* Now do the layout stuff */
		
		
    /* Do some allocations common for all requesters */
    ld = AllocCommon(reqinfo->UserDataSize, intreq, requester, ASLB(AslBase));
    if (ld)
    {
	D(bug("Common stuff allocated\n"));
		
	/* Tell the specific requester to initialize */
		
	ld->ld_Command = LDCMD_INIT;
		
	if (CallHookPkt( &(reqinfo->GadgetryHook), ld, ASLB(AslBase)))
	{
	    struct NewWindow nw;
    	    struct TagItem   wintags[] =
	    {
	        {WA_CustomScreen	, (IPTR)ld->ld_Screen	}, /* stegerg: requesters should not use WA_PubScreen */
		{WA_InnerWidth		, 0			},
		{WA_InnerHeight		, 0			},
		{WA_AutoAdjust		, TRUE			},
		{WA_NewLookMenus    	, TRUE	    	    	},
		{TAG_DONE					}
	    };
    	    ULONG   	    idcmp;
	    BOOL    	    privateidcmp;
	    	
	    memset(&nw, 0L, sizeof (struct NewWindow));
			
	    nw.Width	= (intreq->ir_Width > ld->ld_MinWidth) ? intreq->ir_Width : ld->ld_MinWidth;
	    nw.Height	= (intreq->ir_Height > ld->ld_MinHeight) ? intreq->ir_Height : ld->ld_MinHeight;

	    if (intreq->ir_LeftEdge >= 0)
	    {
	        nw.LeftEdge = intreq->ir_LeftEdge;
	    } else {
	        nw.LeftEdge = (ld->ld_Screen->Width - (nw.Width + ld->ld_WBorLeft + ld->ld_WBorRight)) / 2;
	    }
	    if (intreq->ir_TopEdge >= 0)
	    {
	        nw.TopEdge = intreq->ir_TopEdge;
	    } else {
	        nw.TopEdge = (ld->ld_Screen->Height - (nw.Height + ld->ld_WBorTop + ld->ld_WBorBottom)) / 2;
	    }

	    D(bug("MinWidth: %d, MinHeight: %d\n", ld->ld_MinWidth, ld->ld_MinHeight));
			
	    nw.Title = intreq->ir_TitleText;
	    if (!nw.Title)
	    {
	    	nw.Title = GetString(intreq->ir_TitleID, intreq->ir_Catalog, ASLB(AslBase));
	    }
	    
	    D(bug("\tWindow title: %s", nw.Title));
	    
/*	    nw.FirstGadget	= ld->ld_GList; 
*/
	    nw.Flags	=  WFLG_DRAGBAR    | WFLG_DEPTHGADGET   | WFLG_CLOSEGADGET    |
			   WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM   | WFLG_SIMPLE_REFRESH |
			   WFLG_NOCAREREFRESH;
	    
	    if (!(intreq->ir_Flags & IF_OPENINACTIVE))
	    {
	    	nw.Flags |= WFLG_ACTIVATE;
	    }
	    
	    idcmp = IDCMP_CLOSEWINDOW | IDCMP_GADGETUP      | IDCMP_MOUSEMOVE  |
		    IDCMP_NEWSIZE     | IDCMP_REFRESHWINDOW | IDCMP_GADGETDOWN |
		    IDCMP_MENUPICK    | IDCMP_RAWKEY        | IDCMP_VANILLAKEY |
		    IDCMP_MOUSEBUTTONS;
			
	    wintags[1].ti_Data	= nw.Width;
	    wintags[2].ti_Data	= nw.Height;
		
	    privateidcmp = ((intreq->ir_Flags & IF_PRIVATEIDCMP) || (!intreq->ir_Window));	    
	    if (privateidcmp)
	    {
	    	nw.IDCMPFlags = idcmp;
	    }
	    
	    win = OpenWindowTagList(&nw, wintags);
	    if (win)
	    {
	    	if (!privateidcmp)
		{
		    win->UserPort = intreq->ir_Window->UserPort;
		    ModifyIDCMP(win, idcmp);
		}
		
	    	ld->ld_Window = win;
	    	
		ObtainSemaphore( &(ASLB(AslBase)->ReqListSem) );
		reqnode->rn_ReqWindow = win;
		ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));

	    	D(bug("Window opened\n"));
	    	
	    	D(bug
	    	(
	    	    "\tLeft: %d\n\tTop: %d\n\tWidth: %d\n\tHeight: %d\n",
	    	    win->LeftEdge,
	    	    win->TopEdge,
	    	    win->Width,
	    	    win->Height
	    	 ));
	    	

		/* Constraint the window minsize */
		
		WindowLimits
		(
		    win,
		    ld->ld_MinWidth + win->BorderLeft + win->BorderRight,
		    ld->ld_MinHeight + win->BorderTop + win->BorderBottom,
		    ~0,
		    ~0
		);
		
		D(bug("Window limits set\n"));
				
		SetFont(win->RPort, ld->ld_Font);
			
		/* Layout the requester */
		ld->ld_Command = LDCMD_LAYOUT;
		if (CallHookPkt(&(reqinfo->GadgetryHook), ld, ASLB(AslBase)))
		{
		    D(bug("Gadgetry layout done\n"));
		    
		    if (ld->ld_GList)
		    {
		    	D(bug("Adding glist\n"));
			AddGList(win, ld->ld_GList, -1, -1, NULL);
		    	D(bug("Refreshing glist\n"));			
			RefreshGList(ld->ld_GList, win, NULL, -1);
			
			D(bug("Gadgetlist refreshed\n"));
		    }
		    
		    if (ld->ld_Menu) SetMenuStrip(win, ld->ld_Menu);
	
		    ld->ld_Command = LDCMD_WINDOWOPENED;
		    CallHookPkt(&(reqinfo->GadgetryHook), ld, ASLB(AslBase));
		    		
		    if (intreq->ir_Flags & IF_POPTOFRONT)
		    {
		    	struct Screen 	*frontscr;
		    	ULONG 	    	 ilock;
			
			ilock = LockIBase(0);
			frontscr = IntuitionBase->FirstScreen;
			UnlockIBase(ilock);
			
			if (frontscr != win->WScreen)
			{
		    	    ScreenToFront(win->WScreen);
			    intreq->ir_Flags |= IF_POPPEDTOFRONT;
			}
		    }
		    
		    /* Wait for the user to do something */	
		    success = HandleEvents(ld, reqinfo, ASLB(AslBase));
		
		    /* Finished with event handling, clean up requester 
		    specific stuff */
		
		    ld->ld_Command = LDCMD_CLEANUP;
		    CallHookPkt(&(reqinfo->GadgetryHook), ld, ASLB(AslBase));
		    
		} /* if (CallHookPkt(&(reqinfo->GadgetryHook), ld, ASLB(AslBase))) */
		
		/* win is closed in FreeCommon */
		
	    } /* if (win) */
	    
	} /* if (CallHookPkt( &(reqinfo->GadgetryHook), ld, ASLB(AslBase))) */
		
	ObtainSemaphore( &(ASLB(AslBase)->ReqListSem) );
	reqnode->rn_ReqWindow = NULL;
	ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));

	FreeCommon(ld, ASLB(AslBase));

    } /* if (ld) */

    ReturnBool ("AslRequest", success);

    AROS_LIBFUNC_EXIT
} /* AslRequest */
