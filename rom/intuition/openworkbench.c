/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function OpenWorkBench()
    Lang: english
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */

    AROS_LH0(IPTR, OpenWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
    struct IntuitionBase *, IntuitionBase, 35, Intuition)

/*  FUNCTION
	Attempt to open the Workbench screen.

    INPUTS
	None.

    RESULT
	Tries to (re)open WorkBench screen. If successful return value
	is a pointer to the screen structure, which shouldn't be used,
	because other programs may close the WorkBench and make the
	pointer invalid.
	If this function fails the return value is NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *wbscreen;
    
    LockPubScreenList();
    
    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;
    if (wbscreen)
    {
    	UnlockPubScreenList();
	
	return (IPTR)wbscreen;
    }
    else
    {
    	/* Open the Workbench screen if we don't have one. */
    	UWORD pens[] = { ~0 };
        struct TagItem screenTags[] =
	{
            { SA_Width      , AROS_DEFAULT_WBWIDTH  	},
            { SA_Height     , AROS_DEFAULT_WBHEIGHT 	},
            { SA_Depth      , AROS_DEFAULT_WBDEPTH  	},
            { SA_Type       , WBENCHSCREEN          	},
            { SA_Title      , (IPTR)"Workbench Screen"  },
            { SA_PubName    , (IPTR)"Workbench"     	},
	    { SA_Pens	    , (IPTR)pens    	    	},
            { SA_SharePens  , TRUE                  	},
	    { SA_SysFont    , 1     	    	    	},
            { TAG_END       , 0                     	}
        };
    	struct TagItem modetags[] =
	{
	    { BIDTAG_DesiredWidth   , AROS_DEFAULT_WBWIDTH  },
	    { BIDTAG_DesiredHeight  , AROS_DEFAULT_WBHEIGHT },
	    { BIDTAG_Depth   	    , AROS_DEFAULT_WBDEPTH  },
	    { TAG_DONE	    	    	    	    	    }
	};	
    	ULONG modeid;
	
	modeid = BestModeIDA(modetags);
    	if (modeid != INVALID_ID)
	{
	    APTR  disphandle;
	    
	    if ((disphandle = FindDisplayInfo(modeid)))
	    {
	    	struct DimensionInfo dim;
	    	
		if (GetDisplayInfoData(disphandle, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, modeid))
		{
		    screenTags[0].ti_Data = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
		    screenTags[1].ti_Data = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
		    if (dim.MaxDepth > AROS_DEFAULT_WBDEPTH)
		    {
		    	screenTags[2].ti_Data = dim.MaxDepth;
		    }
		}
	    }
	}
	
        wbscreen = OpenScreenTagList( NULL, screenTags );

        if( !wbscreen )
	{
	    UnlockPubScreenList();
	    
	    return 0;
	    
	}
        
	GetPrivIBase(IntuitionBase)->WorkBench = wbscreen;

        /* Make the screen public. */
        PubScreenStatus( wbscreen, 0 );

    }

    /* We have opened the Workbench Screen. Now  tell the Workbench process
       to open it's windows, if there is one. We still do have the pub screen
       list locked. But while sending the Message to the Workbench task we
       must unlock the semaphore, otherwise there can be deadlocks if the
       Workbench task itself does something which locks the pub screen list.
       
       But if we unlock the pub screen list, then some other task could try
       to close the Workbench screen in the meantime. The trick to solve
       this problem is to increase the psn_VisitorCount of the Workbench
       screen here, before unlocking the pub screen list. This way the
       Workbench screen cannot go away. */
    
    GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount++;
    
    UnlockPubScreenList();
    
    /* Don't call this function while pub screen list is locked! */
    TellWBTaskToOpenWindows(IntuitionBase);
    
    /* Now fix the psn_VisitorCount we have increased by one, above. It's probably
       better to do this by hand, instead of calling UnlockPubScreen, because Un-
       lockPubScreen can send signal to psn_SigTask. */
       
    LockPubScreenList();
    GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount--;
    UnlockPubScreenList();
    
    return (IPTR)wbscreen;

    AROS_LIBFUNC_EXIT
    
} /* OpenWorkBench */
