/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: DosGetString() - Support for localized strings.
    Lang: english
*/

#include "dos_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(STRPTR, DosGetString,

/*  SYNOPSIS */
	AROS_LHA(ULONG, stringNum, D0),

/* LOCATION */
	struct DosLibrary *, DOSBase, 163, Dos)

/*  FUNCTION
	Internal DOS function, will return the string corresponding to
	the number stringNum.

    INPUTS
	stringNum   -   The number of the string you want.

    RESULT
	A pointer to a string, or NULL if no string could be found with
	a matching number.

    NOTES
	Error strings will ALWAYS be less than 80 characters, and should
	ideally be less than 60 characters.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This is dosPrivate5()

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *, DOSBase)

    struct EString *es = EString;

    while(es->Number)
    {
	if(es->Number == stringNum)
	    return es->String;
	es++;
    }
    return NULL;

    AROS_LIBFUNC_EXIT
} /* DosGetString */

