/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:�DosGetString() - Support for localized strings.
    Lang: english
*/

#include "dos_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(STRPTR, DosGetLocalizedString,

/*  SYNOPSIS */
	AROS_LHA(ULONG, stringNum, D0),

/* LOCATION */
	struct DosLibrary *, DOSBase, 154, Dos)

/*  FUNCTION
	Internal DOS function, will return the localized string corresponding to
	the number stringNum. But only once IPrefs has installed locale.library's
	replacement function. Until then this function will just return NULL.

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
    	DosGetString()
	
    INTERNALS
	This is dosPrivate4()

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *, DOSBase)

    return NULL;
    
    AROS_LIBFUNC_EXIT
    
} /* DosGetString */

