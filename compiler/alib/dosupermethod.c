/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: german
*/
#include <intuition/classes.h>
#include <stdarg.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <proto/alib.h>

	IPTR DoSuperMethodA (

/*  SYNOPSIS */
	Class  * cl,
	Object * obj,
	Msg	 message)

/*  FUNCTION
	Sendet eine BOOPSI-Message an ein BOOPSI-Object als ob dieses eine
	Instanz seiner SuperKlasse waere.

    INPUTS
	cl - Class des Objects.
	obj - Das Object, auf welches sich die Operation bezieht.
	message - Die Method-Message. Das erste ULONG der Message definiert den
		Typ, der Rest haengt von der Klasse ab.

    RESULT
	Der Rueckgabewert haengt von der Methode ab. Bei OM_NEW ist es z.B. ein
	Zeiger auf das neu generierte Object; andere Methoden verwenden andere
	Ergebnis-Werte. Diese werden bei der Beschreibung der Klasse definiert
	und sind dort nachzulesen.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs(), GetAttr(), DisposeObject(), DoMethod(),
	"Basic Object-Oriented Programming System for Intuition" und das
	"boopsi Class Reference" Dokument.

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    return CallHookPkt ((struct Hook *)cl->cl_Super, obj, message);
} /* DoSuperMethodA */


ULONG DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...)
{
    AROS_SLOWSTACKMETHODS_PRE(MethodID)
    retval = CallHookPkt ((struct Hook *)cl->cl_Super
	, obj
	, AROS_SLOWSTACKMETHODS_ARG(MethodID)
    );
    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperMethod */

