/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"

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
    struct Gadget * pred;
    struct Gadget * last;
    UWORD count;

    pred = (struct Gadget *)&remPtr->FirstGadget;
    count = 0;

    while (pred->NextGadget && pred->NextGadget != gadget)
    {
	pred = pred->NextGadget;
	count ++;
    }

    if (!pred->NextGadget)
	return ~0;

    for (last=gadget; last->NextGadget && --numGad; last=last->NextGadget);

    pred->NextGadget = last->NextGadget;

    /* stegerg: don't do this. DOpus for example relies on gadget->NextGadget
                not being touched 

    last->NextGadget = NULL;
    
    */

    return count;
    AROS_LIBFUNC_EXIT
} /* RemoveGList */
