/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get a pointer to a method for an object
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include <oop/oop.h>
#include <aros/debug.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH3(OOP_MethodFunc, OOP_GetMethod,

/*  SYNOPSIS */
	AROS_LHA(OOP_Object  *, obj, 	A0),
	AROS_LHA(OOP_MethodID,  mid,	D0),
	AROS_LHA(OOP_Class **, classPtr, A1),

/*  LOCATION */
	struct Library *, OOPBase, 21, OOP)

/*  FUNCTION
	Get a specific method function for a specific object and 
	a specific interface. This a direct pointer to the method
	implementation. The pointer should ONLY be used on the object you
	acquired.

    INPUTS
    	obj	 - pointer to object to get method for.
	mid	 - method id for method to get. This may be obtained with GetMethodID()
	classPtr - A pointer to a location where implementation class pointer will be stored.
		   The obtained method must be called with this class pointer. This pointer
		   is mandatory!

    RESULT
    	The method asked for, or NULL if the method does not exist in
	the object's class.

    NOTES
	!!! Use with EXTREME CAUTION. Very few programs need the extra speed gained
	    by calling a method directly 
	!!!

    EXAMPLE

    BUGS

    SEE ALSO
    	OOP_GetMethodID()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct IFMethod *ifm;
    
    /* First get mid */
    /* Get the method from the object's class */
    ifm = meta_findmethod((OOP_Object *)OOP_OCLASS(obj), mid, (struct Library *)OOPBase);
    if (NULL == ifm)
	return NULL;

    /* Set class pointer */
    *classPtr = ifm->mClass;

    /* Paranoia */
    D(if (NULL == ifm->MethodFunc) bug("!!! OOP/GetMethod(): IFMethod instance had no methodfunc. This should NEVER happen !!!\n");)

    return ifm->MethodFunc;

    AROS_LIBFUNC_EXIT
} /* OOP_GetMethod */
