/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocStrToDateGetCharFunc - locale.library's private replacement
              of dos.library/DosGetLocalizedString function. IPrefs will install
          the patch.
          
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include "locale_intern.h"

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_PLH1(CONST_STRPTR, LocDosGetLocalizedString,

/*  SYNOPSIS */
        AROS_LHA(LONG, stringNum, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 38, Locale)

/*  FUNCTION
        See dos.library/DosGetString
        
    INPUTS
        See dos.library/DosGetString

    RESULT

    NOTES
        This function is not called by apps directly. Instead
        dos.library/DosGetLocalizedString is patched to use this function.
        This means that the LocaleBase parameter above actually points to
        DOSBase, so we make use of the global LocaleBase variable. This
        function is marked as private, therefore the headers generator won't
        mind the different basename in the header.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/DosGetString()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG *p = DOSBase->dl_Errors->estr_Nums;
    UBYTE *q = DOSBase->dl_Errors->estr_Strings;
    STRPTR defstr = NULL;

    /* We should really call the original DosGetString(), but ah well... */
    do
    {
        LONG n = p[0];
        LONG m = p[1];

        while (n <= m)
        {
            if (n == stringNum)
            {
                defstr = q + 1;
                break;
            }

            q += q[0] + 1;
            ++n;
        }

        p += 2;
    }
    while (p[0] != 0);

    return GetCatalogStr(IntLB(LocaleBase)->lb_CurrentLocale->il_DosCatalog,
        stringNum, defstr);

    AROS_LIBFUNC_EXIT
    
}

#undef LocaleBase
