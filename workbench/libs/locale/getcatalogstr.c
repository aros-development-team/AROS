/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH3(CONST_STRPTR, GetCatalogStr,

/*  SYNOPSIS */
	AROS_LHA(const struct Catalog *, catalog,       A0),
	AROS_LHA(ULONG,                  stringNum,     D0),	/* Not a typo! Needs to be unsigned for ICF_INORDER */
	AROS_LHA(CONST_STRPTR,           defaultString, A1),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 12, Locale)

/*  FUNCTION
	This function will return the string specified by the
	stringNum from the given message catalog, or the defaultString
	if the string could not be found.

	If the catalog == NULL, then the defaultString will also be
	returned.

    INPUTS
	catalog -	Message catalog to search. May be NULL.
	stringNum -	ID of the string to find.
	defaultString - String to return in case catalog is NULL or
			string could not be found.

    RESULT
	A pointer to a READ ONLY NULL terminated string. This string
	pointer is valid as long as the catalog remains open.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenCatalog(), CloseCatalog()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)

    CONST_STRPTR str = defaultString;

    if(catalog != NULL)
    {
	struct CatStr *cs = IntCat(catalog)->ic_CatStrings;
	ULONG	      numstrings = IntCat(catalog)->ic_NumStrings;
    	ULONG 	      i = 0;

	for(i = 0; i < numstrings; i++, cs++)
	{
	    if(cs->cs_Id == stringNum)
	    {
		str = cs->cs_String;
		break;
	    }
	    else
	    {
		if((IntCat(catalog)->ic_Flags & ICF_INORDER) &&
		   (cs->cs_Id > stringNum))
		{
		    break;
		}
	    }
	}
    }

    return str;

    AROS_LIBFUNC_EXIT

} /* GetCatalogStr */
