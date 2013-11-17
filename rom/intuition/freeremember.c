/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

        AROS_LH2(void, FreeRemember,

/*  SYNOPSIS */
        AROS_LHA(struct Remember **, rememberKey, A0),
        AROS_LHA(LONG              , reallyForget, D0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 68, Intuition)

/*  FUNCTION
	Free memory allocated by AllocRemember().

    INPUTS
	rememberKey  - address of a pointer to struct Remember
	reallyForget - TRUE  release all memory
	               FALSE release only link nodes

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocRemember()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Remember *next, *key;

    DEBUG_REMEMBER(dprintf("FreeRemember: Key 0x%lx ReallyForget %d\n",
                           rememberKey, reallyForget));

    for (next = *rememberKey; (key = next); )
    {
        next = key->NextRemember;

        if (reallyForget)
            FreeMem (key->Memory, key->RememberSize);

        FreeMem (key, sizeof (struct Remember));
    }

    *rememberKey = NULL;

    AROS_LIBFUNC_EXIT
} /* FreeRemember */
