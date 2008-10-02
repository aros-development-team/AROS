/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DosGetLocalizedString() - Support for localized strings.
    Lang: english
*/

#include "dos_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(STRPTR, DosGetLocalizedString,

/*  SYNOPSIS */
	AROS_LHA(LONG, stringNum, D1),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return NULL;
    
    AROS_LIBFUNC_EXIT
    
} /* DosGetString */

