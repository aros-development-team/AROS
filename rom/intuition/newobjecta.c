/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1997/03/20 16:05:08  digulla
    Fixed bug: Added FindClass()


    Desc: Create a new BOOPSI object
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#include "boopsi.h"

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	AROS_LH3(APTR, NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(struct IClass  *, classPtr, A0),
	AROS_LHA(UBYTE          *, classID, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    Object * object;
    struct _Object carrier;

    /* No classPtr ? */
    if (!classPtr)
	classPtr = FindClass (classID, IntuitionBase);

    if (!classPtr)
	return (NULL); /* Nothing found */

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
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
