/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocStrToDateGetCharFunc - locale.library's private replacement
    	  of dos.library/DosGetLocalizedString function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH1(STRPTR, LocDosGetLocalizedString,

/*  SYNOPSIS */
	AROS_LHA(LONG, stringNum, D0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 38, Locale)

/*  FUNCTION
    	See dos.library/DosGetLocalizedString
	
    INPUTS
    	See dos.library/DosGetLocalizedString

    RESULT

    NOTES
    	This function is not called by apps directly. Instead dos.library/DosGet-
	LocalizedString is patched to use this function. This means, that the
	LocaleBase parameter above actually points to DOSBase!!! But I may not
	rename it, because then no entry for this function is generated in the
	Locale functable by the corresponding script!
	
    EXAMPLE

    BUGS

    SEE ALSO
	dos.library/DosGetString(), dos.library/DosGetLocalizedString()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
extern struct LocaleBase *globallocalebase;
#define LocaleBase globallocalebase

    struct Catalog *catalog = (struct Catalog *)DOSBase->dl_Errors;
    
    return GetCatalogStr(catalog, ((stringNum >= 0) ? stringNum : -stringNum), NULL);
    
    AROS_LIBFUNC_EXIT
    
} /* LocDosGetLocalizedString */

#undef LocaleBase
