/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH3(WORD, WeighTAMatch,

/*  SYNOPSIS */
	AROS_LHA(const struct TextAttr *, reqTextAttr, A0),
	AROS_LHA(const struct TextAttr *, targetTextAttr, A1),
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

    WORD matchweight = MAXFONTMATCHWEIGHT;
    WORD sizematch = 0; /* for temporary keeping data */
    UWORD sizediff;
	
    /* Compare font flags */

    /* No match if req is designed and target not */
    if ((reqTextAttr->ta_Flags & FPF_DESIGNED) && ! (targetTextAttr->ta_Flags & (FPF_DESIGNED | FPF_DISKFONT)))
	return 0;
    
    /* No match if REVPATH is not the same, ignore other flags */
    if ((reqTextAttr->ta_Flags ^ targetTextAttr->ta_Flags) & FPF_REVPATH)
	return 0;

    
    /* Compare font style */
    if ((reqTextAttr->ta_Style & FSF_UNDERLINED) && !(targetTextAttr->ta_Style & FSF_UNDERLINED))
	matchweight &= ~(1<<2);
    if (!(reqTextAttr->ta_Style & FSF_UNDERLINED) && (targetTextAttr->ta_Style & FSF_UNDERLINED))
	matchweight &= ~(1<<11);
    
    if ((reqTextAttr->ta_Style & FSF_BOLD) && !(targetTextAttr->ta_Style & FSF_BOLD))
	matchweight &= ~(1<<3);
    if (!(reqTextAttr->ta_Style & FSF_BOLD) && (targetTextAttr->ta_Style & FSF_BOLD))
	matchweight &= ~(1<<9);
    
    if ((reqTextAttr->ta_Style & FSF_ITALIC) && !(targetTextAttr->ta_Style & FSF_ITALIC))
	matchweight &= ~(1<<4);
    if (!(reqTextAttr->ta_Style & FSF_ITALIC) && (targetTextAttr->ta_Style & FSF_ITALIC))
	matchweight &= ~(1<<10);

    /* Now subtract a value depending on the size difference */
    
    sizediff = abs((WORD)reqTextAttr->ta_YSize - (WORD)targetTextAttr->ta_YSize);
	
    if (sizediff > 511)
	return 0;

    if (reqTextAttr->ta_YSize < targetTextAttr->ta_YSize)
	sizematch = sizediff << 7;
    else
	sizematch = sizediff << 5;
    
    if (sizematch > matchweight)
	matchweight = 0;
    else
	matchweight -= sizematch;
	
    return matchweight;

    AROS_LIBFUNC_EXIT
} /* WeighTAMatch */
