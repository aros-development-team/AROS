/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Late initialization of intuition.
    Lang: english
*/

#include <proto/intuition.h>


/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(BOOL , LateIntuiInit,

/*  SYNOPSIS */
	AROS_LHA(APTR, data, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 120, Intuition)

/*  FUNCTION
	This function permits late initalization
	of intuition (After dos and after graphics hidds are setup,
	but before starup-sequence is run.
	Can be used to open workbench screen.

    INPUTS
	data - unused for now.

    RESULT
    	success - TRUE if initialization went, FALSE otherwise.

    NOTES
	This function is private and AROS specific.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    

    struct TagItem screenTags[] =
    {
	{ SA_Depth, 	4			},
	{ SA_Type,  	WBENCHSCREEN		},
	{ SA_Title, 	(IPTR)"Workbench"   	},
	{ SA_Width, 	800			},
	{ SA_Height,	600			},	
	{ SA_PubName,   (IPTR)"Workbench"       },
	{ TAG_END, 0 }
    };
    
    if (!GetPrivIBase(IntuitionBase)->WorkBench)
    {
	struct Screen *screen;

	screen = OpenScreenTagList (NULL, screenTags);

	if(screen)
	{
	    /* Make the Workbench screen public... */
	    PubScreenStatus(screen, 0);

	    /* ...and make it the default */
	    SetDefaultPubScreen(NULL);

	    IntuitionBase->FirstScreen =   
	    	IntuitionBase->ActiveScreen =   
		GetPrivIBase(IntuitionBase)->WorkBench = screen;
			
	    return TRUE;
	}
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* LateIntuiInit */
