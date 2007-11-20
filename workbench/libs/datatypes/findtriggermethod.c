/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "datatypes_intern.h"
#include <proto/utility.h>

/*****************************************************************************

    NAME */

        AROS_LH3(struct DTMethod *, FindTriggerMethod,

/*  SYNOPSIS */
	AROS_LHA(struct DTMethod *, methods, A0),
	AROS_LHA(STRPTR           , command, A1),
	AROS_LHA(ULONG            , method , D0),

/*  LOCATION */
	struct Library *, DataTypesBase, 44, DataTypes)

/*  FUNCTION

    Search for a specific trigger method in a array of trigger methods (check
    if either 'command' or 'method' matches).

    INPUTS

    methods  --  array of trigger methods; may be NULL
    command  --  name of trigger method (may be NULL; if so, 'command'
                        is not matched against)
    method   --  id of trigger method to search for (may be ~0; if so, don't
                 match against 'method'.

    RESULT

    Pointer to trigger method table entry (struct DTMethod *) or NULL if the
    method wasn't found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    GetDTTriggerMethods(), CopyDTTriggerMethods()

    INTERNALS

    HISTORY

    2.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DTMethod *retval = NULL;
    
    if (methods)
    {
	while(methods->dtm_Method != STM_DONE)
	{
	    if(command != NULL)
	    {
		if(Stricmp(methods->dtm_Command, command) == 0)
		{
		    retval = methods;
		    break;
		}
	    }

	    if(method != ~0)
	    {
		if(methods->dtm_Method == method)
		{
		    retval = methods;
		    break;
		}
	    }
	    
	    methods++;
	}
    }
    
    return retval;

    AROS_LIBFUNC_EXIT
    
} /* FindTriggerMethod */

