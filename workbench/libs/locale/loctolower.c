/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocToLower - locale.library's private replacement
    	  of utility.library/ToLower function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define	DEBUG_CONVTOLOWER(x)	;

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_PLH1(ULONG, LocToLower,

/*  SYNOPSIS */
	AROS_LHA(ULONG, character, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 34, Locale)

/*  FUNCTION
    	See utility.library/ToLower
	
    INPUTS
    	See utility.library/ToLower

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
	utility.library/ToLower(), locale.library/ConvToLower().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    
    REPLACEMENT_LOCK;    

    DEBUG_CONVTOLOWER(dprintf("locToLower: char 0x%lx\n",
			character));

    DEBUG_CONVTOLOWER(dprintf("locToLower: locale 0x%lx\n",
			(struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale));

    retval = ConvToLower((struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale, character);   

    DEBUG_CONVTOLOWER(dprintf("locToLower: retval 0x%lx\n",
			retval));

    REPLACEMENT_UNLOCK;
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocToLower */

#undef LocaleBase
