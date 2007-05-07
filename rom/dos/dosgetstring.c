/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
	AROS_LHA(LONG, stringNum, D0),

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
    	DosGetLocalizedString()
	
    INTERNALS
	This is dosPrivate5()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR retval;

    retval = DosGetLocalizedString(stringNum);

    if (!retval)
    {
    	struct EString *es = EString;
	
	while(es->Number)
	{
	    if(es->Number == stringNum)
	    {
		retval = es->String;
		break;
	    }
	    es++;
	}
    }
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* DosGetString */

