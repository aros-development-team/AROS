/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/28 17:55:35  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <intuition/classes.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <intuition/classusr.h>
	#include <clib/intuition_protos.h>

	__AROS_LH3(APTR, NewObjectA,

/*  SYNOPSIS */
	__AROS_LHA(struct IClass  *, classPtr, A0),
	__AROS_LHA(UBYTE          *, classID, A1),
	__AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 106, Intuition)

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    Object * object;
    struct _Object carrier;

    /* No classPtr ? */
    if (!classPtr)
    {
	/* Search for the class */
	if (!(classPtr = (Class *)FindName (PublicClassList, classID)) )
	    return (NULL); /* Nothing found */
    }

    /* Put the classPtr in our dummy object */
    carrier.o_Class = classPtr;

    /* Try to create a new object */
    if ((object = (Object *) DoMethod (BASEOBJECT(&carrier), OM_NEW,
	    tagList, NULL)))
    {
	OCLASS(object) = classPtr;

	/* One more object */
	classPtr->cl_ObjectCount ++;
    }

    return (object);
    __AROS_FUNC_EXIT
} /* NewObjectA */
