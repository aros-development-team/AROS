/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Find a tooltype entry from an array of tool types.
*/
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(UBYTE *, FindToolType,

/*  SYNOPSIS */
	AROS_LHA(CONST STRPTR *, toolTypeArray, A0),
	AROS_LHA(CONST STRPTR,   typeName,      A1),

/*  LOCATION */
	struct Library *, IconBase, 16, Icon)

/*  FUNCTION
	Finds the supplied typeName inside the given toolTypeArray.

    INPUTS
	toolTypeArray  -  pointer to an array of tooltype strings.
	typeName      -  name of a specific tool-type.

    RESULT
	NULL if the tooltype wasn't found and a pointer to the value
	of the tooltype otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MatchToolValue()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    
    ULONG typenamelen = 0;
    
    /* Make sure we have sane input parameters */
    if (toolTypeArray == NULL || typeName == NULL) return NULL;
    
    typenamelen = strlen(typeName);

    while (*toolTypeArray)
    {
	/* case sensetive compare */
	if (!strncmp (*toolTypeArray, typeName, typenamelen) )
	{
            return (*toolTypeArray+typenamelen+1);
	}

	toolTypeArray ++;
    }

    return NULL;
    
    AROS_LIBFUNC_EXIT
} /* FindToolType */
