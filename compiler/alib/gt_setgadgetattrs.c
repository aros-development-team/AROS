/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set attributes of a gadtools gadget
    Lang: english
*/

#define AROS_TAGRETURNTYPE void
#define AROS_TAGRETURNTYPEVOID

#include "alib_intern.h"

extern struct Library * GadToolsBase;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/gadtools.h>

	void GT_SetGadgetAttrs (

/*  SYNOPSIS */
	struct Gadget    * gad,
        struct Window    * win,
        struct Requester * req,
	ULONG		   tag1,
	...		   )

/*  FUNCTION
        Varargs version of gadtools.library/GT_SetGadgetAttrsA().

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
    GT_SetGadgetAttrsA (gad, win, req, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* GT_SetGadgetAttrs */
