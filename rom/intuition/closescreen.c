/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Close a screen opened via OpenScreen and the like
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/exec.h>
#include <proto/graphics.h>

#ifndef DEBUG_CloseScreen
#   define DEBUG_CloseScreen 0
#endif
#undef DEBUG
#if DEBUG_CloseScreen
#   define DEBUG 1
#endif
#	include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

	AROS_LH1(BOOL, CloseScreen,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 11, Intuition)

/*  FUNCTION

    Release all resources held by a screen and close it down visually.

    INPUTS

    screen  --  pointer to the screen to be closed

    RESULT

    TRUE if the screen is successfully closed, FALSE if there were still
    windows left on the screen (which means the screen is not closed).

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
    struct Screen * parent;

    D(bug("CloseScreen (%p)\n", screen));


    /* If this is a public screen, free related information if there are
       no windows left on the screen */
    if(GetPrivScreen(screen)->pubScrNode != NULL)
    {
	LockPubScreenList();

	if(GetPrivScreen(screen)->pubScrNode->psn_VisitorCount != 0)
	{
	    UnlockPubScreenList();
	    return FALSE;
	}

	Remove((struct Node *)GetPrivScreen(screen)->pubScrNode);

	if(GetPrivScreen(screen)->pubScrNode->psn_Node.ln_Name != NULL)
	    FreeVec(GetPrivScreen(screen)->pubScrNode->psn_Node.ln_Name);
	
	FreeMem(GetPrivScreen(screen)->pubScrNode,
		sizeof(struct PubScreenNode));

	UnlockPubScreenList();
    }
    

    /* Trick: Since NextScreen is the first field of the structure,
	we can use the pointer in the IntuitionBase as a screen with
	the structure-size of one pointer */
    parent = (struct Screen *)&(IntuitionBase->FirstScreen);

    /* For all screens... */
    while (parent->NextScreen)
    {
	/* If the screen to close is the next screen... */
	if (parent->NextScreen == screen)
	{
	    /* Unlink it */
	    parent->NextScreen = screen->NextScreen;

	    /* Check ActiveScreen */
	    if (IntuitionBase->ActiveScreen == screen)
	    {
		if (screen->NextScreen)
		    IntuitionBase->ActiveScreen = screen->NextScreen;
		else if (IntuitionBase->FirstScreen)
		    IntuitionBase->ActiveScreen = parent;
		else
		    IntuitionBase->ActiveScreen = NULL;
	    }
	    
	    /* kill screen bar */	    
	    KillScreenBar(screen, IntuitionBase);
	    
	    /* kill depth gadget */
	    if (((struct IntScreen *)screen)->depthgadget)
	    	DisposeObject(((struct IntScreen *)screen)->depthgadget);
		
	    /* Free the RasInfo of the viewport */
	    FreeMem(screen->ViewPort.RasInfo, sizeof (struct RasInfo));
	    
	    /* Free the screen's bitmap */
	    FreeBitMap(screen->RastPort.BitMap);

	    /* Free the RastPort's contents */
	    DeinitRastPort(&screen->RastPort);

	    DisposeObject(((struct IntScreen *)screen)->DInfo.dri_CheckMark);
	    DisposeObject(((struct IntScreen *)screen)->DInfo.dri_AmigaKey);
	    
	    /* Close the font */
	    CloseFont(((struct IntScreen *)screen)->DInfo.dri_Font);
	    
	    /* Free the memory */
	    FreeMem(screen, sizeof (struct IntScreen));

	    ReturnBool("CloseScreen",TRUE);
	}
    }

    ReturnBool("CloseScreen",FALSE);
    AROS_LIBFUNC_EXIT
} /* CloseScreen */
