/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "datatypes_intern.h"
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>

/*****************************************************************************

    NAME */

        AROS_LH2(ULONG *, FindMethod,

/*  SYNOPSIS */
	AROS_LHA(ULONG *, methods       , A0),
	AROS_LHA(ULONG  , searchmethodid, A1),

/*  LOCATION */
	struct Library *, DataTypesBase, 43, DataTypes)

/*  FUNCTION

    Search for a specific method in a array of methods.

    INPUTS

    methods         --  array of methods; may be NULL
    searchmethodid  --  method to search for

    RESULT

    Pointer to method table entry or NULL if the method wasn't found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    GetDTMethods(), CopyDTMethods()

    INTERNALS

    HISTORY

    2.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(methods == NULL)
	return NULL;

    while(((LONG)(*methods)) != -1)
    {
	if(*methods == searchmethodid)
	    return methods;

	methods++;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindMethod */
