/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

AROS_LH3(APTR, AllocRemember,

         /*  SYNOPSIS */
         AROS_LHA(struct Remember **, rememberKey, A0),
         AROS_LHA(ULONG             , size, D0),
         AROS_LHA(ULONG             , flags, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 66, Intuition)

/*  FUNCTION
    Allocate some memory and remeber it in the Remember-List.
 
    INPUTS
    rememberKey - Store information in this list
    size - How many bytes to allocate
    flags - Attributes (see AllocMem())
 
    RESULT
    Pointer to the allocated memory or NULL.
 
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

    struct Remember *newKey;
    APTR    	     ptr = NULL;

    DEBUG_REMEMBER(dprintf("AllocRemember: Key 0x%lx Size 0x%lx Flags 0x%08lx\n",
                           rememberKey, size, flags));

    newKey = AllocMem (sizeof (struct Remember), MEMF_ANY);

    if (newKey)
    {
        ptr = AllocMem (size, flags);

        if (ptr)
        {
            newKey->NextRemember = *rememberKey;
            newKey->Memory  	 = ptr;
            newKey->RememberSize = size;
	    
            *rememberKey = newKey;
        }
        else
        {
            FreeMem (newKey, sizeof (struct Remember));
        }
    }

    DEBUG_REMEMBER(dprintf("AllocRemember: Ptr 0x%lx\n", ptr));

    return ptr;

    AROS_LIBFUNC_EXIT
} /* AllocRemember */
