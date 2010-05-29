/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get attributes of a gadtools gadget
    Lang: english
*/

#include <exec/types.h>
#define AROS_TAGRETURNTYPE LONG

#include "alib_intern.h"

extern struct Library * GadToolsBase;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/gadtools.h>

	LONG GT_GetGadgetAttrs (

/*  SYNOPSIS */
	struct Gadget    * gad,
	struct Window    * win,
	struct Requester * req,
	Tag		   tag1,
	...		   )

/*  FUNCTION
        Varargs version of gadtools.library/GT_GetGadgetAttrsA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	gadtools.library/GT_GetGadgetAttrsA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    GT_GetGadgetAttrsA (gad, win, req, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* GT_GetGadgetAttrs */
