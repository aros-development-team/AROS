/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

	AROS_LH2(VOID *, OOP_GetMethod,

/*  SYNOPSIS */
	AROS_LHA(OOP_Object  *, obj, 	A0),
	AROS_LHA(OOP_MethodID,  mid,	D0),

/*  LOCATION */
	struct Library *, OOPBase, 21, OOP)

/*  FUNCTION
	Get a specific method function for a specific object and 
	a specific interface. This a direct pointer to the method implementation.
	The pointer should ONLY be used on the object you aquired.

    INPUTS
    	obj	- pointer to object to get method for.
	mid	- method id for method to get. This may be obtained with GetMethodID()

    RESULT
    	The method asked for, or NULL if the method does not exist in
	the object's class.

    NOTES
	!!! Use with EXTREME CAUTION. Very few programs need the extra speed gained
	    by calling a method directly 
	!!!

    EXAMPLE

    BUGS
	It returns VOID *. I got compiler errors when returning
	IPTR (*)(Class *, Object *, Msg)

    SEE ALSO
    	OOP_GetMethodID()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct IFMethod *ifm;
    
    /* First get mid */
    /* Get the method from the object's class */
    ifm = meta_findmethod((OOP_Object *)OOP_OCLASS(obj), mid, (struct Library *)OOPBase);
    if (NULL == ifm)
	return NULL;

    /* Paranoia */    
    if (NULL == ifm->MethodFunc) {
	D(bug("!!! OOP/GetMethod(): IFMethod instance had no methodfunc. This should NEVER happen !!!\n"));
	return NULL;
    }
    
    return (VOID *)ifm->MethodFunc;

    AROS_LIBFUNC_EXIT
} /* OOP_GetMethod */
