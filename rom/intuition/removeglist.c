/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include "inputhandler.h"
#include <intuition/gadgetclass.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH3(UWORD, RemoveGList,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, remPtr, A0),
	AROS_LHA(struct Gadget *, gadget, A1),
	AROS_LHA(LONG           , numGad, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 74, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    
    struct Gadget   *pred;
    struct Gadget   *last;
    struct IIHData  *iihdata;
    LONG    	    numGad2;
    UWORD   	    count;

    if (!numGad) return ~0;
    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
    
    iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    pred    = (struct Gadget *)&remPtr->FirstGadget;
    count   = 0;
    numGad2 = numGad;
    
    while (pred->NextGadget && pred->NextGadget != gadget)
    {
	pred = pred->NextGadget;
	count ++;
    }

    if (pred->NextGadget)
    {
	/* Check if one of the gadgets to be removed is the active gadget.
	   If it is, then make it inactive! */

	for (last = gadget; last && numGad2--; last = last->NextGadget)
	{
    	    if ((iihdata->ActiveGadget == last) || (iihdata->ActiveSysGadget == last))
	    {
		#warning: According to autodocs should also wait for the (left?) mouse button to be released
		switch(last->GadgetType & GTYP_GTYPEMASK)
		{
	    	    case GTYP_CUSTOMGADGET:
		    {		    
			struct gpGoInactive gpgi;

			gpgi.MethodID = GM_GOINACTIVE;
			gpgi.gpgi_GInfo = NULL;
			gpgi.gpgi_Abort = 1; 

			DoGadgetMethodA(last, remPtr, NULL, (Msg)&gpgi);

			if (SYSGADGET_ACTIVE)
			{
			    if (IS_BOOPSI_GADGET(iihdata->ActiveSysGadget))
			    {
			    	DoGadgetMethodA(iihdata->ActiveSysGadget, remPtr, NULL, (Msg)&gpgi);
			    }
			    iihdata->ActiveSysGadget = NULL;
			}

			break;
		    }
		}

		last->Activation &= ~GACT_ACTIVEGADGET;
		iihdata->ActiveGadget = NULL;
	    }

	} /* for (last = gadget; last && numGad2--; last = last->NextGadget) */

	for (last = gadget; last->NextGadget && --numGad; last = last->NextGadget) ; 

	pred->NextGadget = last->NextGadget;

	/* stegerg: don't do this. DOpus for example relies on gadget->NextGadget
                    not being touched */

    #if 0
	last->NextGadget = NULL;
    #endif
	

    } /* if (pred->NextGadget) */
    else
    {
    	count = ~0;
    }
    
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);

    return count;
    
    AROS_LIBFUNC_EXIT
    
} /* RemoveGList */
