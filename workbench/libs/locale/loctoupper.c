/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocToUpper - locale.library's private replacement
    	  of utility.library/ToUper function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define	DEBUG_CONVTOUPPER(x)	;

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_PLH1(ULONG, LocToUpper,

/*  SYNOPSIS */
	AROS_LHA(ULONG, character, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 35, Locale)

/*  FUNCTION
    	See utility.library/ToUpper
	
    INPUTS
    	See utility.library/ToUpper

    RESULT

    NOTES
    	This function is not called by apps directly. Instead dos.library/DosGet-
	LocalizedString is patched to use this function. This means, that the
	LocaleBase parameter above actually points to UtilityBase, so we make use of 
	the global LocaleBase variable. This function is marked as private,
	thus the headers generator won't mind the different basename in the header.
	
    EXAMPLE

    BUGS

    SEE ALSO
	utility.library/ToUpper(), locale.library/ConvToUpper().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    
    REPLACEMENT_LOCK;

    DEBUG_CONVTOUPPER(dprintf("locToUpper: char 0x%lx\n",
			character));

    DEBUG_CONVTOUPPER(dprintf("locToUpper: locale 0x%lx\n",
			(struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale));

    retval = ConvToUpper((struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale, character);

    DEBUG_CONVTOUPPER(dprintf("locToUpperr: retval 0x%lx\n",
			retval));
    REPLACEMENT_UNLOCK;
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocToUpper */
