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


#define	DEBUG_STRNCMP(x)	;
 
 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_PLH3(LONG, LocStrnicmp,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, string1, A0),
	AROS_LHA(CONST_STRPTR, string2, A1),
	AROS_LHA(LONG        , length , D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 32, Locale)

/*  FUNCTION
    	See utility.library/Strnicmp
	
    INPUTS
    	See utility.library/Strnicmp

    RESULT

    NOTES
    NOTES
    	This function is not called by apps directly. Instead dos.library/DosGet-
	LocalizedString is patched to use this function. This means, that the
	LocaleBase parameter above actually points to UtilityBase, so we make use of 
	the global LocaleBase variable. This function is marked as private,
	thus the headers generator won't mind the different basename in the header.
	
    EXAMPLE

    BUGS

    SEE ALSO
	utility.library/Strnicmp(), locale.library/StrnCmp().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG retval;
    
    REPLACEMENT_LOCK;
    
    DEBUG_STRNCMP(dprintf("locStrnCmp: <%s> <%s> len %ld\n",
			string1,
			string2,
			length));

    DEBUG_STRNCMP(dprintf("locStrnCmp: CurrentLocale 0x%lx\n",
			(struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale));

    retval = StrnCmp((struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale,
    	    	     (STRPTR)string1,
		     (STRPTR)string2,
		     length,
		     SC_ASCII);

    DEBUG_STRNCMP(dprintf("StrnCmp: retval 0x%lx\n",
			retval));

    REPLACEMENT_UNLOCK;
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocStrnicmp */
