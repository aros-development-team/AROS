/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocStricmp - locale.library's private replacement
    	  of utility.library/Stricmp function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define	DEBUG_STRCMP(x)	;

extern struct LocaleBase *globallocalebase;

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH2(LONG, LocStricmp,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, string1, A0),
	AROS_LHA(CONST_STRPTR, string2, A1),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 33, Locale)

/*  FUNCTION
    	See utility.library/Stricmp
	
    INPUTS
    	See utility.library/Stricmp

    RESULT

    NOTES
    	This function is not called by apps directly. Instead utility.library/Stricmp
	is patched to use this function. This means, that the LocaleBase parameter
	above actually points to UtilityBase!!! But I may not rename it, because then
	no entry for this function is generated in the Locale functable by the
	corresponding script!
	
    EXAMPLE

    BUGS

    SEE ALSO
	utility.library/Stricmp(), locale.library/StrnCmp().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#define LocaleBase globallocalebase

    LONG retval;
    
    REPLACEMENT_LOCK;
    
    DEBUG_STRCMP(dprintf("locStrCmp: <%s> <%s>\n",
			string1,
			string2));

    DEBUG_STRCMP(dprintf("locStrCmp: CurrentLocale 0x%lx\n",
			(struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale));

    retval = StrnCmp((struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale,
    	    	     (STRPTR)string1,
		     (STRPTR)string2,
		     -1,
		     SC_ASCII);
    
    DEBUG_STRCMP(dprintf("StrCmp: retval 0x%lx\n",
			retval));

    REPLACEMENT_UNLOCK;
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocStricmp */

#undef LocaleBase
