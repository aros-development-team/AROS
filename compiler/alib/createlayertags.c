/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a layer
    Lang: english
*/

#define AROS_TAGRETURNTYPE  struct Layer *
#include <graphics/clip.h>
#include <exec/libraries.h>
#include "alib_intern.h"

extern struct Library * LayersBase;

/*****************************************************************************

    NAME */
#include <graphics/layers.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/layers.h>

	struct Layer * CreateLayerTags (

/*  SYNOPSIS */
	struct Layer_Info * li,
	struct BitMap     * bm,
	LONG                x0,
	LONG                y0,
	LONG                x1,
	LONG                y1,
	LONG                flags,
	Tag	            tag1,
	...)

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
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = CreateLayerTagList(li,bm,x0,y0,x1,y1,flags,AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* CreateLayerTags */
