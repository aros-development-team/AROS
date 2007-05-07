/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new BOOPSI object
    Lang: english
*/
#include <exec/lists.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <clib/intuition_protos.h>

#include "intern.h"

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/boopsi.h>

	AROS_LH3(APTR, NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(struct IClass  *, classPtr, A0),
	AROS_LHA(UBYTE          *, classID, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, BOOPSIBase, 11, BOOPSI)

/*  FUNCTION
	Use this function to create BOOPSI objects (BOOPSI stands for
	"Basic Object Oriented Programming System for Intuition).

	You may specify a class either by it's name (if it's a public class)
	or by a pointer to its definition (if it's a private class). If
	classPtr is NULL, classID is used.

    INPUTS
	classPtr - Pointer to a private class (or a public class if you
		happen to have a pointer to it)
	classID - Name of a public class
	tagList - Initial attributes. Read the documentation of the class
		carefully to find out which attributes must be specified
		here and which can.

    RESULT
	A BOOPSI object which can be manipulated with general functions and
	which must be disposed with DisposeObject() later.

    NOTES
	This functions send OM_NEW to the dispatcher of the class.

    EXAMPLE

    BUGS

    SEE ALSO
	DisposeObject(), SetAttrs(), GetAttr(), MakeClass(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    Object * object;

    EnterFunc(bug("intuition::NewObjectA()\n"));

    /* No classPtr ? */
    if (!classPtr)
	classPtr = FindClass (classID);

    if (!classPtr)
	return (NULL); /* Nothing found */

    D(bug("classPtr: %p\n", classPtr));

    /* Try to create a new object */
    if ((object = (Object *) CoerceMethod (classPtr, (Object *)classPtr, OM_NEW,
	    tagList, NULL)))
	classPtr->cl_ObjectCount ++;

    ReturnPtr("intuition::NewObjectA()", Object *, object);
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
