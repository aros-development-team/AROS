/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH3(WORD, WeighTAMatch,

/*  SYNOPSIS */
	AROS_LHA(struct TextAttr *, reqTextAttr, A0),
	AROS_LHA(struct TextAttr *, targetTextAttr, A1),
	AROS_LHA(struct TagItem  *, targetTags, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 134, Graphics)

/*  FUNCTION
		Determines how well two font descriptions match.

    INPUTS
    	reqTextAttr		- the required textattr.
    	targetTextAttr	- textattr of potential match.
    	targetTags		- tags for the targetTextAttr.

    RESULT
    	A weight number which measures how well the TextAttrs
    	match. The weight may vary from 0 (no match) to
    	MAXFONTMATCHWEIGHT (perfect match).

    NOTES

    EXAMPLE

    BUGS
    	Does not yet take tags into account.

    SEE ALSO

    INTERNALS
    	

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    WORD matchweight = 0;

    WORD sizematch = 0; /* for temporary keeping data */
	
    /* Use words because of abs() */
    WORD reqsize, targetsize;
    UWORD sizediff;
	
    UBYTE comparedflags, comparedstyle;
	

    /* Compare font flags */
	
    /* We get bits set to 1 where flags are equal (both set or both cleared */
    comparedflags = ~(reqTextAttr->ta_Flags ^ targetTextAttr->ta_Flags);

    matchweight  = (comparedflags & FPF_PROPORTIONAL	) ? 1 : 0;
    matchweight |= (comparedflags & FPF_TALLDOT		) ? 1 << 1 : 0;
    matchweight |= (comparedflags & FPF_WIDEDOT		) ? 1 << 2 : 0;
	
    /* Compare font style */
    comparedstyle = ~(reqTextAttr->ta_Style ^ targetTextAttr->ta_Style);

    matchweight |= (comparedstyle & FSF_EXTENDED	) ? 1 << 3 : 0;
    matchweight |= (comparedstyle & FSF_BOLD		) ? 1 << 4 : 0;
    matchweight |= (comparedstyle & FSF_UNDERLINED	) ? 1 << 5 : 0;
    matchweight |= (comparedstyle & FSF_ITALIC		) ? 1 << 6 : 0;
    matchweight |= (comparedstyle & FSF_COLORFONT	) ? 1 << 7 : 0;
	
    /* We now have 7 bits for determinig size match */
	
    reqsize = reqTextAttr->ta_YSize;
    targetsize = targetTextAttr->ta_YSize;
	
    sizediff = abs(reqsize - targetsize);
	
    if (sizediff > 127)
	sizematch = 0;
    else
	sizematch = 127 - sizediff;
	
    /* Size is most significant in the matching */ 
    matchweight |= sizematch << 8;
	
    return (matchweight);	  
  
    AROS_LIBFUNC_EXIT
} /* WeighTAMatch */
