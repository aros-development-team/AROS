/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain the best pen available for a given color
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <graphics/view.h>

/*****************************************************************************

    NAME */
	#include <proto/graphics.h>

	AROS_LH5(LONG, ObtainBestPenA,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm  , A0),
	AROS_LHA(ULONG            , r   , D1),
	AROS_LHA(ULONG            , g   , D2),
	AROS_LHA(ULONG            , b   , D3),
	AROS_LHA(struct TagItem * , tags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 140, Graphics)

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct PaletteExtra     	*pe = cm->PalExtra;
    LONG    	    	    	retval = -1;
    PalExtra_AllocList_Type   	index;

    if (NULL != pe)
    {
	struct TagItem  defaults[] =
	{ 
	    {OBP_Precision, PRECISION_IMAGE },
	    {OBP_FailIfBad, FALSE   	    }
	};
	ULONG 	    	best_distance = (ULONG)-1;


	/* 
	** override the defaults if necessary 
	*/
	defaults[0].ti_Data = GetTagData(OBP_Precision, 
                                	 defaults[0].ti_Data,
                                	 tags);

	defaults[1].ti_Data = GetTagData(OBP_FailIfBad, 
                                	 defaults[1].ti_Data,
                                	 tags);

	/* 
	** let nobody else play with the PalExtra structure 
	*/
	ObtainSemaphore(&pe->pe_Semaphore);

	/* 
	** Walk through the list of shared pens and search
	** for the closest color.
	*/
	index = (PalExtra_AllocList_Type)pe->pe_FirstShared;
	while ((PalExtra_AllocList_Type)-1 != index)
	{
	    ULONG distance = color_distance(cm,r,g,b,index);
	    
	    if (distance < best_distance)
	    {
        	best_distance = distance;
        	retval        = index;
	    }
	    
	    index = PALEXTRA_ALLOCLIST(pe, index);
	}

    	#warning The color distance calc might be different than in AmigaOS. 

	/* 
	** If the best distance to an available color is greater than
	** the square of the tolerance, try to allocate a better
	** color, otherwise increase the shared counter for that color.
	** If only a little amount of colors is free for allocation in the
	** colormap the restrictions towards color matching should be
	** much looser than if the amount of free colors is close to 0.
	** The autodocs say that.
	*/
	if (
	    (retval == -1) ||
            (PRECISION_EXACT == defaults[0].ti_Data && 0 != best_distance ) ||
            (best_distance * pe->pe_NFree  > 
            (defaults[0].ti_Data * defaults[0].ti_Data) * pe->pe_SharableColors
            )
	   )
	{
	    /*
	    ** The given tolerance could not be accomplished.
	    ** Try to allocate a pen. If that fails we
	    ** return -1 if the user specified OBP_FailIfBad = TRUE.
	    */
	    LONG tmp = ObtainPen(cm,-1,r,g,b,0);

	    if (-1 == tmp)
	    {
        	/* 
        	** Return -1 if the user is strict with color matching.
        	** In the other case retval is not changed.
        	*/
        	if (TRUE == defaults[1].ti_Data)
		{
        	    retval = -1;
		}
		else if (retval != -1)
		{
		    /*
		    ** One more application is using this color
		    */
		    
		    PALEXTRA_REFCNT(pe, retval)++;
		}
	    }
	    else
                retval = tmp;
	}
	else
	{
	    /*
	    ** One more application is using this color
	    */
	    PALEXTRA_REFCNT(pe, retval)++;
	}

	ReleaseSemaphore(&pe->pe_Semaphore);
      
    } /* if (NULL != pe) */
    
    return retval;

    AROS_LIBFUNC_EXIT
  
} /* ObtainBestPenA */
