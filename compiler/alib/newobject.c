/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/11/28 10:40:30  aros
    A couple of new functions in amiga.lib

    Easier code to handle stacktags and stackmethods.

    Revision 1.3  1996/11/25 10:53:18  aros
    Allow stacktags on special CPUs

    Revision 1.2  1996/09/17 18:05:45  digulla
    Same names for same parameters

    Revision 1.1  1996/08/28 17:52:29  digulla
    First step to implement amiga.lib
    BOOPSI Utility functions


    Desc:
    Lang: english
*/
#include <intuition/classes.h>
#include <intuition/intuitionbase.h>
#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <clib/intuition_protos.h>

	APTR NewObject (

/*  SYNOPSIS */
	struct IClass * classPtr,
	UBYTE	      * classID,
	ULONG		tag1,
	...		)

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
    AROS_SLOWSTACKTAGS_PRE(tag1)
    NewObjectA (classPtr, classID, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* NewObject */
