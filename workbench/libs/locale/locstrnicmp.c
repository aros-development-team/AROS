/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocStrnicmp - locale.library's private replacement
    	  of utility.library/Strnicmp function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

extern struct LocaleBase *globallocalebase;

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH3(LONG, LocStrnicmp,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, string1, A0),
	AROS_LHA(CONST_STRPTR, string2, A1),
	AROS_LHA(LONG        , length , D0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 32, Locale)

/*  FUNCTION
    	See utility.library/Strnicmp
	
    INPUTS
    	See utility.library/Strnicmp

    RESULT

    NOTES
    	This function is not called by apps directly. Instead utility.library/Strnicmp
	is patched to use this function. This means, that the LocaleBase parameter
	above actually points to UtilityBase!!! But I may not rename it, because then
	no entry for this function is generated in the Locale functable by the
	corresponding script!
	
    EXAMPLE

    BUGS

    SEE ALSO
	utility.library/Strnicmp(), locale.library/StrnCmp().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#define LocaleBase globallocalebase

    LONG retval;
    
    REPLACEMENT_LOCK;
    
    retval = StrnCmp((struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale,
    	    	     (STRPTR)string1,
		     (STRPTR)string2,
		     length,
		     SC_ASCII);
    
    REPLACEMENT_UNLOCK;
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocStrnicmp */

#undef LocaleBase
