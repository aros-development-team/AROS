/*
    Copyright (C) 2001 AROS - The Amiga Research OS
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

extern struct LocaleBase *globallocalebase;

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH1(UBYTE, LocToLower,

/*  SYNOPSIS */
	AROS_LHA(ULONG, character, D0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 34, Locale)

/*  FUNCTION
    	See utility.library/ToLower
	
    INPUTS
    	See utility.library/ToLower

    RESULT

    NOTES
    	This function is not called by apps directly. Instead utility.library/ToLower
	is patched to use this function. This means, that the LocaleBase parameter
	above actually points to UtilityBase!!! But I may not rename it, because then
	no entry for this function is generated in the Locale functable by the
	corresponding script!
	
    EXAMPLE

    BUGS

    SEE ALSO
	utility.library/ToLower(), locale.library/ConvToLower().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#define LocaleBase globallocalebase

    UBYTE retval;
    
    REPLACEMENT_LOCK;    
    retval = ConvToLower((struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale, character);   
    REPLACEMENT_UNLOCK;
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocToLower */

#undef LocaleBase
